/*
 * ps2-kbd.c
 *
 *  Created on: 2013-07-24
 *      Author: Jacques Deschênes
 *      DESCRIPTION: interface clavier PS/2
 *					 P1.0  signal clock
 *					 P1.1  signal data
 *
 *------------------------------------------------
 *  Références:
 *  	http://retired.beyondlogic.org/keyboard/keybrd.htm#1
 *  	http://www.computer-engineering.org/
 *  	http://en.wikipedia.org/wiki/PS/2_keyboard
 *  	http://wiki.osdev.org/PS2_Keyboard
 *------------------------------------------------
 */
#include <msp430.h>
#include "ps2-kbd.h"

// utilisation du port 1 pour
// l'interface clavier PS/2
#define KBD_CLK BIT0 // signal clock
#define KBD_DAT BIT1 // signal data



volatile unsigned char kbd_queue[32]; // file circulaire pour les codes reçus du clavier.
volatile unsigned char head, tail; // tête et queue de la file
volatile unsigned char  in_byte, bit_cnt, parity, rx_flags, kbd_leds;

void wait_idle(){// attend msec d'inactivité, utilisation du WDT comme minuterie
				 // crystal 32Khz installé
	WDTCTL = WDT_ADLY_16; // délais 16 msec avec crystal 32Khz
	IFG1 &= ~WDTIFG; // RAZ indicateur interruption
	while (!(IFG1 & WDTIFG)) {
		if (!((P1IN & (KBD_CLK+KBD_DAT))==(KBD_CLK+KBD_DAT))){
			IFG1 &= ~WDTIFG;
			WDTCTL = WDT_ADLY_16;
		}
	}
	WDTCTL = WDTPW+WDTHOLD;
} // wait_idle()


int init_ps2_kbd(){ // initialisation du clavier
	volatile unsigned int c;
	P1DIR &= ~(KBD_CLK|KBD_DAT); // les 2 en entrée.
	P1SEL &= ~(KBD_CLK|KBD_DAT);  // pas de périphérique
	P1SEL2 &= ~(KBD_CLK|KBD_DAT); // sur ces 2 E/S
	P1REN &= ~(KBD_CLK|KBD_DAT); // pas de pullup.
	P1IES |= KBD_CLK; // int. sur transition descendante.
	bit_cnt=0;
	head=0;
	tail=0;
	rx_flags=0;
	wait_idle();
	P1IFG &= KBD_CLK;
	P1IE |= KBD_CLK;  // interruption sur le signal clock
	if (!kbd_send(KBD_RESET)){
		return 0;
	}
	while ((rx_flags & F_ERROR+F_RCVD)==0); // attend résultat BAT
	if (rx_flags & F_ERROR)
		return 0;
	c=get_scancode();
	if (c!=BAT_OK)
		return 0;
	return 1;
}// init_ps2_kbd()

inline void enable_keyboard_rx(){ // active l'interruption
	bit_cnt=0;
	P1IFG  &= KBD_CLK;
	P1IE |= KBD_CLK;
} // enable_keyboard()

inline void disable_keyboard_rx(){ // désactive l'interruption
	P1IE &= ~KBD_CLK;
}// disable_keyboard()


int kbd_send(char cmd){ /* envoie d'un caractère de commande au clavier */
	bit_cnt=0;
	parity=0;
	P1IE &= ~KBD_CLK; // désactive les interruptions sur KBD_CLK
	P1OUT &= ~KBD_CLK; // MCU prend le contrôle de la ligne KBD_CLK
	P1DIR |= KBD_CLK;   	//  mis à 0  KBD_CLK
	_delay_cycles(MCLK_FRQ * 150); 	// délais minimum 100µsec marge 50µsec
	P1OUT &= ~KBD_DAT;		// prend le contrôle de la ligne KBD_DAT
	P1DIR |= KBD_DAT;   	// met KBD_DAT à zéro
	P1DIR &= ~(KBD_CLK); 	// libère la ligne clock
	while (!(P1IN & KBD_CLK)); // attend que le clavier mette la ligne clock à 1
	while (bit_cnt<8){      // envoie des 8 bits, moins significatif en premier.
		while (P1IN & KBD_CLK);   // attend clock à 0
		if (cmd&1){
			P1OUT |= KBD_DAT;
			parity++;
		}else{
			P1OUT &= ~KBD_DAT;
		}
		cmd >>= 1;
		while (!(P1IN & KBD_CLK)); // attend clock à 1
		bit_cnt++;				  // un bit de plus envoyé.
	}
	while (P1IN & KBD_CLK);   // attend clock à 0
	if (!(parity & 1)){
		P1OUT |= KBD_DAT;
	}else{
		P1OUT &= ~KBD_DAT;
	}
	while (!(P1IN & KBD_CLK)); // attend clock à 1
	while (P1IN & KBD_CLK);   // attend clock à 0
	P1DIR &= ~KBD_DAT;  		// libère la ligne data
	while (!(P1IN & KBD_CLK)); // attend clock à 1
	while (P1IN & (KBD_DAT+KBD_CLK)); 	// attend que le clavier mette data et clock à 0
	while (!((P1IN & (KBD_DAT+KBD_CLK))==(KBD_DAT+KBD_CLK))); // attend que les 2 lignes reviennent à 1.
	bit_cnt=0;
	P1IFG &= ~KBD_CLK;
	P1IE |= KBD_CLK;
	while ((rx_flags & F_ERROR+F_RCVD)==0); // attend keyboard ACK
	if ((rx_flags & F_ERROR) || (get_scancode()!=KBD_ACK)){
		return 0;
	}else{
		return 1;
	}
}// kbd_send()

#define COMPLETED 1
#define RELEASE 2
#define EXTENDED 4
#define PRN_KEY 8
#define PAUSE_KEY 16
int get_scancode(){ // entier positif si touche enfoncée, entier négatif si touche relachée
	unsigned int i, flags;
	int code;
	code = 0;
	flags=0;
	while (!(flags & COMPLETED)){
		if (head!=tail){
			code = kbd_queue[head];
			head++;
			head &= 31;
			if (code==XTD_KEY){
				flags |= EXTENDED;
			}else if (code==KEY_REL){
				flags |= RELEASE;
			}else if (code==0xE1){ // PAUSE
				for (i=7;i;i--){     // élimine les 7 prochains caractères
					while (head==tail);
					head++;
					head &= 31;
				}
				flags = COMPLETED+PAUSE_KEY;
			}else if ((flags&EXTENDED)&& (code==0x12)){ // touche PRINT SCREEN enfoncée
				for (i=2;i;i--){ // élimine les 2 codes suivants
					while (head==tail);
					head++;
				}
				flags = COMPLETED+PRN_KEY;
			}else if ((flags&EXTENDED)&& (code==0x7c)){ // touche PRINT SCREEN relâchée
				for (i=4;i;i--){ // élimine les 4 codes suivants
					while (head==tail);
					head++;
				}
				flags = COMPLETED+PRN_KEY+RELEASE;
			}else{
				flags |=COMPLETED;
			}
			if (!(flags & COMPLETED)){
				while (head==tail); // attend touche suivante
			}
		}else{
			break;
		}
	}
	if (flags & PAUSE_KEY){
		code = PAUSE;
	}else if (flags & PRN_KEY){
		code = PRN;
	}
	if (flags & RELEASE){
		code |= REL_BIT; // négatif pour touche relâchée
	}
	if (flags & EXTENDED){
		code |= XT_BIT; //
	}
	P1IE &= KBD_CLK;// section critique
	if (head==tail){
		rx_flags &= ~F_RCVD;
	}
	P1IE |= KBD_CLK; // fin section critique
	return code;
}// get_scancode()

extern t_scan2key translate[],alt_char[],xt_char[],shifted_key[];

int get_key(int scancode){
	int a,i;
	a=0;
	if (scancode & XT_BIT){
		i=0;
		while (xt_char[i].code){
			if (xt_char[i].code==scancode){
				a=xt_char[i].ascii;
				break;
			}
			i++;
		} // while (xt_char[i].code)
	}else if (rx_flags & F_SHIFT|F_CAPS){
		i=0;
		while (shifted_key[i].code){
			if (shifted_key[i].code==(scancode&0xff)){
				a=shifted_key[i].ascii;
				break;
			}
			i++;
		}// while (shifted_key.code)
		if (!a){
			i=0;
			while (translate[i].code){
				if (translate[i].code==(scancode&0xff)){
					a=translate[i].ascii;
					break;
				}
				i++;
			}// while (translate.code)
			if (a>='a' && a<='z'){
				a -=32;
			}
		} // if (!a)
	}else{
		i=0;
		while (translate[i].code){
			if (translate[i].code==(scancode&0xff)){
				a=translate[i].ascii;
				break;
			}
			i++;
		}// while (translate.code)
		if (a>='a' && a<='z'){
			a -=32;
		}
	}
	return a|(scancode&0xff00);
}// get_key()

int set_kbd_leds(unsigned int leds_state){
	if (!kbd_send(KBD_LED)){
		return 0;
	}
	if (!kbd_send(leds_state)){
		return 0;
	}
	return 1;
} // set_kbd_leds()

#pragma vector=PORT1_VECTOR
__interrupt void kbd_clk_isr(void){
	if (!(P1IFG & KBD_CLK)) return; // pas la bonne interruption
	P1IFG &= ~KBD_CLK;
	switch (bit_cnt){
	case 0:   // start bit
		if (P1IN & KBD_DAT)
			return; // ce n'est pas un start bit
		parity=0;
		bit_cnt++;
		break;
	case 9:   // paritée
		if (P1IN & KBD_DAT)
			parity++;
		if (!(parity & 1)){
			rx_flags |= F_ERROR;
			P1IE &= KBD_CLK; //désactivation interruptions
		}
		bit_cnt++;
		break;
	case 10:  // stop bit
		kbd_queue[tail]=in_byte;
		tail++;
		tail &=31;
		bit_cnt=0;
		rx_flags |= F_RCVD;
		break;
	default:
		in_byte >>=1;
		if(P1IN & KBD_DAT){
			in_byte |=128;
			parity++;
		}
		bit_cnt++;
	}
} // kbd_clk_isr()

