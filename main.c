/*
 *  DATE: 2013-07-22
 *  AUTEUR: Jacques Deschênes
 *  DESCRIPTION: Démo interface avec clavier PS/2
 *  MONTAGE:
 *   	LAUNCHPAD				CLAVIER
 *   	 G2553				mini DIN-6 (PS/2)
 *        P1.0	---------------	1 clock
 *   	  P1.1	---------------	3 data
 *   	 0 volt ---------------	2 V-
 *   	3,6volt ---------------	5 V+
 *   	 P2.2   --------------- haut-parleur
 */

#include <msp430.h> 

#include "ps2-kbd.h"
#include "qwerty.h"



#define OK_LED BIT6
#define AUDIO_OUT BIT2 // sortie audio pour test3, clavier musical

void configure_clock(){
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;
}// configure_clock();

void delay_ms(unsigned int ms){
	while (ms--)
		_delay_cycles(16000);
} // delay_ms()

void kbd_error(){
	P1OUT |= OK_LED;
	LPM4;
}

typedef struct letter2tone{// transcription de lettre à note
	int letter;
	int tone;
}t_letter2tone;

const t_letter2tone scale[]={ // table de transcription code clavier vers notes
		{0x1c,3822}, //do4
		{0x1d,3608}, //do#4
		{0x1b,3405}, //ré4
		{0x24,3214}, //ré#4
		{0x23,3034}, //mi4
		{0x2b,2863}, //fa4
		{0x2c,2703}, //fa#4
		{0x34,2551}, //sol4
		{0x35,2408}, //sol#4
		{0x33,2273}, //la4
		{0x3c,2145}, //la#4
		{0x3b,2025}, //si4
		{0x42,1911}, //do5
		{0x44,1804}, //do#5
		{0x4b,1703}, //ré5
		{0x4d,1607}, //ré#5
		{0,0}
};

int get_tone(int code){
	int i, tone=0;
	i=0;
	while (scale[i].letter){
		if (scale[i].letter==(code&0xff)){
			tone=scale[i].tone;
			break;
		}else{
			i++;
		}

	}
	return tone;
}

volatile unsigned char vib_level=1; // 0 - 6
volatile  char step;
/*
 * main.c
 */
void main(void) {
	int c, run,tone;

	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    configure_clock();
    BCSCTL3 = XCAP_3; /* 12.5pF pour que LFXT1 fonctionne avec
                         le crystal 32768Hz fourni avec le launchpad */
    IE1 |= OFIE;
    _enable_interrupts();
    P1REN &= ~OK_LED; //pullup sur les Entrées
    P1OUT &= ~OK_LED; // non utilisées.
    P1DIR |= OK_LED;
    P1OUT &= ~OK_LED;
    if (init_ps2_kbd()){
    	P1OUT |= OK_LED;
    	delay_ms(500);
    	P1OUT &= ~OK_LED;
    	int led=1;
    	while (1){ // test1: allumes les 3 LEDs en séquence
    		if (!set_kbd_leds(led)){
    			kbd_error();
    		}
    		led <<= 1;
    		if (led>4)led=1;
    		delay_ms(250);
    		c=get_scancode();
    		if (c){
    			while (c=get_scancode());
    			break;
    		}

    	} // test1
    	if (!set_kbd_leds(0)) kbd_error();
    	run=1;
    	while (run){ // test2: répond aux touches CAPSLOCK, NUMLOCK,SCROLL
    		c=get_scancode();
    		switch(c&0x1ff){
    		case 0:
    			break;
    		case CAPS_LOCK:
    			if (c & REL_BIT){
    				kbd_leds ^= F_CAPS;
    				if (!set_kbd_leds(kbd_leds))
    					kbd_error();
    			}
    			break;
    		case NUM_LOCK:
    			if (c & REL_BIT){
    				kbd_leds ^= F_NUM;
    				if (!set_kbd_leds(kbd_leds))
    					kbd_error();
    			}
    			break;
    		case SCROLL_LOCK:
    			if (c & REL_BIT){
    				kbd_leds ^= F_SCROLL;
    				if (!set_kbd_leds(kbd_leds))
    					kbd_error();
    			}
    			break;
    		case KEY_ESC:
    			if (c & REL_BIT){
    				run=0;
    			}
    			break;
    		}
    	}//while (1) test2
    	// test 3 , clavier musical
    	if (!set_kbd_leds(7)) kbd_error();
    	delay_ms(500);
    	if (!set_kbd_leds(0)) kbd_error();
    	run=1;
		// configuration TA1 pour sortie tonalités
		P2DIR |= AUDIO_OUT;
		P2OUT &= ~AUDIO_OUT;
		// timer a0 pour vibrato
		TA0CTL = TASSEL_1+MC_1; // ACLK, up to ccr0
		TA0CCR0 = 8192;
		TA0CCTL0 |= CCIE;
		// timer a1 notes
		TA1CTL = TASSEL_2+ID_3+MC_3;  // SMCLK/8
		TA1CCTL1= OUTMOD_2; //Toggle/reset
		step=0;
    	while (run){ // test 3
    		c=get_scancode();
    		tone=get_tone(c);
    		if (tone){
    				if (c & REL_BIT){
    					P2SEL &= ~AUDIO_OUT;
    				}else{
    					if (!(kbd_leds&F_CAPS+F_SHIFT)){
    						TA1CCR0 = tone;
    						TA1CCR1 = tone>>1;
    					}else{
    						TA1CCR0 = tone>>1;
    						TA1CCR1 = tone>>2;
    					}
    					P2SEL |= AUDIO_OUT;
    				}
    		}else{ // else if (a)
    			switch(c&0x1ff){
    			case 0:
    				break;
        		case CAPS_LOCK:
        			if (c & REL_BIT){
        				kbd_leds ^= F_CAPS;
        				if (!set_kbd_leds(kbd_leds))
        					kbd_error();
        			}
        			break;
        		case F1: // modification du timbre
        			if (c & REL_BIT){
        				kbd_leds ^= F_SCROLL;
        				TA1CCTL1 ^= OUTMOD0; // modifie le timbre
        				if (!set_kbd_leds(kbd_leds))
        					kbd_error();
        			}
        			break;
        		case LSHIFT:
        			if (!(c & REL_BIT)){
        				kbd_leds |= F_SHIFT;
        			}else{
        				kbd_leds &= ~F_SHIFT;
        			}
        		case UP_ARROW: // augmente fréquence vibrato
        			if ((c & REL_BIT) && (TACCR0>700))
        				TACCR0 -= TACCR0>>3;
        			break;
        		case DOWN_ARROW: // diminue fréquence vibrato
        			if ((c & REL_BIT) && (TACCR0<8192))
        				TACCR0 += TACCR0>>3;
        			break;
        		case LEFT_ARROW: // diminu profondeur vibrato
        			if ((c & REL_BIT) && vib_level>0){
        				vib_level--;
        			}
        			break;
        		case RIGHT_ARROW: // augmente profondeur vibrato
        			if ((c & REL_BIT) && vib_level<7){
        				vib_level++;
        			}
        			break;
    			}
    		}// else if (a)
    	} // test 3, clavier musical
    } // if (init_ps2_kbd())
    else{
    	kbd_error(); // erreur clavier
    }
	while (1);
}


#pragma vector=NMI_VECTOR /* interruption non masquable. Va se produire si
                           * l'oscillateur LFXT1 tombe en panne. */

__interrupt void nmi_ (void)
{
  do
  {
    IFG1 &= ~OFIFG;              // remet à zéro l'indicateur erreur Osc.
    _delay_cycles(0xFFFF);  // délais pour laissé du temps à OFIFG de
            				// se remettre a 1 s'il y a encore faute.
  } while (IFG1 & OFIFG);  // boucle tant qu'il y a erreur
  IE1 &= ~OFIE;           // désactive l'interruption sur erreur osc.
}

// vecteurs d'interruptions inutilisés
#pragma vector=PORT2_VECTOR
#pragma vector=ADC10_VECTOR
#pragma vector=USCIAB0TX_VECTOR
#pragma vector=USCIAB0RX_VECTOR
#pragma vector=TIMER0_A1_VECTOR
#pragma vector=WDT_VECTOR
#pragma vector=COMPARATORA_VECTOR
#pragma vector=TIMER1_A1_VECTOR
#pragma vector=TIMER1_A0_VECTOR
__interrupt void not_used(void){
	return;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void ta0_ccr0_isr(void){
	DCOCTL ^= vib_level;
}// ta0_ccr0_isr()
