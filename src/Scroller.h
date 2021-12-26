

#ifndef __Scroller_H__
#define __Scroller_H__ 

/*  
 ~~~~~~~~~~~~~~~~
 Wordclock-Project
 ~~~~~~~~~~~~~~~~
© SERENE - Rene Lang 2021 (http://withstupid.net)
 
*/


#include "FastLED.h"
#include "MyWC12x12_config.h"


#include "LEDMatrix.h"
#include "LEDText.h"

class Scroller
{
private:
   static void resetLEDs(void);
public:
   static void ScrollerInitText(String text, CRGB c = CRGB::White);
   static int ScrollerStepText();
   static void ScrollerShowAndRun(String text, CRGB c = CRGB::White) ;
   static void ScrollerTest(String text);
   Scroller();
   ~Scroller();
   
};

#endif 
