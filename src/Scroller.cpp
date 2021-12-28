#include <Arduino.h>
#include "Scroller.h"


extern CRGB leds[NUM_LEDS];   // defined in MyWC12x12.cpp
extern char buffer[256];      // defined in MyWC12x12.cpp
extern cLEDText ScrollingMsg; // defined in MyWC12x12.cpp

Scroller::Scroller()
{
  
}

Scroller::~Scroller()
{
}
void Scroller::resetLEDs() {
  int i;

  for (i = 0; i < NUM_LEDS; i++) 
  {
    leds[i] = CRGB::Black;
  }
}

void Scroller::ScrollerInitText(String text, CRGB c /*= CRGB::White*/) {
    Serial.println("Scroller::ScrollerInitText: " + text);
    text = "   " + text;      // smooth in

    text.toCharArray(buffer, 256);

    resetLEDs();

    ScrollingMsg.SetText((unsigned char *)buffer, strlen(buffer));
    ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, c.r, c.g, c.b);
}

int Scroller::ScrollerStepText() {
	
  int i;

	i=ScrollingMsg.UpdateText();
  Serial.println("Scroller::ScrollerStepText: " + i);  
	if (i!= -1) {
		FastLED.show();
	}

	return i;
}

void Scroller::ScrollerShowAndRun(String text, CRGB c/* = CRGB::White*/) {
    char buffer[256];

    Serial.println("Scroller::ScrollerShowAndRun: " + text);  

    text = "   " + text;      // smooth in

    text.toCharArray(buffer, 256);

    resetLEDs();

    ScrollingMsg.SetText((unsigned char *)buffer, strlen(buffer));
    ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, c.r, c.g, c.b);

    while (ScrollingMsg.UpdateText() != -1)
    {
        FastLED.show();
        delay(LAUFSCHRIFT_SPEED);
    }
    Serial.println("Scroller::ScrollerShowAndRun: completed.");
}

void Scroller::ScrollerTest(String text) {
	Scroller::ScrollerInitText(text, CRGB::White);

	while (Scroller::ScrollerStepText() != -1) {
		delay(LAUFSCHRIFT_SPEED);
	}
}






