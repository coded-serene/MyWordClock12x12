//
// WORDCLOCK 12x12
//
// Matthias Roessler
//
// fuer FabLab Landkreis Fuerth e.V.
//
// TODO:
// - Ausweisen des wttr.in-Ortes der letzten Zeitermittlung auf der Webseite
// - Logging in Dateisystem
// - Verschönern der Web-Seite
// - Nach Speichern der neuen Parameter des Formulars, ist die Anzeige "aus dem Tritt"
// - Refactoring: Aufteilen in mehrere Dateien
//
// DONE:
// 20200309 Fehler bei der Stundenberechnung vor/nach (15min 20 min)
// 20200317 Verschoenern der Webseite: Optische Trenner eingefügt
//          Unterstützung für HERZ_DATUM vervollstaendigt
//          Farbskala anpassen
// 20200318 Fehler bei der Temperaturanzeige sechzehn statt sechszehn, analog siebzehn
//          Bei der Abfrage der Temperatur wird jetzt auch der tatsächtlich von wttr.in verwendete Ort bestimmt und auf der Webseite ausgegeben
//          Verschoenern der Webseite: "gefährliche" Buttons rot
//                                     Prozentzahl explizit im Label der %-Werte auf der Webseite
//
// Feature Toggles aktivieren mit define, deaktivieren mit undef
#define GEBURTSTAGE 1
#define LAUFSCHRIFT 1
#define TEMPERATURE 1
#define LOCALE 1
#define DUNKELSCHALTUNG 1
#define HERZ 1
#define FEATURE_OTA 1

//#undef GEBURTSTAGE
//#undef LAUFSCHRIFT
//#undef TEMPERATURE
//#undef LOCALE
//#undef DUNKELSCHALTUNG
//#undef HERZ
//#undef FEATURE_OTA

// unterdrücken der PRAGMA-Meldungen in FastLED
#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0	// gegen Glitching

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <FastLED.h>            // http://fastled.io      https://github.com/FastLED/FastLED
#include <DNSServer.h>
#include <NTPClient.h>          // The MIT License (MIT) Copyright (c) 2015 by Fabrice Weinberg
#include <FS.h>
#include <ArduinoJson.h>        // arduinojson.org
#include <time.h>

//
// Allgemeine Festlegungen
// Größe der Matrix, damit Azahl der LEDs (+4 wg MiutenLEDs) und Anschluss der LED-Kette an den D2
//
#define GRID_ROWS 12
#define GRID_COLS 12
#define NUM_LEDS (GRID_ROWS * GRID_COLS) + 4
#define DATA_PIN 4
// Je nach LED-Stripe kann RGB oder GRB (oder vllt. auch ein total anderer Typ auszuwählen sein
#define LEDCOLORORDER RGB



#ifdef FEATURE_OTA
#include <ArduinoOTA.h>         // OTA library
#include <ESP8266mDNS.h>		// für OTA
#endif

#ifdef LAUFSCHRIFT
// Einbinden nach FastLED.h
//
#include <LEDMatrix.h>          // LEDMatrix V4 class by Aaron Liddiment (c) 2015
#include <LEDText.h>            // LEDText V6 class by Aaron Liddiment (c) 2015
#include <FontMatrise.h>        // LEDText V6 class by Aaron Liddiment (c) 2015

#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX

cLEDMatrix < GRID_COLS, -GRID_ROWS, MATRIX_TYPE > mleds;
cLEDText ScrollingMsg;

#define LAUFSCHRIFT_SPEED	150
#endif

//
// Auswahl der Buchstabenmasken
//
#define MASKE_MR 1
#undef MASKE_FABLAB_FUERTH

#ifdef MASKE_FABLAB_FUERTH
#undef HERZ						// Kein Herz auf der Buchstabenmaske des Fablab Fuerth
#endif

#ifdef HERZ
#define HERZ_AUS 		0		// grundsätzlich ist das Herz aus (Hintergrundfarbe)
#define HERZ_AN 		1		// das Herz wird in Vordergrundfarbe angezeigt
#define HERZ_ROT 		2		// das Herz wird rot angezeigt
#define HERZ_DATUM 		3		// das Herz wird am <DATUM> in rot angezeigt
#define HERZ_STD_DATUM	4		// das Herz wird in Vordergrundfarbe angezaigt, aber am <DATUM> in rot
#endif

#ifdef LOCALE
// Sprachlokalisierungen, vorerst nur Deutsch und Fränkisch
#define L_DEUTSCHLAND 		0
#define L_FRANKEN 			1
#define L_W_DEUTSCHLAND		"Deutsch"
#define L_W_FRANKEN			"Fr&aumlnkisch"
#define L_S_DEUTSCHLAND     25
#define L_S_FRANKEN 		15
#endif


CRGB leds[NUM_LEDS];

typedef struct {
  CRGB color_bg;
  CRGB color_fg;
  int brightness;
  int timezone;
#ifdef DUNKELSCHALTUNG
  bool dunkelschaltung_active;
  int dunkelschaltung_start;
  int dunkelschaltung_end;
  int dunkelschaltung_brightness;
#endif
#ifdef TEMPERATURE
  int temp_active;
  CRGB temp_color;
  String city;
#endif
#ifdef LOCALE
  int locale;
#endif
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
#ifdef HERZ
  int herz;
  int dat_herz;
#endif
} config_t;

//
// Wortindices, sind unabhängig von der Buchstabenmaske, beschreiben die Position im Array der Worte
//
#define W_NOWORD			-1
#define W_UHR				0
#define W_FUENF_NACH		1
#define W_ZEHN_NACH			2
#define W_VIERTEL_NACH		3
#define W_ZWANZIG_NACH		4
#define W_FUENF_VOR_HALB	5
#define W_HALB				6
#define W_FUENF_NACH_HALB	7
#define W_ZWANZIG_VOR		8
#define W_VIERTEL_VOR		9
#define W_ZEHN_VOR			10
#define W_FUENF_VOR			11
#define W_ES_IST			12
#define W_ZWOELF			13
#define W_EINS				14
#define W_ZWEI				15
#define W_DREI				16
#define W_VIER				17
#define W_FUENF				18
#define W_SECHS				19
#define W_SIEBEN            20
#define W_ACHT              21
#define W_NEUN              22
#define W_ZEHN 				23
#define W_ELF 				24
#define W_EIN				25
#define W_WLAN				26
#define W_VIERTEL			27
#define W_ZEHN_VOR_HALB		28
#define W_ZEHN_NACH_HALB	29
#define W_DREIVIERTEL		30
#define W_UND				31
#define W_ZWANZIG			32
#define W_DREISSIG			33
#define W_PLUS				34
#define W_MINUS				35
#define W_GRAD				36
#define W_NULL				37
#define W_PUNKT				38
#define W_EIN_PUNKT			39
#define W_ZWEI_PUNKTE		40
#define W_DREI_PUNKTE		41
#define W_VIER_PUNKTE		42
#define W_ZEHNER			43
#define W_SIEB				44
#define W_SECH				45
#ifdef MASKE_MR
#define W_HERZ				46
#define W_ARRAYGROESSE		47
#else
#define W_ARRAYGROESSE		46
#endif

//
// Wortlisten
//
int wordsindex_single_m[] = { W_NOWORD, W_EIN_PUNKT, W_ZWEI_PUNKTE, W_DREI_PUNKTE, W_VIER_PUNKTE};
int wordsindex_minutes[] = { W_UHR, W_FUENF_NACH, W_ZEHN_NACH, W_VIERTEL_NACH, W_ZWANZIG_NACH, W_FUENF_VOR_HALB, W_HALB, W_FUENF_NACH_HALB, W_ZWANZIG_VOR, W_VIERTEL_VOR, W_ZEHN_VOR, W_FUENF_VOR };
int wordsindex_hours[] = {W_ZWOELF, W_EINS, W_ZWEI, W_DREI, W_VIER, W_FUENF, W_SECHS, W_SIEBEN, W_ACHT, W_NEUN, W_ZEHN, W_ELF};
int wordsindex_ziffern[] = {W_NULL, W_EIN, W_ZWEI, W_DREI, W_VIER, W_FUENF, W_SECHS, W_SIEBEN, W_ACHT, W_NEUN};

// Ab wann (Minutenindex) wird von den "nach"-Angaben auf "vor"-Angaben umgeschaltet unf damit die tatsächliche Stunde um eine erhöht.
// Kann in uterschiedliche Lokalisierungen unterschiedlich ausfallen
int words_umschaltung_schwellwert = L_S_DEUTSCHLAND;

//
// Abbildung der Buchstabenmasken
// Welche LED ist für welches Wort zu aktivieren?
//
// Ergibt sich aus dem Layout der Maske. Jedes Wort wird mit -1 terminiert. Da pro Wort 13 ints vorgesehen werden, kann in einer 12x12-Matrix eine ganze Zeile mit einem Wort angeschaltet werden.
//

#ifdef MASKE_FABLAB_FUERTH

int words[W_ARRAYGROESSE][13] = {								// reicht für eine Zeile plus -1 als terminierender Wert
  { 134, 133, 132,  -1}, 										// uhr
  { 8,   9,  10,  11, 18, 17, 16, 15, -1}, 						// fuenf nach
  { 23,  22,  21,  20,  18, 17, 16, 15, -1}, 					// zehn nach
  { 28,  29,  30,  31,  32, 33, 34,  18, 17, 16, 15, -1}, 		// viertel nach
  { 114, 113, 112, 111, 110, 109, 108,  18, 17, 16, 15, -1}, 	// zwanzig nach
  {  8,   9,  10,  11,  14,  13,  12,  47, 46, 45, 44, -1}, 	// fuenf vor halb
  { 47, 46, 45, 44, -1},										// halb
  {  8,   9,  10,  11, 18,  17,  16,  15,  47, 46, 45, 44, -1},	// fuenf nach halb
  {  114, 113, 112, 111, 110, 109, 108,  14, 13, 12, -1}, 		// zwanzig vor
  { 28,  29,  30,  31,  32, 33, 34,  14,  13,  12,  -1,  -1}, 	// viertel vor
  { 23, 22, 21, 20, 14,  13,  12,  -1},						 	// zehn vor
  { 8,   9,  10,  11, 14,  13,  12,  -1}, 						// fuenf vor
  { 0, 1, 3, 4, 5, -1},											// es ist
  { 143, 142, 141, 140, 139,  -1}, 								// zwoelf
  { 39,  38,  37,  36,  -1}, 									// eins
  { 70,  69,  68,  67,  -1}, 									// zwei
  { 72,  73,  74,  75,  -1}, 									// drei
  { 80,  81,  82,  83,  -1}, 									// vier
  { 76,  77,  78,  79,  -1}, 									// fuenf
  { 97,  98,  99, 100, 101,  -1}, 								// sechs
  { 89,  88,  87,  86,  85,  84, -1},							// sieben
  { 94,  93,  92,  91,  -1},									// acht
  { 55,  56,  57,  58,  -1},									// neun
  {104, 105, 106, 107,  -1},									// zehn
  {129, 130, 131,  -1},											// elf
  { 39,  38,  37,  -1}, 		 								// ein
  { 69, 62, 94, 84, -1 },										// WLAN
  { 28,  29,  30,  31,  32,  33,  34, -1}, 						// viertel
  { 23,  22,  21,  20,  12,  13,  14, 47, 46, 45, 44, -1},		// zehn vor halb
  { 23,  22,  21,  20,  18,  17,  16, 15, 47, 46, 45, 44, -1},	// zehn nach halb
  { 24, 25, 26,  27,  28,  29,  30,  31,  32, 33, 34, -1 },		// dreiviertel
  { 119, 118, 117, -1 },										// und
  { 114, 113, 112, 111, 110, 109, 108, -1 },					// zwanzig
  { 120, 121, 122, 123, 124, 125, 126, 127, -1 },				// dreissig
  { 43, 42, 41, 40, -1 },										// plus
  { 49, 50, 51, 52, 53, -1 },									// minus
  { 138, 137, 136, 135, -1 },									// grad
  {  61, 62, 63, 64, -1 },										// null
  { 103, -1 },													// punkt
  { 144, -1},													// ein punkt
  { 144, 145, -1},												// zwei punkte
  { 144, 145, 146, -1},											// drei punkte
  { 144, 145, 146, 147, -1},									// vier punkte
  { 104, 105, 106, 107, -1},									// zehn (aber unten)
  { 97,  98,  99, 100, -1},										// sieb (zehn)
  { 89,  88,  87,  86, -1}										// sech (zehn)
};
#else
int words[W_ARRAYGROESSE][13] = {                				 // reicht für eine Zeile plus -1 als terminierender Wert
  { 134,133,132,											-1}, // uhr
  { 7,8,9,10, 44,43,42,41, 									-1}, // fuenf nach
  { 23,22,21,20,  44,43,42,41, 								-1}, // zehn nach
  { 29,30,31,32,33,34,35, 44,43,42,41,			 			-1}, // viertel nach
  { 12, 13, 14, 15, 16, 17, 18,  44, 43, 42, 41, 			-1}, // zwanzig nach
  {  7, 8, 9, 10,  47, 46, 45,  36, 37, 38, 39, 			-1}, // fuenf vor halb
  { 36, 37, 38, 39, 										-1}, // halb
  {  7, 8, 9, 10, 44, 43, 42, 41,  36, 37, 38, 39, 			-1}, // fuenf nach halb
  { 12, 13, 14, 15, 16, 17, 18, 45, 46, 47, 				-1}, // zwanzig vor
  { 29, 30, 31, 32, 33, 34, 35,  45, 46, 47, 				-1}, // viertel vor
  { 23, 22, 21, 20, 45, 46, 47,  							-1}, // zehn vor
  { 7, 8, 9, 10, 45, 46, 47,  								-1}, // fuenf vor
  { 0, 1, 3, 4, 5, 											-1}, // es ist !
  { 127,128,129,130,131,  									-1}, // zwoelf
  { 95,94,93,92,  											-1}, // eins
  { 87,86,85,84,  											-1}, // zwei
  { 91,90,89,88,  											-1}, // drei
  { 71,70,69,68,  											-1}, // vier
  { 63,62,61,60,  											-1}, // fuenf
  { 72,73,74,75,76, 										-1}, // sechs
  { 76,77,78,79,80,81, 										-1}, // sieben
  { 67,66,65,64,  											-1}, // acht
  { 96,97,98,99,  											-1}, // neun
  {100, 101, 102, 103,  									-1}, // zehn
  {104, 105, 106,  											-1}, // elf
  { 93,94,95,  												-1}, // ein
  { 11,35, 59, 83, 											-1}, // WLAN
  { 29, 30, 31, 32, 33, 34, 35, 							-1}, // viertel
  { 23,22,21,20, 45,46,47, 36,37,38,39, 					-1}, // zehn vor halb
  { 23,22,21,20, 44,43,42,41, 36,37,38,39, 					-1}, // zehn nach halb
  { 25,26,27,28,29,30,31,32,33,34,35,						-1}, // dreiviertel
  { 119,118,117,											-1}, // und
  { 120,121,122,123,124,125,126, 							-1}, // zwanzig
  { 115,114,113,112,111,110,109,108, 						-1}, // dreissig
  { 48,49,50,51, 											-1}, // plus
  { 54,55,56,57,58, 										-1}, // minus
  { 138, 137, 136, 135, 									-1}, // grad
  { 143,142,141,140, 										-1}, // null
  { 144, 													-1}, // punkt
  { 144, 													-1}, // ein punkt
  { 144, 145, 												-1}, // zwei punkte
  { 144, 145, 146, 											-1}, // drei punkte
  { 144, 145, 146, 147, 									-1}, // vier punkte
  { 100,101,102,103, 										-1}, // zehn (aber unten)
  { 76,77,78,79, 											-1}, // sieb (zehn)
  { 72,73,74,75, 											-1}, // sech (zehn)
  { 2,														-1}  // Herz
};
#endif

#define CONFIGFILE "/wordclock_config.json"
#define MODE_TIME 0

#ifdef TEMPERATURE
#define MODE_TEMP 1
#define ERR_TEMP  1000 		 // Fehlerwert, wenn die Temperatur nicht ermittelt werden kann
int temperature = ERR_TEMP;  // temperatur
int temperatur_minute = -1;
unsigned long temperatur_millis = -1;
String temperature_real_location;	// von wttr.in tatsächlich genutzter Ort, zur Ausgabe auf der Webseite
#define TEMPERATURE_AUS 0
#define TEMPERATURE_MINUTE 1
#define TEMPERATURE_5MINUTE 2

#define TEMPERATURE_DURATION 5000
#endif

#ifdef GEBURTSTAGE
#define MODE_BIRTHDAY 2
int heute_tag = -1;
int heute_monat = -1;
int heute_jahr = -1;
int geburtstag_minute = -1;
unsigned long geburtstag_millis = -1;
int geburtstag_ende = 0;
//
// Für die Geburtstage soll die Farbe der aktivierten LEDs wie ein Regenbogen durchscrollen.
// In dieser Matrix wird vermerkt, welche LED aktiviert ist.
//
uint8_t mask[NUM_LEDS];
#endif

#ifdef LAUFSCHRIFT
char buffer[256];
#endif

int mymode = MODE_TIME;
int brightness = 20;
int timezone = 0;
int hour = -1;
int minute = -1;
String ip = "";

config_t CONFIG;

//
// Zeitermittlung
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 3600000);

//
// Webserver für die Konfiguration
ESP8266WebServer server(80);

//
// Wifi-Manager für die Netzverbindung
WiFiManager wifiManager;


// Setzen der Helligkeit in %
//
void SetBrightness(int b) {
	if ((b<0)||(b>100))
		return;
	
	FastLED.setBrightness((int)((float)b * 2.55));
}

//
// Funktionen zum WiFi-Betrieb
//
void resetWiFi() {
  wifiManager.resetSettings();
}
void resetConfig() {
  SPIFFS.remove(CONFIGFILE);
}
void resetAllAndReboot() {
  resetConfig();
  resetWiFi();
  ESP.reset();
  delay(5000);
}




void loadConfig() {

  // Falbackwerte
  CONFIG.color_bg = CRGB::Black;
  CONFIG.color_fg = CRGB::White;
  CONFIG.brightness = 50; // %

  CONFIG.timezone = 1;

#ifdef DUNKELSCHALTUNG
  // Fallback: Dunkelschaltung inaktiv
  CONFIG.dunkelschaltung_active = false;
  CONFIG.dunkelschaltung_brightness = 5; // %
  CONFIG.dunkelschaltung_start = -1;
  CONFIG.dunkelschaltung_end = -1;
#endif
#ifdef TEMPERATURE
  // Temperaturanzeige zu jeder Minute
  CONFIG.temp_active = TEMPERATURE_MINUTE;
  CONFIG.city = "Erlangen";
#endif
#ifdef LOCALE
  // allg. deutsche Sprache
  CONFIG.locale = L_DEUTSCHLAND;
#endif
#ifdef GEBURTSTAGE
  CONFIG.geb_1 = -1;
  CONFIG.geb_2 = -1;
  CONFIG.geb_3 = -1;
  CONFIG.geb_4 = -1;
  CONFIG.geb_5 = -1;
  CONFIG.geb_name_1 = "";
  CONFIG.geb_name_2 = "";
  CONFIG.geb_name_3 = "";
  CONFIG.geb_name_4 = "";
  CONFIG.geb_name_5 = "";
#endif
#ifdef HERZ
  CONFIG.herz = HERZ_ROT;
  CONFIG.dat_herz = -1;
#endif


  File file = SPIFFS.open(CONFIGFILE, "r");

  if (!file) {
    Serial.println("Failed to open config file.");
    saveConfig();
    return;
  }

  Serial.println("Load config.");

  StaticJsonDocument<1024> doc;
  deserializeJson(doc, file);

  Serial.println(doc.as<String>());

  CONFIG.color_bg.r = doc["color_bg_r"].as<int>();
  CONFIG.color_bg.g = doc["color_bg_g"].as<int>();
  CONFIG.color_bg.b = doc["color_bg_b"].as<int>();

  CONFIG.color_fg.r = doc["color_fg_r"].as<int>();
  CONFIG.color_fg.g = doc["color_fg_g"].as<int>();
  CONFIG.color_fg.b = doc["color_fg_b"].as<int>();

  CONFIG.brightness = doc["brightness"].as<int>();
  brightness = CONFIG.brightness;
  
  SetBrightness(brightness);
  
  CONFIG.timezone = doc["timezone"].as<int>();
  timeClient.setTimeOffset(CONFIG.timezone * 3600);

#ifdef DUNKELSCHALTUNG
  CONFIG.dunkelschaltung_active = doc["dunkelschaltung_active"].as<bool>();
  CONFIG.dunkelschaltung_start = doc["dunkelschaltung_start"].as<int>();
  CONFIG.dunkelschaltung_end = doc["dunkelschaltung_end"].as<int>();
  CONFIG.dunkelschaltung_brightness = doc["dunkelschaltung_brightness"].as<int>();
#endif
#ifdef TEMPERATURE
  CONFIG.temp_active = doc["temp_active"].as<int>();
  CONFIG.city = doc["city"].as<char *>();
#endif
#ifdef LOCALE
  CONFIG.locale = doc["locale"].as<int>();
  // richtiges Sprachmodell auswaehlen
  changeLocale();
#endif
#ifdef GEBURTSTAGE
  CONFIG.geb_1 = doc["geb_1"].as<int>();
  CONFIG.geb_2 = doc["geb_2"].as<int>();
  CONFIG.geb_3 = doc["geb_3"].as<int>();
  CONFIG.geb_4 = doc["geb_4"].as<int>();
  CONFIG.geb_5 = doc["geb_5"].as<int>();
  CONFIG.geb_name_1 = doc["geb_name_1"].as<char *>();
  CONFIG.geb_name_2 = doc["geb_name_2"].as<char *>();
  CONFIG.geb_name_3 = doc["geb_name_3"].as<char *>();
  CONFIG.geb_name_4 = doc["geb_name_4"].as<char *>();
  CONFIG.geb_name_5 = doc["geb_name_5"].as<char *>();
#endif
#ifdef HERZ
  CONFIG.herz = doc["herz"].as<int>();
  CONFIG.dat_herz = doc["dat_herz"].as<int>();
#endif

  file.close();
}

void saveConfig() {
  File file = SPIFFS.open(CONFIGFILE, "w");

  if (!file) {
    Serial.println("Can't open configfile for writing");
	Serial.println(CONFIGFILE);
    return;
  }

  Serial.println("Save config.");

  StaticJsonDocument<1024> doc;
  
  doc["color_bg_r"] = CONFIG.color_bg.r;
  doc["color_bg_g"] = CONFIG.color_bg.g;
  doc["color_bg_b"] = CONFIG.color_bg.b;
  doc["color_fg_r"] = CONFIG.color_fg.r;
  doc["color_fg_g"] = CONFIG.color_fg.g;
  doc["color_fg_b"] = CONFIG.color_fg.b;
  doc["brightness"] = CONFIG.brightness;
  doc["timezone"] = CONFIG.timezone;
#ifdef DUNKELSCHALTUNG
  doc["dunkelschaltung_active"] = CONFIG.dunkelschaltung_active;
  doc["dunkelschaltung_start"] = CONFIG.dunkelschaltung_start;
  doc["dunkelschaltung_end"] = CONFIG.dunkelschaltung_end;
  doc["dunkelschaltung_brightness"] = CONFIG.dunkelschaltung_brightness;
#endif
#ifdef TEMPERATURE
  doc["city"] = CONFIG.city;
  doc["temp_active"] = CONFIG.temp_active;
#endif
#ifdef LOCALE
  doc["locale"] = CONFIG.locale;
#endif
#ifdef GEBURTSTAGE
  doc["geb_1"] = CONFIG.geb_1;
  doc["geb_2"] = CONFIG.geb_2;
  doc["geb_3"] = CONFIG.geb_3;
  doc["geb_4"] = CONFIG.geb_4;
  doc["geb_5"] = CONFIG.geb_5;
  doc["geb_name_1"] = CONFIG.geb_name_1;
  doc["geb_name_2"] = CONFIG.geb_name_2;
  doc["geb_name_3"] = CONFIG.geb_name_3;
  doc["geb_name_4"] = CONFIG.geb_name_4;
  doc["geb_name_5"] = CONFIG.geb_name_5;
#endif
#ifdef HERZ
  doc["herz"] = CONFIG.herz;
  doc["dat_herz"] = CONFIG.dat_herz;
#endif

  serializeJson(doc, file);

  Serial.println(doc.as<String>());

  file.close();
}

CRGB hexToRgb(String value) {
  value.replace("#", "");

  CRGB rgb = (uint32_t) strtol( value.c_str(), NULL, 16);

  return rgb;
}

String rgbToHex(const CRGB hex) {
  long hexColor = ((long)hex.r << 16L) | ((long)hex.g << 8L) | (long)hex.b;

  String out = String(hexColor, HEX);

  while (out.length() < 6) {
    out = "0" + out;
  }

  return out;
}

#ifdef TEMPERATURE
// Farbabstufung der Temperaturanzeige
//
CRGB GetTemperatureColor(int t) {
	// -39 bis -5	blau
	// -4  bis 0	hellblau
	// 1   bis 5    gruen
	// 6   bis 15   hellgruen
	// 16  bis 20   gelb
	// 21  bis 27   orange
	// 28  bis 39   rot
	if (t<=-5) {
		return CRGB::Blue;
	}
	else if (t<=0) {
		return CRGB::LightSkyBlue;
	}
	else if (t<=5) {
		return CRGB::SeaGreen;
	}
	else if (t<=15) {
		return CRGB::Green;
	}
	else if (t<=20) {
		return CRGB::Yellow;
	}
	else if (t<=27) {
		return CRGB::Orange;
	}
	else {
		return CRGB::Red;
	}
	return CRGB::Red;
}


// WETTER, Temperatur holen
String GetTemperatureRealLocation() {
  int httpCode;	
  HTTPClient http;            //Declare object of class HTTPClient
  String weatherstring;
  String payload;
  String location;
  int location_start;
  int location_end;
  
  if (WiFi.status() == WL_CONNECTED) {                    						//Check WiFi connection status
    weatherstring = "http://wttr.in/" + CONFIG.city + "?1";    //Specify request destination

	Serial.println(weatherstring);
	
    http.begin(weatherstring);
    httpCode = http.GET();

    if (httpCode == 200) {
	  int l = http.getSize();
	  
      payload = http.getString();                  						//Get the response payload
	  	  
      location_start = payload.indexOf("Location: ");
	  if (location_start >= 0) {
		location_end = payload.indexOf("[",location_start);
			
		if (location_end>=0) {
		  location = payload.substring(location_start+10, location_end -1);
		} else {
		  location = String("Fehler bei der Ortsbestimmung: Parsen - Unbekannter Ort<br>" + weatherstring + "<br>" + payload);
		}
	  } else {
		location = String("Fehler bei der Ortsbestimmung: Unbekannter Ort<br>")  + weatherstring + String("<br>len(payload)=") + String(payload.length()) + String(" payload=") + payload + String("<br>ret=") + httpCode + String("<br>len=") + String(l);
	  }
			
	} else {
	  Serial.print  ("ERROR getting TEMPERATURE: httpCode=");   Serial.println(httpCode);
	  location = String("Fehler bei der Ortsbestimmung: Anfragefehler " + httpCode);
    }

    http.end();
  }
  
  Serial.println(location);
  
  return location;
}

int GetTemperature() {
  int httpCode;	
  int temperature = ERR_TEMP; // is a kind of error code!
  HTTPClient http;            //Declare object of class HTTPClient
  String weatherstring;
  String payload;
  String temp_temperature;
  
  temperature_real_location = GetTemperatureRealLocation();
	
  if (WiFi.status() == WL_CONNECTED) {                    						//Check WiFi connection status
    weatherstring = "http://wttr.in/" + CONFIG.city + "?format=\%t";    //Specify request destination

	Serial.println(weatherstring);
	
    http.begin(weatherstring);
    httpCode = http.GET();

    if (httpCode >= HTTP_CODE_OK) {
      payload = http.getString();                  						//Get the response payload

      temp_temperature = payload.substring(0, payload.length() - 4); 	//Format is "+x°C\n" wobei ° zwei Bytes einnimmt!

      if ((temp_temperature.charAt(0) == '-') || (temp_temperature.charAt(0) == '+')) { 
		temperature = temp_temperature.toInt();
      }

	  http.end();
    } else {
      Serial.print  ("ERROR getting TEMPERATURE: httpCode=");   Serial.println(httpCode);
	  temperature_real_location = String("Fehler bei der Temperaturabfrage: Anfragefehler " + httpCode);

      http.end();                                           						//Close connection
    }

  }
  
  Serial.println(temperature);
  
  //temperature_real_location = GetTemperatureRealLocation();
  
  return temperature;
}

#endif

//
// Ermittlung, ob die aktuelle Uhrzeit im Intervall zwischen dunkelschaltung_start und dunkelschaltung_end liegt
// und die Dunkelschaltung aktiviert wurde
//
bool jetztDunkelschaltung(int hour, int minute) {
  int jetzt;

  if (!CONFIG.dunkelschaltung_active || CONFIG.dunkelschaltung_start == -1 || CONFIG.dunkelschaltung_end == -1) {
    return false;
  }

  jetzt = 100 * hour + minute;

  if (CONFIG.dunkelschaltung_start <= CONFIG.dunkelschaltung_end ) {
    if (jetzt >= CONFIG.dunkelschaltung_start && jetzt <= CONFIG.dunkelschaltung_end) {
      return true;
    }
  }
  else {
    if (jetzt >= CONFIG.dunkelschaltung_start || jetzt <= CONFIG.dunkelschaltung_end) {
      return true;
    }
  }

  return false;
}

// Funktionen zum Arbeiten mit dem LED-Streifen
//
// set-Funktionen setzen nur die Informationen im Array für die LEDs
// show-Funktionen rufen auch ein FastLED.show() auf und setzen am Beginn der Funktion die LEDs zurück
//
// alle LEDs auf Hintergrundfarbe setzen
//
void resetLEDs() {
  int i;

  for (i = 0; i < NUM_LEDS; i++) {
    leds[i] = CONFIG.color_bg;
#ifdef GEBURTSTAGE
    mask[i] = false;
#endif
  }
}

//
// Ein einzelnes Wort aus der Worttabelle setzen
//
void setWord(int word, CRGB col = CRGB::White) {
  int i = 0;

  Serial.printf("setWord(%d)", word); Serial.println();

  if (word < 0 || word >= W_ARRAYGROESSE) return;

  while (words[word][i] >= 0) {
    leds[words[word][i]] = col;
    //		Serial.printf("setWord(%d) LED(%d)",word,words[word][i]); Serial.println();
#ifdef GEBURTSTAGE
    mask[words[word][i]] = true;
#endif
    i++;
  };
}

//
// Ein einzelnes Wort aus der Worttabelle setzen und anzeigen lassen
//
void showWord(int w, CRGB c = CRGB::White) {
  resetLEDs();
  setWord(w, c);
  FastLED.show();
}


//
// Eine Zahl anzeigen
// Temperaturwert von -39 bis +39 möglich
//
void setNumber(int n, CRGB c = CRGB::White) {
  int einer;

  if (n < -39 || n > 39) return;

  if (n > 0) setWord(W_PLUS, c);
  if (n < 0) {
    setWord(W_MINUS, c);
    n = -n;
  }

  if (n == 0) {
    setWord(W_NULL, c);
  }
  else if (n == 11) {
    setWord(W_ELF, c);
  }
  else if (n == 12) {
    setWord(W_ZWOELF, c);
  }
  else if (n == 16) {
    setWord(W_SECH, c);
    setWord(W_ZEHNER, c);
  }
  else if (n == 17) {
    setWord(W_SIEB, c);
    setWord(W_ZEHNER, c);
  }
  else {
    einer = n % 10;

    if (einer > 0) {
      setWord(wordsindex_ziffern[einer], c);
    }

    if (n > 9 && n < 20) {
      setWord(W_ZEHNER, c);
    }
    else if (n > 19 && n < 30) {
      setWord(W_UND, c);
      setWord(W_ZWANZIG, c);
    }
    else if (n > 29) {
      setWord(W_UND, c);
      setWord(W_DREISSIG, c);
    }
  }

}

#ifdef GEBURTSTAGE
bool isGeburtstagheut() {
  int heute;

  heute = heute_tag * 100 + heute_monat;

  if ( heute == CONFIG.geb_1 || heute == CONFIG.geb_2 || heute == CONFIG.geb_3 || heute == CONFIG.geb_4 || heute == CONFIG.geb_5 )
    return true;
  else
    return false;

  return false;
}

String getGebName() {
  int heute;

  heute = heute_tag * 100 + heute_monat;

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

void showWLAN(CRGB c) {
  resetLEDs();
  setWord(W_WLAN, c);
  FastLED.show();
}

#ifdef TEMPERATURE
void showTemperature(int t, CRGB c) {

  if (t < -39 || t > 39) return;

  resetLEDs();

  setWord(W_ES_IST, c);
  setNumber(t, c);
  setWord(W_GRAD, c);

  FastLED.show();
}
#endif

#ifdef LAUFSCHRIFT
void startLaufschrift(String text, CRGB c = CRGB::White) {
  text = "   " + text;      // smooth in

  text.toCharArray(buffer, 256);

  resetLEDs();

  ScrollingMsg.SetText((unsigned char *)buffer, strlen(buffer));
  ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, c.r, c.g, c.b);
}

int stepLaufschrift() {
	int i;
	
	i=ScrollingMsg.UpdateText();
	
	if (i!= -1) {
		FastLED.show();
	}
	
	return i;
}

void showLaufschrift(String text, CRGB c = CRGB::White) {
  char buffer[256];

  Serial.println("Laufschrift " + text);
  
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
}
#endif

#ifdef HERZ
void setHerz() {
  int heute;

  switch (CONFIG.herz) {
  case HERZ_AUS:
		break;
  case HERZ_AN:
		setWord(W_HERZ, CONFIG.color_fg);
		break;
  case HERZ_ROT:
		setWord(W_HERZ, CRGB::Red);
		break;
  case HERZ_DATUM:
		heute = heute_tag * 100 + heute_monat;

		if (heute == CONFIG.dat_herz) {
			setWord(W_HERZ, CRGB::Red);
		}
		break;
  case HERZ_STD_DATUM:
		heute = heute_tag * 100 + heute_monat;

		if (heute == CONFIG.dat_herz) {
			setWord(W_HERZ, CRGB::Red);
		} else {
			setWord(W_HERZ, CONFIG.color_fg);
		}
		break;
  }
}
#endif
  

void showTime(int hour, int minute) {
  int singleMinute;
  if (hour == -1 || minute == -1) return;

  singleMinute = minute % 5;
  minute = (minute - (minute % 5));

  if (minute >= words_umschaltung_schwellwert) hour += 1;

  minute = minute / 5;
  hour = hour % 12;

  resetLEDs();

#ifdef HERZ
  setHerz();
#endif

  setWord(W_ES_IST, CONFIG.color_fg);

  // Minuten
  setWord(wordsindex_minutes[minute], CONFIG.color_fg);

  // Stunden
  if (hour == 1 && minute == 0) {
    // Sonderfall Ein Uhr statt EINS Uhr
    setWord(W_EIN, CONFIG.color_fg);
  }
  else {
    setWord(wordsindex_hours[hour], CONFIG.color_fg);
  }

  // Minutenpunkte
  setWord(wordsindex_single_m[singleMinute], CONFIG.color_fg);

  FastLED.show();
}


//
// Funktionen rund um die Konfigurationsseite des eigenen Webservers
//
String pad(int value) {
  if (value < 10) {
    return "0" + String(value);
  }

  return String(value);
}


// Hauptformular
//
String getTimeForm() {
  String content = "";
  String label = "";

  // Anzeigefarbe
  content += "<div>";
  content += "<label>Vordergrundfarbe</label>";
  content += "<input name=\"fg\" value=\"#" + rgbToHex(CONFIG.color_fg) + "\" type=\"color\">";
  content += "</div>";

  // Hintergrundfarbe
  content += "<div>";
  content += "<label>Hintergrundfarbe</label>";
  content += "<input name=\"bg\" value=\"#" + rgbToHex(CONFIG.color_bg) + "\" type=\"color\">";
  content += "</div>";

  // Helligkeit
  content += "<div>";
  content += "<label>Helligkeit (" + String(CONFIG.brightness) + "%)</label>";
  content += "<input type=\"range\" name=\"brightness\" min=\"0\" max=\"100\" value=\"" + String(CONFIG.brightness) + "\">";
  content += "</div>";

  content += "<hr>";
  
  // Zeitzone
  content += "<div>";
  content += "<label>Zeitzone</label>";
  content += "<select name=\"tz\">";

  for (int i = -12; i < 13; i++) {
    label = String(i);

    if (i > 0) {
      label = "+" + label;
    }

    content += htmlOption(label, String(i), String(CONFIG.timezone));
  }
  content += "</select>";
  content += "</div>";


#ifdef LOCALE
  content += "<hr>";
  
  content += "<div>";
  content += "<label>Region</label>";
  content += "<select name=\"locale\">";
  content += htmlOption(L_W_DEUTSCHLAND, String(L_DEUTSCHLAND), String(CONFIG.locale));
  content += htmlOption(L_W_FRANKEN, String(L_FRANKEN), String(CONFIG.locale));
  content += "</select>";
  content += "</div>";
#endif
#ifdef HERZ
  content += "<hr>";
  
  content += "<div>";
  content += "<label>Herz</label>";
  content += "<select class=\"time\" name=\"herz\" onchange=\"herzchanged\">";
  content += htmlOption("Aus", String(HERZ_AUS), String(CONFIG.herz));
  content += htmlOption("Standardfarbe", String(HERZ_AN), String(CONFIG.herz));
  content += htmlOption("Rot", String(HERZ_ROT), String(CONFIG.herz));
  content += htmlOption("Aus + Rot am Datum", String(HERZ_DATUM), String(CONFIG.herz));
  content += htmlOption("Standardfarbe + Rot am Datum", String(HERZ_STD_DATUM), String(CONFIG.herz));
  content += "</select>";
  content += "<label>Datum (TTMM)</label>";
  content += "<input name=\"dat_herz\" value=\"" + ((CONFIG.dat_herz>0)?String(CONFIG.dat_herz):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";
#endif

#ifdef DUNKELSCHALTUNG
  content += "<hr label=\"Dunkelschaltung\">";
  content += "<div>";
  content += "<label>Nachtmodus</label>";
  content += "<input type=\"checkbox\" name=\"dunkelschaltung_active\" value=\"1\"";
  if (CONFIG.dunkelschaltung_active) {
    content += "checked";
  };
  content += ">";
  content += "</div>";

  // abgesenkte Helligkeit
  content += "<div>";
  content += "<label>reduzierte Helligkeit (" + String(CONFIG.dunkelschaltung_brightness) + "%)</label>";
  content += "<input type=\"range\" name=\"dunkelschaltung_brightness\" min=\"0\" max=\"50\" value=\"" + String(CONFIG.dunkelschaltung_brightness) + "\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Nachtmodus Startzeit</label>";
  content += "<select class=\"time\" name=\"dunkelschaltung_start\">";

  for (int i = 0; i < 24; i++) {
    content += htmlOption(pad(i) + ":00", String(i * 100), String(CONFIG.dunkelschaltung_start));
    content += htmlOption(pad(i) + ":30", String(i * 100 + 30), String(CONFIG.dunkelschaltung_start));
  }

  content += "</select>";
  content += "</div>";

  content += "<div>";
  content += "<label>Nachtmodus Endzeit</label>";
  content += "<select class=\"time\" name=\"dunkelschaltung_end\">";

  for (int i = 0; i < 24; i++) {
    content += htmlOption(pad(i) + ":00", String(i * 100), String(CONFIG.dunkelschaltung_end));
    content += htmlOption(pad(i) + ":30", String(i * 100 + 30), String(CONFIG.dunkelschaltung_end));
  }

  content += "</select>";
  content += "</div>";
#endif
#ifdef TEMPERATURE

  content += "<hr>";
  
  content += "<div>";
  content += "<label>Temperaturanzeige</label>";
  
  content += "<select class=\"time\" name=\"temp_active\">";
  content += htmlOption("Aus", String(0), String(CONFIG.temp_active));
  content += htmlOption("Jede Minute", String(1), String(CONFIG.temp_active));
  content += htmlOption("Jede 5. Minute", String(2), String(CONFIG.temp_active));
  content += "</select>";
  content += "</div>";

  content += "<div>";
  content += "<label>Ort</label>";
  content += "<input name=\"city\" value=\"" + String(CONFIG.city) + "\">";
  content += "<p>" + temperature_real_location + "</p>";
  content += "</div>";
#endif

#ifdef GEBURTSTAGE
  content += "<hr>";
  
  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_1\" value=\"" + String(CONFIG.geb_name_1) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_1\" value=\"" + ((CONFIG.geb_1>0)?String(CONFIG.geb_1):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_2\" value=\"" + String(CONFIG.geb_name_2) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_2\" value=\"" + ((CONFIG.geb_2>0)?String(CONFIG.geb_2):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_3\" value=\"" + String(CONFIG.geb_name_3) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_3\" value=\"" + ((CONFIG.geb_3>0)?String(CONFIG.geb_3):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_4\" value=\"" + String(CONFIG.geb_name_4) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_4\" value=\"" + ((CONFIG.geb_4>0)?String(CONFIG.geb_4):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_5\" value=\"" + String(CONFIG.geb_name_5) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_5\" value=\"" + ((CONFIG.geb_5>0)?String(CONFIG.geb_5):String(" ")) + "\" maxlength=\"4\">";
  content += "</div>";
#endif

  return content;
}
String htmlOption(String label, String value, String store) {
  String content = "<option value=\"" + value + "\"";

  if (store == value) content += " selected=\"selected\"";

  content += ">" + label + "</option>";

  return content;
}

#ifdef LOCALE
void changeLocale() {

  Serial.println("Locale=" + CONFIG.locale);

  if (CONFIG.locale == L_FRANKEN) {
    Serial.println(L_W_FRANKEN);
    words_umschaltung_schwellwert = L_S_FRANKEN;
    wordsindex_minutes[3] = W_VIERTEL;
    wordsindex_minutes[4] = W_ZEHN_VOR_HALB;
    wordsindex_minutes[8] = W_ZEHN_NACH_HALB;
    wordsindex_minutes[9] = W_DREIVIERTEL;
  }
  else {
    words_umschaltung_schwellwert = L_S_DEUTSCHLAND;
    Serial.println(L_W_DEUTSCHLAND);
    wordsindex_minutes[3] = W_VIERTEL_NACH;
    wordsindex_minutes[4] = W_ZWANZIG_NACH;
    wordsindex_minutes[8] = W_ZWANZIG_VOR;
    wordsindex_minutes[9] = W_VIERTEL_VOR;
  }
}
#endif


void change() {

  for (int i = 0; i < server.args(); i++)
  {
    Serial.println(server.argName(i) + " = " + server.arg(i));
  }

  if (server.hasArg("submit")) {

    if (server.arg("submit") == "testPower") {
		resetLEDs();
		SetBrightness(100);
		for (int i=0; i<NUM_LEDS; i++) {
		  leds[i] = CRGB::White;
		
		  FastLED.show();
		  
		  delay(1000);
		}
		brightness=CONFIG.brightness;
	}

#ifdef LOCALE
    else if (server.arg("submit") == "testLocale") {
      for (int i = 0; i < 60; i++) {
        showTime(12, i);
        delay(250);
      }
    }
#endif

#ifdef TEMPERATURE
    else if (server.arg("submit") == "testTemp") {
		for (int i=-39; i<40; i++) {
			showTemperature(i, GetTemperatureColor(i));
			delay(250);
		}
	}
#endif

#ifdef LAUFSCHRIFT
    else if (server.arg("submit") == "testLaufschrift") {
		startLaufschrift("DIES IST EIN TEST.", CRGB::White);
		
		while (stepLaufschrift() != -1) {
			delay(LAUFSCHRIFT_SPEED);
		}
		
	}
#endif
    else if (server.arg("submit") == "ResetConfig") {
      resetConfig();
    }
    else if (server.arg("submit") == "ResetWiFi") {
      resetWiFi();
    }
    else if (server.arg("submit") == "ResetAll") {
      resetAllAndReboot();
    }
    else if (server.arg("submit") == "save") {

      if (server.hasArg("fg")) CONFIG.color_fg = hexToRgb(server.arg("fg"));
      if (server.hasArg("bg")) CONFIG.color_bg = hexToRgb(server.arg("bg"));
      if (server.hasArg("brightness")) CONFIG.brightness = server.arg("brightness").toInt();
      if (server.hasArg("tz")) CONFIG.timezone = server.arg("tz").toInt();

#ifdef DUNKELSCHALTUNG
      CONFIG.dunkelschaltung_active = server.hasArg("dunkelschaltung_active");
      if (server.hasArg("dunkelschaltung_start")) CONFIG.dunkelschaltung_start = server.arg("dunkelschaltung_start").toInt();
      if (server.hasArg("dunkelschaltung_end")) CONFIG.dunkelschaltung_end = server.arg("dunkelschaltung_end").toInt();
      if (server.hasArg("dunkelschaltung_brightness")) CONFIG.dunkelschaltung_brightness = server.arg("dunkelschaltung_brightness").toInt();
#endif

#ifdef TEMPERATURE
      if (server.hasArg("temp_active")) CONFIG.temp_active = server.hasArg("temp_active");
      if (server.hasArg("city"))        CONFIG.city        = server.arg("city");
#endif

#ifdef LOCALE
      if (server.hasArg("locale")) CONFIG.locale = server.arg("locale").toInt();
#endif

#ifdef GEBURTSTAGE
	  String s;
	  
      if (server.hasArg("geb_1")) CONFIG.geb_1 = server.arg("geb_1").toInt();
      if (server.hasArg("geb_2")) CONFIG.geb_2 = server.arg("geb_2").toInt();
      if (server.hasArg("geb_3")) CONFIG.geb_3 = server.arg("geb_3").toInt();
      if (server.hasArg("geb_4")) CONFIG.geb_4 = server.arg("geb_4").toInt();
      if (server.hasArg("geb_5")) CONFIG.geb_5 = server.arg("geb_5").toInt();
      if (server.hasArg("geb_name_1")) { s = server.arg("geb_name_1"); s.toUpperCase();	CONFIG.geb_name_1 =  s; }
      if (server.hasArg("geb_name_2")) { s = server.arg("geb_name_2"); s.toUpperCase();	CONFIG.geb_name_2 =  s; }
      if (server.hasArg("geb_name_3")) { s = server.arg("geb_name_3"); s.toUpperCase();	CONFIG.geb_name_3 =  s; }
      if (server.hasArg("geb_name_4")) { s = server.arg("geb_name_4"); s.toUpperCase();	CONFIG.geb_name_4 =  s; }
      if (server.hasArg("geb_name_5")) { s = server.arg("geb_name_5"); s.toUpperCase();	CONFIG.geb_name_5 =  s; }
#endif
#ifdef HERZ
	if (server.hasArg("herz")) CONFIG.herz = server.arg("herz").toInt();
	if (server.hasArg("dat_herz")) CONFIG.dat_herz = server.arg("dat_herz").toInt();
#endif

#ifdef DUNKELSCHALTUNG
	  if (jetztDunkelschaltung(hour, minute)) {
		SetBrightness(CONFIG.dunkelschaltung_brightness);
		Serial.print  ("Dunkelschaltung jetzt!"); Serial.println(brightness);
	  } else {
	    SetBrightness(CONFIG.brightness);
      }
#else
	SetBrightness(CONFIG.brightness);
#endif


      saveConfig();
      hour = -1;        // Zeitanzeige in loop() erzwingen

      temperature = GetTemperature();
      changeLocale();


    }
  }
}

void handleRootPath() {
  String content = "";

  change();

  content += "<!DOCTYPE html><html>";
  content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  content += "<style>";
  content += "* { box-sizing: border-box; }";
  content += "html, body { font-family: Helvetica; margin: 0; padding: 0; }";
  content += ".form { margin: auto; max-width: 400px; }";
  content += ".form div { margin: 0; padding: 20px 0; width: 100%; font-size: 1.4rem; }";
  content += "label { width: 60%; display: inline-block; margin: 0; vertical-align: middle; }";
  content += "input, select { width: 38%; display: inline-block; margin: 0; border: 1px solid #eee; padding: 0; height: 40px; vertical-align: right; }";
  content += "select.time { width: 18%; }";
  content += "button { display: inline-block; width: 100%; font-size: 1.4rem; background-color: green; border: 1px solid #eee; color: #fff; padding-top: 10px; padding-bottom: 10px; }";
  content += "button.danger { display: inline-block; width: 100%; font-size: 1.4rem; background-color: red; border: 1px solid #eee; color: #fff; padding-top: 10px; padding-bottom: 10px; }";
  content += "</style>";
  content += "</head>";
  content += "<body>";



  content += "<h1 align=center>WordClock Konfiguration</h1>";
  content += "<form class=\"form\" method=\"post\" action=\"\">";
  
  content += "<div>";
  content += getTimeForm();
  content += "</div>";

  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"save\">Speichern</button>";
  content += "</div>";
  
  content += "<hr label=\"Tests\">";
  
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"testLocale\">Test Region</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"testTemp\">Test Temperatur</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"testLaufschrift\">Test Laufschrift</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"testPower\">Test Stromverbrauch</button>";
  content += "</div>";
  
  content += "<hr label=\"Zurücksetzen\">";
  
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetConfig\" background-color=\"red\";>Konfiguration zur&uuml;cksetzen</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetWiFi\" background-color: red;>WLAN-Parameter zur&uuml;cksetzen</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetAll\" background-color: red;>Komplett zur&uuml;cksetzen und Reboot</button>";
  content += "</div>";

  content += "</form>";
  content += "</body></html>";

  server.sendHeader("Location", "http://" + ip);
  server.send(200, "text/html", content);

}


void showIP(String sIP) {

  if (sIP == NULL) return;

#ifdef LAUFSCHRIFT
  showLaufschrift("WLAN: " + sIP);
#else
  for (unsigned int i = 0; i < sIP.length(); ++i) {
    resetLEDs();
    switch ((char)sIP[i]) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        setWord(wordsindex_hours[(int)((char)sIP[i] - '0')], CRGB::Blue);
        break;
      case '.':
        setWord(W_PUNKT, CRGB::Blue);
        break;
      default:
        break;
    }
    FastLED.show();
    delay(1500);
  }
#endif
}


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  showWLAN(CRGB::Yellow);

  Serial.println("WiFiManager Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster

}

#ifdef GEBURTSTAGE
bool getLocalTime(struct tm * info, uint32_t ms) {
  uint32_t count = ms / 10;
  time_t now;

  time(&now);
  localtime_r(&now, info);

  if (info->tm_year > (2016 - 1900)) {
    return true;
  }

  while (count--) {
    delay(10);
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > (2016 - 1900)) {
      return true;
    }
  }
  return false;
}

void GetDatum(int *tag, int *monat, int *jahr) {

  struct tm tmstruct;

  // Datum herausfinden
  configTime(3600 * CONFIG.timezone, 1 * 3600, "0.pool.ntp.org", "time.nist.gov", "1.pool.ntp.org");

  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
  //

  *tag = tmstruct.tm_mday;
  *monat = (tmstruct.tm_mon) + 1;
  *jahr = (tmstruct.tm_year) + 1900;
}
#endif

void setup() {
  IPAddress ipL;

  Serial.begin(74880);


#ifdef GEBURTSTAGE
  Serial.println("Feature Geburtstage enabled!");
#endif
#ifdef LAUFSCHRIFT
  Serial.println("Feature Laufschrift enabled!");
#endif
#ifdef LOCALE
  Serial.println("Feature Lokalisierung enabled!");
#endif
#ifdef TEMPERATURE
  Serial.println("Feature Temperaturanzeige enabled!");
#endif
#ifdef FEATURE_OTA
  Serial.println("Feature OTA enabled!");
#endif
#ifdef DUNKELSCHALTUNG
  Serial.println("Feature Dunkelschaltung enabled!");
#endif
#ifdef HERZ
  Serial.println("Feature Herz enabled!");
#endif


  // Dateisystem initialisieren
  SPIFFS.begin();

  // Konfiguration laden
  loadConfig();

  // LEDstreifen anhängen
  FastLED.addLeds<WS2812B, DATA_PIN, LEDCOLORORDER>(leds, NUM_LEDS);



#ifdef LAUFSCHRIFT
  mleds.SetLEDArray(&leds[0]);

  ScrollingMsg.SetFont(MatriseFontData);
  ScrollingMsg.Init(&mleds, mleds.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
#endif

  // initiale Helligkeit setzen
  SetBrightness(brightness);
  
  
  
  
  
  // WLAN-Konfiguration
  //
  showWord(W_WLAN, CRGB::Red);
  WiFi.hostname("WordClock");

  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("WordClock")) {
    Serial.println("WiFiManager; failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  showWord(W_WLAN, CRGB::Green);
  delay(1000);

  ipL = WiFi.localIP();
  ip = ipL.toString();
  showIP(ip);
  // WLAN passt




#ifdef FEATURE_OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");
  ArduinoOTA.setHostname("WordClock");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("OTA start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA upload progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();             //OTA initialization
  Serial.println("OTA ready");
#endif

  // start webserver
  server.on("/", handleRootPath);
  server.begin();

  // start time-service
  timeClient.begin();
  timeClient.update();

#ifdef GEBURTSTAGE
  GetDatum(&heute_tag, &heute_monat, &heute_jahr);
#endif
}

void setMode(int i) {
	Serial.println(String("setMode alt=") + String(mymode) + String(" neu=") + String(i));
	mymode = i;
}

void loop() {

  unsigned long jetzt;

#ifdef FEATURE_OTA
  ArduinoOTA.handle();	
#endif
 
 // Zeit aktualisieren
  
  timeClient.update();

  int h = timeClient.getHours();
  int m = timeClient.getMinutes();

#ifdef GEBURTSTAGE
  // Datum ermitteln, wenn es 0-Uhr ist, oder das Datum unbekannt ist
    if ((h == 0 && m == 0) || heute_tag == -1 || heute_monat == -1 || heute_jahr == -1 ) {
      // einmal pro Tag das Datum holen, oder wenn einer der heute_*-Werte unbestimmt ist
      GetDatum(&heute_tag, &heute_monat, &heute_jahr);
    }
#endif


#ifdef TEMPERATURE
  // Temperatur alle 15 Minuten aktualisieren, oder falls keine Temperatur ermittelt werden konnte
  if (CONFIG.temp_active != 0 && (m == 0 || m == 15 || m == 30 || m == 45 || temperature == ERR_TEMP)) {
    temperature = GetTemperature();
  }
#endif  
  

#ifdef DUNKELSCHALTUNG
  if (jetztDunkelschaltung(hour, minute)) {
	SetBrightness(CONFIG.dunkelschaltung_brightness);
	//Serial.print  ("Dunkelschaltung jetzt!"); Serial.println(brightness);
  } else {
	SetBrightness(CONFIG.brightness);
  }
#else
	SetBrightness(CONFIG.brightness);
#endif


  switch (mymode) {

#ifdef TEMPERATURE
  case MODE_TEMP:
	// Temperatur ist angezeigt worden,
	//		zur Minute temperatur_minute
	//		und getstartet um temperatur_millis
	// 	Ausschalten nach x Milisekunden
	jetzt = millis();
	
	if (jetzt>temperatur_millis+TEMPERATURE_DURATION || jetzt < temperatur_millis) {
#ifdef GEBURTSTAGE
		setMode(MODE_BIRTHDAY);
#else
		setMode(MODE_TIME);
		hour = -1;			// Zeitanzeige erzwingen, falls wieder MODE_TIME
#endif
	}
	else {
		;	// nix tun, nur auf den nächsten Durchlauf warten
	}
	
	break;
#endif

#ifdef GEBURTSTAGE
  case MODE_BIRTHDAY:
#ifdef LAUFSCHRIFT
	if (isGeburtstagheut() && ((m % 5) == 0)) {
		if (m != geburtstag_minute ) {
			
			Serial.println("Starte Geburtstag");
			
			// Zu dieser Minute ist die Geburtstagslaufschrift noch nicht erschienen
			// und es gibt einen Geburtstag
			startLaufschrift("HAPPY BIRTHDAY," + getGebName() + "!");
			geburtstag_ende = 0;
			geburtstag_minute = m;
			geburtstag_millis= millis();
		}
		
		if (m == geburtstag_minute && geburtstag_ende != -1) {
			jetzt = millis();
			
			Serial.println("Warte Geburtstag");
			
			if (jetzt>geburtstag_millis+LAUFSCHRIFT_SPEED || jetzt < geburtstag_millis) {
				//einen Schritt in der Laufschrift
				geburtstag_ende = stepLaufschrift();
				geburtstag_millis = jetzt;
			
				if (geburtstag_ende == -1) {
					setMode(MODE_TIME);
					hour = -1;
				}
			}
		}
	}
	else {
		setMode(MODE_TIME);
		hour = -1;
	}
#endif
#else
	setMode(MODE_TIME);
	hour = -1;
	break;
#endif

  case MODE_TIME:
  	if (h != hour || m != minute) {
	  // neue Minute

      hour = h;
      minute = m;

#ifdef TEMPERATURE
	  //
	  // Auf Temperaturanzeige umschalten, wenn eine Temperatur ermittelt wurde, Temperaturanzeige aktiv ist und zu dieser Minute noch keine Temperatur angezeigt wurde
	  if (m != temperatur_minute && (CONFIG.temp_active == TEMPERATURE_MINUTE || (CONFIG.temp_active == TEMPERATURE_5MINUTE && ((m%5)==0))) && temperature != ERR_TEMP && temperature != -1) {
		// temperaturanzeige starten
		temperatur_minute = m;
		temperatur_millis = millis();
		setMode(MODE_TEMP);
	    showTemperature(temperature, GetTemperatureColor(temperature));
		// Temperaturanzeige wird in mode_temperatur beendet
      }
	  else {
        showTime(hour, minute);
	  }
#else
#ifdef GEBURTSTAGE
      setMode(MODE_GEBURTSTAGE);
#else	
	  showTime(hour, minute);
#endif
#endif
    }

#ifdef GEBURTSTAGE
    if (isGeburtstagheut() && mymode == MODE_TIME) {
      EVERY_N_MILLISECONDS( 50 ) {
        setRainbowColor();
        FastLED.show();
      }
    }
	break;
#endif
  }
  
  server.handleClient();
}
