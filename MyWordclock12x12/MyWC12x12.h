#define MYWC12X12_H

#ifdef MyWC12x12_INO

#else
extern CRGB leds[NUM_LEDS];
extern config_t CONFIG;
extern String ip;

extern int hour;
extern int minute;
extern int temperature;

extern void SetBrightness(int);
extern void resetLEDs();
extern void setNumber(int, CRGB);
extern void setWord(int, CRGB);

extern void saveConfig();

extern void changeLocale();
extern void showTime(int, int);

extern void startLaufschrift(String, CRGB);
extern int stepLaufschrift();

extern void resetWiFi();
extern void resetConfig();
extern void resetAllAndReboot();

extern int jetztDunkelschaltung(int, int);

#endif