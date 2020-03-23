#define TEMPERATUR_H
//#include "MyWC12x12_config.h"

#ifdef TEMPERATURE

#define MODE_TEMP 				1

#define ERR_TEMP  				1000 		 // Fehlerwert, wenn die Temperatur nicht ermittelt werden kann

#define TEMPERATURE_AUS 		0
#define TEMPERATURE_MINUTE 		1
#define TEMPERATURE_5MINUTE 	2
#define TEMPERATURE_DURATION 	5000

#ifdef TEMPERATUR_CPP

#else
extern int temperature;
extern String temperature_real_location;

extern void testTemperatur();

extern CRGB GetTemperatureColor(int t);

extern String GetTemperatureRealLocation(String s);
extern int GetTemperature(String s);
extern void showTemperature(int t, CRGB c);

#endif

#endif