#define TEMPERATUR_H

#ifdef TEMPERATURE

#define ERR_TEMP  				    1000 	// Fehlerwert, wenn die Temperatur nicht ermittelt werden kann
#define ERR_TEMP_TOLERANCE_MINUTES  15      // erst nach 15 Minuten ohne aktuelle Temperatur einen Fehler (rotes Wort GRAD) anzeigen

#define TEMPERATURE_AUS 		    0
#define TEMPERATURE_MINUTE 		    1
#define TEMPERATURE_5MINUTE 	    2

#define TEMPERATURE_DURATION 	    5000    // Anzeigedauer in Millisekunden
#define TEMPERATURE_REFETCH_MINUTES 15      // Abstand in Minuten in dem die Temperatur neu geholt wird. Wg. Modulo-Operation idealerweise ein Teiler von 60
#define TEMPERATURE_REFETCH_SHIFT   0       // Anscheinend ist die Abfrage zu ganzen Viertelstunden problematisch. Also verschiebe ich die Abfrage um 1 Minuten

#ifdef TEMPERATUR_CPP

#else

extern int      mywc_g_temperature;
extern String   mywc_g_temperature_real_location;
extern String   mywc_g_debug_temperature;

extern void testTemperatur();

extern int GetTemperature(String s);
extern void showTemperature(int t);

#endif

#endif
