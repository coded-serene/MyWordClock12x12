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
// - Refactoring: Var brightness ist überflüssig
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
// 20200321 Umfangreiches Refactoring
// bis  	- Featuretoggle fuer HERZ und LOCALE entfernt, ab sofort enthalten.
// 20200322
//
//
#define MyWC12x12_INO
#include <Arduino.h>
#include <WebServer.h>
#include <LITTLEFS.h>
#include <WiFi.h>
#include "MyWC12x12_config.h"

#include <WiFiManager.h>        // wifimanager by tablatronix  https://github.com/tzapu/WiFiManager
#include <FastLED.h>            // http://fastled.io      https://github.com/FastLED/FastLED
#include <NTPClient.h>          // The MIT License (MIT) Copyright (c) 2015 by Fabrice Weinberg
#include "LittleFS.h"
#include <ArduinoJson.h>        // arduinojson.org
#include <time.h>

#include "MyWC12x12_temperatur.h"
#include "MyWC12x12_webserver.h"
#include "MyWC12x12_maske.h"
#include "MyWC12x12_geburtstage.h"

#ifdef FEATURE_OTA
#include <ArduinoOTA.h>         // OTA library
//#include <ESP8266mDNS.h>		// für OTA
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

char buffer[256];
#endif

// forward decl
void setup();
void loop();
void saveConfig();
void testPower();
void loadConfig();
void testLocale();
void resetLEDs();
void setBrightness(int b);
void showTime(int hour, int minute);
//
// exportierte Variable
//
CRGB leds[NUM_LEDS];
config_t CONFIG;
String ip;

int g_hour                      = -1;
int g_minute                    = -1;
int g_heute_tag                 = -1;
int g_heute_monat               = -1;
int g_heute_jahr                = -1;

int g_reboot_hour			    = -1;
int g_reboot_minute			    = -1;
int g_reboot_heute_tag 		    = -1;
int g_reboot_heute_monat	    = -1;
int g_reboot_heute_jahr		    = -1;

//
// lokale Modulvariable
//
int hour                        = -1;
int minute                      = -1;

static int myMode;			                         // aktueller Anzeigemodus für den Zustandsautomaten
// static bool mode_change;

#ifdef TEMPERATURE
// int getTemperature_minute       = -1;				// Minute, zu der zum letzten Mal die Temperatur angezeigt wurde
unsigned long temperatur_millis = -1;
#endif

#ifdef GEBURTSTAGE
// int geburtstag_minute           = -1;				// Minute, zu der zum letzten Mal die Geburtstagslaufschrift angezeigt wurde
unsigned long geburtstag_millis = -1;
int geburtstag_ende             = 0;
#endif

//
// Wifi-Manager für die Netzverbindung
WiFiManager wifiManager;


//
//
//
//
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
        Serial.println(L_W_DEUTSCHLAND);

        words_umschaltung_schwellwert = L_S_DEUTSCHLAND;
        wordsindex_minutes[3] = W_VIERTEL_NACH;
        wordsindex_minutes[4] = W_ZWANZIG_NACH;
        wordsindex_minutes[8] = W_ZWANZIG_VOR;
        wordsindex_minutes[9] = W_VIERTEL_VOR;
    }
}


//
// Funktionen zum Laden und Speichern der Konfiguration
//

void defaultConfig() {
    // Falbackwerte
#ifdef FEATURE_BG
    CONFIG.color_bg                     = CRGB::Black;
#endif
    CONFIG.color_fg 					= CRGB::White;
    CONFIG.brightness                   = BRIGHTNESS_DEFAULT; // %

    CONFIG.timezone 					= 1;

    // Fallback: Dunkelschaltung inaktiv
    CONFIG.dunkelschaltung_active       = true;
    CONFIG.dunkelschaltung_brightness   = 5; // %
    CONFIG.dunkelschaltung_start 		= 2200;
    CONFIG.dunkelschaltung_end          = 600;

#ifdef TEMPERATURE
    // Temperaturanzeige zu jeder Minute
    CONFIG.temp_active                  = TEMPERATURE_MINUTE;
    CONFIG.city                         = "Schwanstetten";
#endif

    // allg. deutsche Sprache
    CONFIG.locale                       = L_DEUTSCHLAND;

#ifdef GEBURTSTAGE
    CONFIG.geb_1                        = -1;
    CONFIG.geb_2                        = -1;
    CONFIG.geb_3                        = -1;
    CONFIG.geb_4                        = -1;
    CONFIG.geb_5                        = -1;
    CONFIG.geb_name_1                   = "";
    CONFIG.geb_name_2                   = "";
    CONFIG.geb_name_3                   = "";
    CONFIG.geb_name_4                   = "";
    CONFIG.geb_name_5                   = "";
#endif

    CONFIG.herz 						= HERZ_ROT;
    CONFIG.dat_herz 					= -1;
}


void loadConfig() {

    defaultConfig();

    File file = LITTLEFS.open(CONFIGFILE, "r");       // changed from SPIFFS (deprecated) to LittleFS

    if (!file) {
        Serial.println("Failed to open config file.");
        saveConfig();
        return;
    }

    Serial.println("Load config.");

    StaticJsonDocument<1024> doc;
    deserializeJson(doc, file);

    Serial.println(doc.as<String>());
#ifdef FEATURE_BG
    CONFIG.color_bg.r 				    = doc["color_bg_r"].as<int>();
    CONFIG.color_bg.g 				    = doc["color_bg_g"].as<int>();
    CONFIG.color_bg.b 				    = doc["color_bg_b"].as<int>();
#endif

    CONFIG.color_fg.r 				    = doc["color_fg_r"].as<int>();
    CONFIG.color_fg.g 				    = doc["color_fg_g"].as<int>();
    CONFIG.color_fg.b 				    = doc["color_fg_b"].as<int>();

    CONFIG.brightness 				    = doc["brightness"].as<int>();

    CONFIG.timezone 					= doc["timezone"].as<int>();
    //timeClient.setTimeOffset(CONFIG.timezone * 3600);
    configTime(3600 * CONFIG.timezone, 1 * 3600, "0.pool.ntp.org", "time.nist.gov", "1.pool.ntp.org");

    CONFIG.dunkelschaltung_active 	    = doc["dunkelschaltung_active"].as<bool>();
    CONFIG.dunkelschaltung_start 		= doc["dunkelschaltung_start"].as<int>();
    CONFIG.dunkelschaltung_end          = doc["dunkelschaltung_end"].as<int>();
    CONFIG.dunkelschaltung_brightness   = doc["dunkelschaltung_brightness"].as<int>();

    setBrightness(BRIGHTNESS_AUTO);

#ifdef TEMPERATURE
    CONFIG.temp_active 				    = doc["temp_active"].as<int>();
    CONFIG.city 						    = doc["city"].as<const char *>();
#endif

    CONFIG.locale 					    = doc["locale"].as<int>();
    // richtiges Sprachmodell auswaehlen
    changeLocale();

#ifdef GEBURTSTAGE
    CONFIG.geb_1 						= doc["geb_1"].as<int>();
    CONFIG.geb_2 						= doc["geb_2"].as<int>();
    CONFIG.geb_3 						= doc["geb_3"].as<int>();
    CONFIG.geb_4 						= doc["geb_4"].as<int>();
    CONFIG.geb_5 						= doc["geb_5"].as<int>();
    CONFIG.geb_name_1 				    = doc["geb_name_1"].as<const char *>();
    CONFIG.geb_name_2 				    = doc["geb_name_2"].as<const char *>();
    CONFIG.geb_name_3 				    = doc["geb_name_3"].as<const char *>();
    CONFIG.geb_name_4 				    = doc["geb_name_4"].as<const char *>();
    CONFIG.geb_name_5 				    = doc["geb_name_5"].as<const char *>();
#endif

    CONFIG.herz 						= doc["herz"].as<int>();
    CONFIG.dat_herz 					= doc["dat_herz"].as<int>();

    file.close();
}


void saveConfig() {
  File file = LITTLEFS.open(CONFIGFILE, "w");

  if (!file) {
    Serial.println("Can't open configfile for writing");
	Serial.println(CONFIGFILE);
    return;
  }

  Serial.println("Save config.");

  StaticJsonDocument<1024> doc;

#ifdef FEATURE_BG
  doc["color_bg_r"] 				= CONFIG.color_bg.r;
  doc["color_bg_g"] 				= CONFIG.color_bg.g;
  doc["color_bg_b"] 				= CONFIG.color_bg.b;
#endif
  doc["color_fg_r"] 				= CONFIG.color_fg.r;
  doc["color_fg_g"] 				= CONFIG.color_fg.g;
  doc["color_fg_b"] 				= CONFIG.color_fg.b;
  doc["brightness"] 				= CONFIG.brightness;
  doc["timezone"] 					= CONFIG.timezone;

  doc["dunkelschaltung_active"] 	= CONFIG.dunkelschaltung_active;
  doc["dunkelschaltung_start"] 		= CONFIG.dunkelschaltung_start;
  doc["dunkelschaltung_end"] 		= CONFIG.dunkelschaltung_end;
  doc["dunkelschaltung_brightness"]	= CONFIG.dunkelschaltung_brightness;

#ifdef TEMPERATURE
  doc["city"] 						= CONFIG.city;
  doc["temp_active"] 				= CONFIG.temp_active;
#endif

  doc["locale"] 					= CONFIG.locale;

#ifdef GEBURTSTAGE
  doc["geb_1"] 						= CONFIG.geb_1;
  doc["geb_2"] 						= CONFIG.geb_2;
  doc["geb_3"] 						= CONFIG.geb_3;
  doc["geb_4"] 						= CONFIG.geb_4;
  doc["geb_5"] 						= CONFIG.geb_5;
  doc["geb_name_1"] 				= CONFIG.geb_name_1;
  doc["geb_name_2"] 				= CONFIG.geb_name_2;
  doc["geb_name_3"] 				= CONFIG.geb_name_3;
  doc["geb_name_4"] 				= CONFIG.geb_name_4;
  doc["geb_name_5"] 				= CONFIG.geb_name_5;
#endif
  doc["herz"] 						= CONFIG.herz;
  doc["dat_herz"] 					= CONFIG.dat_herz;

  serializeJson(doc, file);

  Serial.println(doc.as<String>());

  file.close();
}


// Testfunktionen für den LED-Streifen
//
void testPower() {

	resetLEDs();

	setBrightness(BRIGHTNESS_MAX);

	for (int i=0; i<NUM_LEDS; i++) {
	  leds[i] = CRGB::White;

	  FastLED.show();

	  delay(1000);
	}

	setBrightness(BRIGHTNESS_AUTO);
}

void testLocale() {
	for (int i = 0; i < 60; i++) {
		showTime(12, i);
		delay(500);
	}
}

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

//
// Zeitfunktionen
//
/**/

void GetTimeAndDate(int *stunden, int *minuten, int *tag, int *monat, int *jahr) {
	struct tm tmstruct;

	getLocalTime(&tmstruct, 5000);

	*stunden	= tmstruct.tm_hour;
	*minuten	= tmstruct.tm_min;
	*tag		= tmstruct.tm_mday;
	*monat		= tmstruct.tm_mon + 1;
	*jahr		= tmstruct.tm_year + 1900;
}

String GetDatumZeitString() {
    struct tm tmstruct;
    char buffer[80];


    getLocalTime(&tmstruct, 5000);

    strftime(buffer, 80, "%d.%m.%Y %H:%M", &tmstruct);

    return String(buffer);
}
// Funktionen zum Arbeiten mit dem LED-Streifen
//
// set-Funktionen setzen nur die Informationen im Array für die LEDs
// show-Funktionen rufen auch ein FastLED.show() auf und setzen am Beginn der Funktion die LEDs zurück
//
// alle LEDs auf Hintergrundfarbe setzen
//

// Setzen der Helligkeit in %
//
void setBrightness(int b) {

	if (b == BRIGHTNESS_AUTO) {
		b = (jetztDunkelschaltung(hour, minute)) ? CONFIG.dunkelschaltung_brightness : CONFIG.brightness;
	}

	if ((b<0)||(b>100))
		return;

	FastLED.setBrightness((int)((float)b * 2.55));
}


void resetLEDs() {
  int i;

  for (i = 0; i < NUM_LEDS; i++) {
#ifdef FEATURE_BG
    leds[i] = CONFIG.color_bg;
#else
    leds[i] = CRGB::Black;
#endif
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
// Eine Zahl mit Vorzeichen anzeigen, Wert von -39 bis +39 möglich
//
void setNumber(int n, CRGB c = CRGB::White) {
	int einer;

	// ungültige Werte
	if (n < -39 || n > 39) return;

	// Vorzeichen Plus oder Minus, keines bei 0 Grad
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
		else if (n == 20) {
		  setWord(W_ZWANZIG, c);
		}
		else if (n > 20 && n < 30) {
		  setWord(W_UND, c);
		  setWord(W_ZWANZIG, c);
		}
		else if (n == 30) {
		  setWord(W_DREISSIG, c);
		}
		else if (n > 30) {
		  setWord(W_UND, c);
		  setWord(W_DREISSIG, c);
		}
	}
}


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

void testLaufschrift() {
	startLaufschrift("DIES IST EIN TEST.", CRGB::White);

	while (stepLaufschrift() != -1) {
		delay(LAUFSCHRIFT_SPEED);
	}
}
#endif

void setHerz() {

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
		if (g_heute_tag * 100 + g_heute_monat == CONFIG.dat_herz) {
			setWord(W_HERZ, CRGB::Red);
		}
		break;
  case HERZ_STD_DATUM:
		if (g_heute_tag * 100 + g_heute_monat == CONFIG.dat_herz) {
			setWord(W_HERZ, CRGB::Red);
		} else {
			setWord(W_HERZ, CONFIG.color_fg);
		}
		break;
  }
}


//
// Ein einzelnes Wort aus der Worttabelle setzen und anzeigen lassen
//
void showWord(int w, CRGB c = CRGB::White) {
  resetLEDs();
  setWord(w, c);
  FastLED.show();
}

void showWLAN(CRGB c) {
  resetLEDs();
  setWord(W_WLAN, c);
  FastLED.show();
}

//
//	Die Kernfunktion, Umwandeln der aktuellen Zeit in Worte und anzeigen
//
void showTime(int hour, int minute) {
  int singleMinute;
  if (hour == -1 || minute == -1) return;

  singleMinute = minute % 5;
  minute = (minute - (minute % 5));

  if (minute >= words_umschaltung_schwellwert) hour += 1;

  minute = minute / 5;
  hour = hour % 12;

  resetLEDs();

  setHerz();

  if (WiFi.status() != WL_CONNECTED) {
	setWord(W_WLAN, CRGB::Red);
  }

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


//
// Funktionen zum WiFi-Betrieb
//
void resetWiFi() {
  wifiManager.resetSettings();
}
void resetConfig() {
  LITTLEFS.remove(CONFIGFILE);
}
void resetAllAndReboot() {
  resetConfig();
  resetWiFi();
  //ESP.reset();
  ESP.restart();
  delay(5000);
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


//
// evtl hat sich ein Parameter geändert. Dann die Anzeige neu initialisieren
//
void restart() {
	// evtl. haben wir eine neue Zeitzone....
	configTime(3600 * CONFIG.timezone, 1 * 3600, "0.pool.ntp.org", "time.nist.gov", "1.pool.ntp.org");

	// evtl. geaenderte Helligkeit durchsetzen
	setBrightness(BRIGHTNESS_AUTO);

#ifdef TEMPERATURE
	// evtl. wurde der Ort geaendert, also sicherheitshalber die Temperatur neu holen
	mywc_g_temperature = GetTemperature(CONFIG.city);
#endif

    Serial.println("restart()");

	// vllt. ist die Regionaleinstellung geändert...
	changeLocale();

	// Zeitanzeige in loop() erzwingen
	hour = -1;
}

//
// Initialisierung
//
void setup() {
	IPAddress ipL;


	Serial.begin(115200);

#ifdef GEBURTSTAGE
	Serial.println("Feature Geburtstage enabled!");
#endif
#ifdef LAUFSCHRIFT
	Serial.println("Feature Laufschrift enabled!");
#endif
#ifdef TEMPERATURE
	Serial.println("Feature Temperaturanzeige enabled!");
#endif
#ifdef FEATURE_OTA
	Serial.println("Feature OTA enabled!");
#endif
#ifdef FEATURE_BG
	Serial.println("Feature Background enabled!");
#endif

	// Dateisystem initialisieren
	LITTLEFS.begin();

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
	setBrightness(BRIGHTNESS_DEFAULT);

	// WLAN-Konfiguration
	//
	showWord(W_WLAN, CRGB::Red);
	WiFi.hostname("iotWordClock12x12");

	wifiManager.setAPCallback(configModeCallback);

	if (!wifiManager.autoConnect("WordClock")) {
		Serial.println("WiFiManager; failed to connect and hit timeout");
		//reset and try again, or maybe put it to deep sleep
		//ESP.reset();
		ESP.restart();
		delay(1000);
	}

	showWord(W_WLAN, CRGB::Green);
	delay(1000);

	ipL = WiFi.localIP();
	ip = ipL.toString();
	showIP(ipL.toString());
	//
	// WLAN-Konfiguration passt


	// OTA
	//
#ifdef FEATURE_OTA
	// Port defaults to 8266
	// ArduinoOTA.setPort(8266);
	ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	// ArduinoOTA.setHostname("myesp8266");
	ArduinoOTA.setHostname("iotWordClock12x12");

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

			// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using LittleFS.end()
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

    g_heute_tag        = -1;
    myMode             = MODE_TEMP_FIRST;			// aktueller Anzeigemodus für den Zustandsautomaten
#ifdef TEMPERATURE
    mywc_g_temperature = ERR_TEMP;
#endif

	// start webserver
	startServer();

	// Configure time-service, mit Sommer/Winterzeit von einer Stunde
	configTime(3600 * CONFIG.timezone, 1 * 3600, "0.pool.ntp.org", "time.nist.gov", "1.pool.ntp.org");

    // Holen der RebootZeit
    GetTimeAndDate(&g_reboot_hour, &g_reboot_minute, &g_reboot_heute_tag, &g_reboot_heute_monat, &g_reboot_heute_jahr);
}

void loop() {

#if defined(GEBURTSTAGE) || defined(TEMPERATURE)
	unsigned long jetzt;
#endif
#ifdef GEBURTSTAGE
    String geb_name;
#endif

	//
	// Zeit aktualisieren
	//
	GetTimeAndDate(&g_hour, &g_minute, &g_heute_tag, &g_heute_monat, &g_heute_jahr);

	//
	// Diese Aktionen sollten nur noch einmal pro Minute laufen
	//
	if (hour != g_hour || minute != g_minute) {

    Serial.println("loop() new minute");

		// neue Minute
		hour 		= g_hour;
		minute 		= g_minute;

		// wir starten mit der Temperatur....
		myMode 		= MODE_TEMP_FIRST;

		// Falls gerade die Dunkelschaltung erfolgt...
		setBrightness(BRIGHTNESS_AUTO);

		if ((g_minute % 5 == 0) && (WiFi.status() != WL_CONNECTED)) {
			// alle 5 Minuten
			// Falls das WLAN nicht funktioniert, reconnect versuchen
            (void)wifiManager.autoConnect("WordClock");
        }
	}


	switch (myMode) {

	case MODE_TIME_FIRST:
		showTime(g_hour, g_minute);

        myMode = MODE_TIME;
		break;

    case MODE_TIME:
		// Nix tun. Die Zeit sollte bereits angezeigt werden und dann warten, auf eine neue Minute
		break;

#ifdef GEBURTSTAGE
	case MODE_BIRTHDAY_FIRST:
#ifdef LAUFSCHRIFT
		if (isGeburtstagheut(g_heute_tag, g_heute_monat) && ((g_minute % 5) == 0)) {
                geb_name = getGebName(g_heute_tag, g_heute_monat);
                geb_name.trim();

                if (geb_name != "") {
                    // Nur starten, falls der Name nicht nur aus Leerzeichen besteht
    				Serial.println("Starte Geburtstag");

    				// Zu dieser Minute ist die Geburtstagslaufschrift noch nicht erschienen
    				// und es gibt einen Geburtstag
    				startLaufschrift("HAPPY BIRTHDAY," + geb_name + "!");
    				geburtstag_ende = false;
    				// geburtstag_minute = g_minute;
    				geburtstag_millis= millis();
                }
			}
#else
		// Wenn keine Geburtstagslaufschrift, dann MODE durchschalten
#endif
        myMode = MODE_BIRTHDAY;
		break;

	case MODE_BIRTHDAY:
		if (!geburtstag_ende) {
			jetzt = millis();

			Serial.println("Warte Geburtstag " + String(jetzt) + " geburtstag_millis=" + String(geburtstag_millis) + " Diff=" + String(jetzt - geburtstag_millis));

			if (jetzt>geburtstag_millis+LAUFSCHRIFT_SPEED || jetzt < geburtstag_millis) {
				//einen Schritt in der Laufschrift
				geburtstag_ende = stepLaufschrift();
				geburtstag_millis = jetzt;
			}
		}
		else {
			myMode = MODE_TIME_FIRST;
		}
		break;
#endif

#ifdef TEMPERATURE
	case MODE_TEMP_FIRST:
    if (CONFIG.temp_active != TEMPERATURE_AUS) {

      Serial.println("MODE_TEMP_FIRST");
      //
      // Temperatur alle TEMPERATURE_REFETCH_MINUTES Minuten aktualisieren, oder falls keine Temperatur ermittelt werden konnte
      // Weil der Dienst zu vollen Viertelstunden Fehler lieferte habe ich den REFETCH_SHIFT eingebaut.
      //
      if	( (g_minute % TEMPERATURE_REFETCH_MINUTES) == TEMPERATURE_REFETCH_SHIFT || mywc_g_temperature == ERR_TEMP) 	{ // wenn die Temperatur ermittelt werden soll oder muss (keine aktuelle Temperatur)
        Serial.println("MODE_TEMP_FIRST Ort=" + CONFIG.city);
        mywc_g_temperature = GetTemperature(CONFIG.city);
        Serial.println("MODE_TEMP_FIRST temperature=" + mywc_g_temperature);
      }

      Serial.println(mywc_g_temperature);
      //
      // Auf Temperaturanzeige umschalten, wenn eine Temperatur ermittelt wurde, Temperaturanzeige aktiv ist und zu dieser Minute noch keine Temperatur angezeigt wurde
      // getTemperature_minute ist die Zeit zu der zuletzt die Temperatur angezeigt wurde
      //
      if (	CONFIG.temp_active == TEMPERATURE_MINUTE
        || (CONFIG.temp_active == TEMPERATURE_5MINUTE && ((g_minute % 5)==0))
      ) {
        // temperaturanzeige starten
        temperatur_millis = millis();		// zur Realisierung einer Anzeigedauer diese Startzeit der Anzeige merken

        showTemperature(mywc_g_temperature);

        myMode = MODE_TEMP;
      }
    	else {
    			// zu dieser Minute keine Temperaturanzeige
#ifdef GEBURTSTAGE
			    myMode = MODE_BIRTHDAY_FIRST;
#else
			    myMode = MODE_TIME_FIRST;
#endif
		  }
    }
    else {
#ifdef GEBURTSTAGE
		    myMode = MODE_BIRTHDAY_FIRST;
#else
		    myMode = MODE_TIME_FIRST;
#endif
    }
		break;

    case MODE_TEMP:
		// Temperatur ist angezeigt worden,
		//		zur Minute getTemperature_minute
		//		und getstartet um temperatur_millis
		// 	Ausschalten nach x Milisekunden
		jetzt = millis();

		if (jetzt>temperatur_millis+TEMPERATURE_DURATION || jetzt < temperatur_millis) {
            // Zum nächsten Mode weiterschalten
#ifdef GEBURTSTAGE
			myMode = MODE_BIRTHDAY_FIRST;
#else
			myMode = MODE_TIME_FIRST;
#endif
		}

		break;
#endif

    default:
        showTime(g_hour, g_minute);
        break;

	}

	//
	// Diese Aktionen bei jedem Schleifendurchlauf
	//
#ifdef GEBURTSTAGE
	if (isGeburtstagheut(g_heute_tag, g_heute_monat) && myMode == MODE_TIME) {
		EVERY_N_MILLISECONDS( 50 ) {
			setRainbowColor();
			FastLED.show();
		}
	}
#endif

	//
	// evtl. Update Einspielen
	//
#ifdef FEATURE_OTA
	ArduinoOTA.handle();
#endif

	//
	// Webserver bedienen
	//
	handleClientServer();
}
