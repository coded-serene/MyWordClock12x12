#define TEMPERATUR_H
//#include "MyWC12x12_config.h"

#ifdef TEMPERATURE

#define MODE_TEMP 				    2
#define MODE_TEMP_FIRST             1

#define ERR_TEMP  				    1000 	// Fehlerwert, wenn die Temperatur nicht ermittelt werden kann

#define ERR_TEMP_TOLERANCE_MINUTES  15      // erst nach 15 Minuten ohne aktuelle Temperatur einen Fehler (rotes Wort GRAD) anzeigen

#define TEMPERATURE_AUS 		    0
#define TEMPERATURE_MINUTE 		    1
#define TEMPERATURE_5MINUTE 	    2
#define TEMPERATURE_DURATION 	    5000

#ifdef TEMPERATUR_CPP

#else

extern int mywc_g_temperature;
extern String mywc_g_temperature_real_location;

extern void testTemperatur();

extern int GetTemperature(String s);
extern void showTemperature(int t);

#endif

#endif
