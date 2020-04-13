#define MYWC12X12_H

#ifdef MyWC12x12_INO

#else
extern CRGB leds[NUM_LEDS];
extern config_t CONFIG;
extern String ip;

extern int g_hour;
extern int g_minute;
extern int g_heute_tag;
extern int g_heute_monat;
extern int g_heute_jahr;

extern int g_reboot_hour;
extern int g_reboot_minute;
extern int g_reboot_heute_tag;
extern int g_reboot_heute_monat;
extern int g_reboot_heute_jahr;

extern void setBrightness(int);
extern void resetLEDs();
extern void setNumber(int, CRGB);
extern void setWord(int, CRGB);

extern void testPower();
extern void testLocale();

extern void saveConfig();

extern void changeLocale();
extern void showTime(int, int);

extern String GetDatumZeitString();

#ifdef TEMPERATURE
extern void startLaufschrift(String, CRGB);
extern int stepLaufschrift();
extern void testLaufschrift();
#endif

extern void resetWiFi();
extern void resetConfig();
extern void resetAllAndReboot();

extern int jetztDunkelschaltung(int, int);

extern void restart();

#endif
