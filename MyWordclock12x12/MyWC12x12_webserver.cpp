#define WEBSERVER_CPP
#include "MyWC12x12_config.h"

#include <ESP8266WebServer.h>
#include "MyWC12x12.h"
#include "MyWC12x12_webserver.h"
#include "MyWC12x12_temperatur.h"


//
// interne Funktionen
//
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

//
// Funktionen rund um die Konfigurationsseite des eigenen Webservers
//
String pad(int value) {
  if (value < 10) {
    return "0" + String(value);
  }

  return String(value);
}

String htmlOption(String label, String value, String store) {
  String content = "<option value=\"" + value + "\"";

  if (store == value) content += " selected=\"selected\"";

  content += ">" + label + "</option>";

  return content;
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

  for (int i = -12; i <= 12; i++) {
    label = String(i);

    if (i > 0) {
      label = "+" + label;
    }

    content += htmlOption(label, String(i), String(CONFIG.timezone));
  }
  content += "</select>";
  content += "</div>";


  content += "<hr>";
  
  content += "<div>";
  content += "<label>Region</label>";
  content += "<select name=\"locale\">";
  content += htmlOption(L_W_DEUTSCHLAND, String(L_DEUTSCHLAND), String(CONFIG.locale));
  content += htmlOption(L_W_FRANKEN, String(L_FRANKEN), String(CONFIG.locale));
  content += "</select>";
  content += "</div>";

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


#ifdef TEMPERATURE
  content += "<hr>";
  
  content += "<div>";
  content += "<label>Temperaturanzeige</label>";
  
  content += "<select class=\"time\" name=\"temp_active\">";
  content += htmlOption("Aus", 				String(0), String(CONFIG.temp_active));
  content += htmlOption("Jede Minute", 		String(1), String(CONFIG.temp_active));
  content += htmlOption("Jede 5. Minute", 	String(2), String(CONFIG.temp_active));
  content += "</select>";
  content += "</div>";

  content += "<div>";
  content += "<label>Ort</label>";
  content += "<input name=\"city\" value=\"" + String(CONFIG.city) + "\">";
  content += "<p style=\"font-size:0.75em;\">" + mywc_g_temperature_real_location + "</p>";
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

//
//	Argumente des http-request auslesen und die Änderungen erzwingen
//
void change() {

	/* 
	for (int i = 0; i < server.args(); i++)
	{
		Serial.println(server.argName(i) + " = " + server.arg(i));
	}
	*/

	if (server.hasArg("submit")) {

		if (server.arg("submit") == "testPower") {
			testPower();
		}
		else if (server.arg("submit") == "testLocale") {
			testLocale();
		}
#ifdef TEMPERATURE
		else if (server.arg("submit") == "testTemp") {
			testTemperatur();
		}
#endif
#ifdef LAUFSCHRIFT
		else if (server.arg("submit") == "testLaufschrift") {
			testLaufschrift();
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

			if (server.hasArg("fg")) 							CONFIG.color_fg 					= hexToRgb(server.arg("fg"));
			if (server.hasArg("bg")) 							CONFIG.color_bg 					= hexToRgb(server.arg("bg"));
			if (server.hasArg("brightness")) 					CONFIG.brightness 					= server.arg("brightness").toInt();

			if (server.hasArg("tz")) 							CONFIG.timezone 					= server.arg("tz").toInt();

			if (server.hasArg("dunkelschaltung_active"))		CONFIG.dunkelschaltung_active 		= server.arg("dunkelschaltung_active");
			if (server.hasArg("dunkelschaltung_start")) 		CONFIG.dunkelschaltung_start 		= server.arg("dunkelschaltung_start").toInt();
			if (server.hasArg("dunkelschaltung_end")) 			CONFIG.dunkelschaltung_end 			= server.arg("dunkelschaltung_end").toInt();
			if (server.hasArg("dunkelschaltung_brightness"))	CONFIG.dunkelschaltung_brightness	= server.arg("dunkelschaltung_brightness").toInt();

			if (server.hasArg("locale")) 						CONFIG.locale 						= server.arg("locale").toInt();

			if (server.hasArg("herz")) 							CONFIG.herz 						= server.arg("herz").toInt();
			if (server.hasArg("dat_herz")) 						CONFIG.dat_herz 					= server.arg("dat_herz").toInt();

#ifdef TEMPERATURE
			if (server.hasArg("temp_active")) 					CONFIG.temp_active 					= server.arg("temp_active").toInt();
			if (server.hasArg("city"))        					CONFIG.city        					= server.arg("city");
#endif
#ifdef GEBURTSTAGE
			String s;

			if (server.hasArg("geb_1")) 						CONFIG.geb_1 						= server.arg("geb_1").toInt();
			if (server.hasArg("geb_2")) 						CONFIG.geb_2 						= server.arg("geb_2").toInt();
			if (server.hasArg("geb_3")) 						CONFIG.geb_3 						= server.arg("geb_3").toInt();
			if (server.hasArg("geb_4")) 						CONFIG.geb_4 						= server.arg("geb_4").toInt();
			if (server.hasArg("geb_5")) 						CONFIG.geb_5 						= server.arg("geb_5").toInt();
			if (server.hasArg("geb_name_1")) 					{ s = server.arg("geb_name_1"); 	s.toUpperCase();	CONFIG.geb_name_1 = s; }
			if (server.hasArg("geb_name_2")) 					{ s = server.arg("geb_name_2"); 	s.toUpperCase();	CONFIG.geb_name_2 = s; }
			if (server.hasArg("geb_name_3")) 					{ s = server.arg("geb_name_3"); 	s.toUpperCase();	CONFIG.geb_name_3 = s; }
			if (server.hasArg("geb_name_4")) 					{ s = server.arg("geb_name_4"); 	s.toUpperCase();	CONFIG.geb_name_4 = s; }
			if (server.hasArg("geb_name_5")) 					{ s = server.arg("geb_name_5"); 	s.toUpperCase();	CONFIG.geb_name_5 = s; }
#endif
			//
			// Änderungen durchsetzen
			//
			
			// Speichern
			saveConfig();
			
			// Werte für die Anzeige aktualisieren und neu starten
			restart();
		}
	}
}

void handleRootPath() {
  String content = "";

  change();

  content += "<!DOCTYPE html><html>";
  content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  content += "<style>\n";
  content += "* { box-sizing: border-box; }\n";
  content += "html, body { font-family: Helvetica; margin: 0; padding: 0; }\n";
  content += ".form { margin: auto; max-width: 400px; }\n";
  content += ".form div { margin-top: 10; width: 100%; }\n";
  content += "label { width: 60%; display: inline-block; margin: 0; vertical-align: middle; }";
  content += "input, select { width: 38%; display: inline-block; margin: 0; border: 1px solid #eee; padding: 0; height: 40px; vertical-align: right; }";
  content += "select.time { width: 18%; }";
  content += "button { display: inline-block; width: 100%; font-size: 1.4rem; background-color: green; border: 1px solid #eee; color: #fff; padding-top: 10px; padding-bottom: 10px; }";
  content += "button.danger {  background-color: red;  }";
  content += "button.test {  background-color: yellow; }";
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

  content += "<hr label=\"Tests\">";
  
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testLocale\">Test Region</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testTemp\">Test Temperatur</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testLaufschrift\">Test Laufschrift</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testPower\">Test Stromverbrauch</button>";
  content += "</div>";
  
  content += "</form>";
  content += "</body></html>";

  server.sendHeader("Location", "http://" + ip);
  server.send(200, "text/html", content);

}

void startServer() {
  server.on("/", handleRootPath);
  server.begin();
}

void handleClientServer() {
  server.handleClient();
}