#define GEBURTSTAGE_CPP
#include "MyWC12x12_config.h"

#ifdef GEBURTSTAGE

#include <FastLED.h>            // http://fastled.io      https://github.com/FastLED/FastLED

#include "MyWC12x12.h"
#include "MyWC12x12_geburtstage.h"


bool isGeburtstagheut(int tag, int monat) {
  int heute;

  heute = tag * 100 + monat;

  if ( heute == CONFIG.geb_1 || heute == CONFIG.geb_2 || heute == CONFIG.geb_3 || heute == CONFIG.geb_4 || heute == CONFIG.geb_5 )
    return true;
  else
    return false;

  return false;
}

String getGebName(int tag, int monat) {
  int heute;

  heute = tag * 100 + monat;

  if ( heute == CONFIG.geb_1 ) return (CONFIG.geb_name_1);
  if ( heute == CONFIG.geb_2 ) return (CONFIG.geb_name_2);
  if ( heute == CONFIG.geb_3 ) return (CONFIG.geb_name_3);
  if ( heute == CONFIG.geb_4 ) return (CONFIG.geb_name_4);
  if ( heute == CONFIG.geb_5 ) return (CONFIG.geb_name_5);

  return "";
}

void setRainbowColor() {
  int i;
  CHSV hsv;
  CHSV hsv2;
  int ini = false;


  for (i = 0; i < NUM_LEDS; i++) {
    if (mask[i]) {
      if (!ini) {
        ini = true;
        hsv2 = rgb2hsv_approximate(leds[i]);
        hsv.hue = hsv2.hue + 5; //deltahue
        hsv.val = 255;
        hsv.sat = 240;
      }

      leds[i] = hsv;
      hsv.hue += 5; //deltahue
    }
  }
}
#endif