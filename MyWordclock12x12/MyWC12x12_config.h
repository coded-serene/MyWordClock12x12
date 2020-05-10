#ifndef CONFIG_H
#define CONFIG_H

//
// Feature Toggles aktivieren mit define, deaktivieren mit undef
//
#define GEBURTSTAGE 1
#define LAUFSCHRIFT 1
#define TEMPERATURE 1
#define FEATURE_OTA 1

//#undef GEBURTSTAGE
//#undef LAUFSCHRIFT
//#undef TEMPERATURE
//#undef LOCALE
//#undef FEATURE_OTA


#define FASTLED_INTERNAL			// unterdrücken der PRAGMA-Meldungen in FastLED
#define FASTLED_ALLOW_INTERRUPTS 0	// gegen Glitching

#include <FastLED.h>            	// http://fastled.io      https://github.com/FastLED/FastLED

// #define LEDCOLORORDER RGB			// Je nach LED-Stripe kann RGB oder GRB (oder vllt. auch ein total anderer Typ auszuwählen sein
#define LEDCOLORORDER GRB

#define DATA_PIN 4

//
// Allgemeine Festlegungen
// Größe der Matrix, damit Azahl der LEDs (+4 wg MiutenLEDs) und Anschluss der LED-Kette an den D2
//
#define GRID_ROWS 12
#define GRID_COLS 12
#define NUM_LEDS (GRID_ROWS * GRID_COLS) + 4

#define CONFIGFILE "/wordclock_config.json"

#define MODE_TEMP_FIRST     1
#define MODE_TEMP 			2
#define MODE_BIRTHDAY_FIRST 3
#define MODE_BIRTHDAY       4
#define MODE_TIME_FIRST     5
#define MODE_TIME           6

typedef struct {
  CRGB color_bg;
  CRGB color_fg;
  int brightness;
  int timezone;

  bool dunkelschaltung_active;
  int dunkelschaltung_start;
  int dunkelschaltung_end;
  int dunkelschaltung_brightness;

#ifdef TEMPERATURE
  int temp_active;
  CRGB temp_color;
  String city;
#endif
  int locale;
#ifdef GEBURTSTAGE
  String geb_name_1;
  String geb_name_2;
  String geb_name_3;
  String geb_name_4;
  String geb_name_5;
  int geb_1;
  int geb_2;
  int geb_3;
  int geb_4;
  int geb_5;
#endif
  int herz;
  int dat_herz;
} config_t;

#define HERZ_AUS 			0		// grundsätzlich ist das Herz aus (Hintergrundfarbe)
#define HERZ_AN 			1		// das Herz wird in Vordergrundfarbe angezeigt
#define HERZ_ROT 			2		// das Herz wird rot angezeigt
#define HERZ_DATUM 			3		// das Herz wird am <DATUM> in rot angezeigt
#define HERZ_STD_DATUM		4		// das Herz wird in Vordergrundfarbe angezaigt, aber am <DATUM> in rot


// Sprachlokalisierungen, Deutsch und Fränkisch :-)
#define L_DEUTSCHLAND 		0
#define L_FRANKEN 			1
#define L_W_DEUTSCHLAND		"Deutsch"
#define L_W_FRANKEN			"Fr&aumlnkisch"
#define L_S_DEUTSCHLAND     25
#define L_S_FRANKEN 		15

#define BRIGHTNESS_AUTO 	-1
#define BRIGHTNESS_DEFAULT	75
#define BRIGHTNESS_MAX		100

#define LAUFSCHRIFT_SPEED 	150

#endif
