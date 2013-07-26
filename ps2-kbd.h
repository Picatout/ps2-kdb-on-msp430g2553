/*
 * ps2-kbd.h
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

#ifndef PS2_KBD_H_
#define PS2_KBD_H_

#define MCLK_FRQ	16  //fréquence en Mhz

// structure pour la table de transcription des codes clavier
typedef struct scan2key{
	unsigned int code:8;
	unsigned int ascii:8;
}t_scan2key;


// commandes contrôle clavier
#define KBD_RESET 0xFF  // commande RAZ clavier
#define KBD_LED 0xED    // commande de contrôle des LEDS

#define XT_BIT	(1<<8)  // indicateur code étendu
#define REL_BIT (1<<15) // indicateur touche relâchée

// touches spéciales
#define CAPS_LOCK		0x58
#define NUM_LOCK		0x77
#define SCROLL_LOCK		0x7E
#define LSHIFT			0x12
#define LCTRL			0x14
#define LALT			0x11
#define KEY_ESC			0x76
#define F1				0x05
#define F2				0x06
#define F3				0x04
#define F4				0x0c
#define F5				0x03
#define F6				0x0b
#define F7				0x83
#define F8				0x0a
#define F9				0x01
#define F10				0x09
#define F11				0x78
#define F12				0x07
#define PRN				XT_BIT|0x12   // touche PrtSc enfoncée 4 codes: 0xe012e07c, relâchée 6 codes: 0xe0f07ce0f012
#define PAUSE			XT_BIT|0xe1   // touche Pause séquence de 8 codes 0xe11477e1f014f077
#define UP_ARROW		XT_BIT|0x75
#define DOWN_ARROW		XT_BIT|0x72
#define LEFT_ARROW		XT_BIT|0x6B
#define RIGHT_ARROW		XT_BIT|0x74
#define INSERT			XT_BIT|0x70
#define HOME			XT_BIT|0x6c
#define	PGUP			XT_BIT|0x7d
#define PGDN			XT_BIT|0x7a
#define	DEL				XT_BIT|0x71
#define END				XT_BIT|0x69
#define KPDIV			XT_BIT|0x4a
#define KPMUL			0x7c
#define KPMINUS			0x7b
#define KPPLUS			0x79
#define KPENTER			XT_BIT|0x5a
#define KPDOT			0x71
#define KP0				0x70
#define KP1				0x69
#define KP2				0x72
#define KP3				0x7a
#define KP4				0x6b
#define KP5				0x73
#define KP6				0x74
#define KP7				0x6c
#define KP8				0x75
#define KP9				0x7d

// réponses du clavier aux commandes
#define KBD_ACK 0xFA // ACK confirmation envoyé par le clavier
#define KBD_RSND 0xFE // le clavier demande de renvoyer la commande
#define BAT_OK 0xAA // test clavier réussi (Basic Assurance Test)
#define BAT_ERROR 0xFC // échec test clavier


#define KEY_REL 0xF0 // code touche relâchée
#define XTD_KEY 0xE0 // code étendu

#define F_ERROR 1 // erreur paritée dans rx_flags
#define F_RCVD 2 // signal réception d'un octet du clavier dans rx_flags


#define F_SCROLL 1 // bit indicateur scroll_lock dans kbd_leds
#define F_NUM  2 // bit indicateur numlock dans kbd_leds
#define F_CAPS 4 // bit indicateur capslock dans kbd_leds
#define F_SHIFT 8  // bit indicateur shift dans kbd_leds

extern volatile unsigned char rx_flags, kbd_leds;


// keyboard API
int init_ps2_kbd();  // initialisation E/S et RAZ clavier
void enable_keyboard_rx();  // active interruption sur KBD_CLK
void disable_keyboard_rx(); // désactive interruption sur KBD_CLK
int get_scancode();  // obtient le code clavier en tête de la file
int get_key(int);  // obtient la transcription du code en ASCII
int kbd_send(char cmd);  // envoie une commande au clavier
int set_kbd_leds(unsigned int leds_state); // contrôle l'état des LEDS du clavier


#endif /* PS2_KBD_H_ */
