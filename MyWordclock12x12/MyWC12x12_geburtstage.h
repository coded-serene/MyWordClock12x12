#define GEBURTSTAGE_H
//#include "MyWC12x12_config.h"

#ifdef GEBURTSTAGE

#define MODE_BIRTHDAY 2

#ifdef GEBURTSTAGE_CPP

//
// Für die Geburtstage soll die Farbe der aktivierten LEDs wie ein Regenbogen durchscrollen.
// In dieser Matrix wird vermerkt, welche LED aktiviert ist.
//
uint8_t mask[NUM_LEDS];

#else
//
// Für die Geburtstage soll die Farbe der aktivierten LEDs wie ein Regenbogen durchscrollen.
// In dieser Matrix wird vermerkt, welche LED aktiviert ist.
//
extern uint8_t mask[];

extern bool isGeburtstagheut(int, int);
extern String getGebName(int, int);
extern void setRainbowColor();
#endif
#endif