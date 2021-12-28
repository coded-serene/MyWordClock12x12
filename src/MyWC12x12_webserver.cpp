#define WEBSERVER_CPP
#include <Arduino.h>
#include <WebServer.h>
#include "MyWC12x12_config.h"
#include "MyWC12x12.h"
#include "MyWC12x12_webserver.h"
#include "MyWC12x12_temperatur.h"
#include "main.h"
#include "Scroller.h"
#include "BannerText.h"


// forward decl
void managePathArgsTetrisGame(void) ;

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

String dat2String(int value) {
    if (value <= 100) {
        return String(" ");
    }
    else if (value < 1000) {
      return "0" + String(value);
    }
    else
        return String(value);
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
String getFormRoot() {
  String content = "";
  String label = "";

  // Anzeigefarbe
  content += "<h2>Grundeinstellungen</h2>";
  content += "<div>";
  content += "<label>Vordergrundfarbe</label>";
  content += "<input name=\"fg\" value=\"#" + rgbToHex(CONFIG.color_fg) + "\" type=\"color\">";
  content += "</div>";

#ifdef FEATURE_BG
  // Hintergrundfarbe
  content += "<div>";
  content += "<label>Hintergrundfarbe</label>";
  content += "<input name=\"bg\" value=\"#" + rgbToHex(CONFIG.color_bg) + "\" type=\"color\">";
  content += "</div>";
#endif

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
  content += "<input name=\"dat_herz\" value=\"" + dat2String(CONFIG.dat_herz) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<hr label=\"Dunkelschaltung\">";
  content += "<div>";
  content += "<h2>Nachtmodus</h2>";
  // content += "<input type=\"checkbox\" name=\"dunkelschaltung_active\" value=\"1\"";
  // if (CONFIG.dunkelschaltung_active) {
  //   content += "checked";
  // };
  // content += ">";
  content += "</div>";

  // abgesenkte Helligkeit
  content += "<div>";
  content += "<label>reduzierte Helligkeit (" + String(CONFIG.dunkelschaltung_brightness) + "%)</label>";
  content += "<input type=\"range\" name=\"dunkelschaltung_brightness\" min=\"0\" max=\"50\" value=\"" + String(CONFIG.dunkelschaltung_brightness) + "\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Nachtmodus Startzeit</label>";
  content += "<select class=\"time\" name=\"dunkelschaltung_start\">";

  content += htmlOption("Aus", String(-1), String(CONFIG.dunkelschaltung_start));

  for (int i = 0; i < 24; i++) {
    content += htmlOption(pad(i) + ":00", String(i * 100), String(CONFIG.dunkelschaltung_start));
    content += htmlOption(pad(i) + ":30", String(i * 100 + 30), String(CONFIG.dunkelschaltung_start));
  }

  content += "</select>";
  content += "</div>";

  content += "<div>";
  content += "<label>Nachtmodus Endzeit</label>";
  content += "<select class=\"time\" name=\"dunkelschaltung_end\">";

  content += htmlOption("Aus", String(-1), String(CONFIG.dunkelschaltung_end));

  for (int i = 0; i < 24; i++) {
    content += htmlOption(pad(i) + ":00", String(i * 100), String(CONFIG.dunkelschaltung_end));
    content += htmlOption(pad(i) + ":30", String(i * 100 + 30), String(CONFIG.dunkelschaltung_end));
  }

  content += "</select>";
  content += "</div>";


#ifdef TEMPERATURE
  content += "<hr>";
  content += "<h2>Temperaturanzeige</h2>";
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
  content += "<!-- hr><p style=\"font-size:0.75em;\">" + mywc_g_debug_temperature + "</p> -->";
  content += "</div>";
#endif


/* MQTT-Config */
  content += "<hr>";
  content += "<h2>Netzwerk / MQTT</h2>";
  content += "<div>";
  content += "<label>Hostname</label>";
  content += "<input name=\"txtNetworkHostname\" value=\"" + String(CONFIG.networkHostname) + "\">";
  content += "<label>MQTT-Server <x-small>(disabled if empty)</x-small></label>";
  content += "<input name=\"txtMqttServer\" placeholder='leave empty to disable mqtt' value=\"" + String(CONFIG.mqttServerName) + "\">";
  content += "<label>MQTT-Port</label>";
  content += "<input name=\"txtMqttPort\" value=\"" + String(CONFIG.mqttPort) + "\">";
  content += "<label>MQTT-UserName (empty if unsecure)</label>";
  content += "<input name=\"txtMqttUserName\" value=\"" + String(CONFIG.mqttUserName) + "\" >";
  content += "<label>MQTT-Password</label>";
  content += "<input type='password' name=\"txtMqttPassword\" value=\"" + String(CONFIG.mqttPassword) + "\" >";
  content += "<div>";
  content += "<label>Subscripted mqtt-Topic: &lthostname&gt/Config/#</label>";
  content += "<label>topic 'r': startStop tetris</label>";
  content += "<label>topic 'a': tetris brick left</label>";
  content += "<label>topic 's': tetris brick right</label>";
  content += "<label>topic 'y': tetris brick down</label>";
  content += "<label>topic ' ': tetris brick flip</label>";
  content += "</div>";
  content += "<div>";
  content += "<label>BannerHintText</label><input name=\"txtBannerHintText\" value=\"" + globalBannerText.GetRuntimeBannerHintText()+ "\" >";
  content += "<label>BannerText (immer an)</label><input name=\"txtBannerText\" value=\"" + globalBannerText.GetRuntimeBannerText() + "\" >";
  content += "</div>";
  content += "</div>";
/////////////////////
#ifdef GEBURTSTAGE
  content += "<hr>";
  content += "<h2>Geburtstage</h2>";
  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_1\" value=\"" + String(CONFIG.geb_name_1) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_1\" value=\"" + dat2String(CONFIG.geb_1) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_2\" value=\"" + String(CONFIG.geb_name_2) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_2\" value=\"" + dat2String(CONFIG.geb_2) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_3\" value=\"" + String(CONFIG.geb_name_3) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_3\" value=\"" + dat2String(CONFIG.geb_3) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_4\" value=\"" + String(CONFIG.geb_name_4) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_4\" value=\"" + dat2String(CONFIG.geb_4) + "\" maxlength=\"4\">";
  content += "</div>";

  content += "<div>";
  content += "<label>Name</label>";
  content += "<input name=\"geb_name_5\" value=\"" + String(CONFIG.geb_name_5) + "\" maxlength=\"24\">";
  content += "<label>Geburtstag (TTMM)</label>";
  content += "<input name=\"geb_5\" value=\"" + dat2String(CONFIG.geb_5) + "\" maxlength=\"4\">";
  content += "</div>";
#endif

  return content;
}

//
//	Argumente des http-request auslesen und die Änderungen erzwingen
//
void managePathArgsRoot() {

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
			Scroller::ScrollerTest(globalBannerText.GetRuntimeBannerText());
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
#ifdef FEATURE_BG
			if (server.hasArg("bg")) 							CONFIG.color_bg 					= hexToRgb(server.arg("bg"));
#endif
			if (server.hasArg("brightness")) 					CONFIG.brightness 					= server.arg("brightness").toInt();

			if (server.hasArg("tz")) 							CONFIG.timezone 					= server.arg("tz").toInt();

			// if (server.hasArg("dunkelschaltung_active"))		CONFIG.dunkelschaltung_active 		= server.arg("dunkelschaltung_active").toInt();
			if (server.hasArg("dunkelschaltung_start")) 		CONFIG.dunkelschaltung_start 		= server.arg("dunkelschaltung_start").toInt();
			if (server.hasArg("dunkelschaltung_end")) 			CONFIG.dunkelschaltung_end 			= server.arg("dunkelschaltung_end").toInt();
                                                                CONFIG.dunkelschaltung_active       = (CONFIG.dunkelschaltung_start>=0 && CONFIG.dunkelschaltung_end>=0);
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

      if (server.hasArg("txtMqttServer")) 					{ s = server.arg("txtNetworkHostname"); 	CONFIG.networkHostname = s; }
      if (server.hasArg("txtMqttServer")) 					{ s = server.arg("txtMqttServer"); 	s.toUpperCase();	CONFIG.mqttServerName = s; }
      if (server.hasArg("txtMqttPort")) 					{ CONFIG.mqttPort = server.arg("txtMqttPort").toInt(); }
      if (server.hasArg("txtMqttUserName")) 					{ s = server.arg("txtMqttUserName"); 	CONFIG.mqttUserName = s; }
      if (server.hasArg("txtMqttPassword")) 					{ s = server.arg("txtMqttPassword"); 	CONFIG.mqttPassword = s; }

      if (server.hasArg("txtBannerHintText")) 					{ s = server.arg("txtBannerHintText"); 	globalBannerText.SetRuntimeBannerHintText(s); }
      if (server.hasArg("txtBannerText")) 					{ s = server.arg("txtBannerText"); 	globalBannerText.SetRuntimeBannerText(s); }
			//
			// Änderungen durchsetzen
			//

			// Speichern
			saveConfig();

			// Werte für die Anzeige aktualisieren und neu starten
			//restart();
		}
	}

}
String getFormHeaderHtml(void) 
{
  String content = "";
  content += "<!DOCTYPE html><html>";
  content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  content += "<style>\n";
  content += "* { box-sizing: border-box; }\n";
  content += "html, body { font-family: Helvetica; margin: 0; padding: 0; }\n";
  content += ".form { margin: auto; max-width: 400px; }\n";
  content += ".form div { margin-top: 10; width: 100%; }\n";
  content += "label { width: 60%; display: inline-block; margin: 0; vertical-align: middle; }\n";
  content += "input, select { width: 38%; display: inline-block; margin: 0; border: 1px solid #eee; padding: 0; height: 40px; vertical-align: middle; }\n";
  content += "select.time { width: 18%; }\n";
  content += "button { display: inline-block; width: 100%; font-size: 1.4rem; background-color: green; border: 1px solid #eee; color: #fff; padding-top: 10px; padding-bottom: 10px; }\n";
  content += "button.danger {  background-color: red;  }\n";
  content += "button.test {  background-color: yellow; }\n";
  content += "h2 { font-size:medium; }\n";
  content += "</style>\n";
  content += "</head>\n";
  content += "<body>\n";
  return content;
}
String getFormTetrisGame(void) 
{
  String content = getFormHeaderHtml();
  
  content += "<h1 align=center >Tetris on Wordclock. - Play it!</h1>";
  content += "<form class=\"form\" method=\"post\" action=\"\">";
  
  content += "<div> <button name='submit' type='submit' value='flip'>Drehen</button> </div>";

  content += "<table width=100%><tr><td>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"left\">Links</button>";
  content += "</div>";
  content += "</td><td>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"right\">Rechts</button>";
  content += "</div>";
  content += "</td></tr></table>";
    
  content += "<div> <button name='submit' type='submit' value='down'>Runter</button> </div>";
  content += "<br/><br/><div> <button class='danger' name='submit' type='submit' value='back'>Start/Restart Game</button> </div>";

  content += "</form>";
  content += "</body></html>";

  return content;
}
void handleTetrisGame(void)
{
  managePathArgsTetrisGame();

  String content = getFormTetrisGame();

  server.sendHeader("Location", "http://" + ip+"/tetris");
  server.send(200, "text/html", content);

}
//
//	Argumente des http-request auslesen und die Änderungen erzwingen
//
void managePathArgsTetrisGame(void) 
{

	/*
	for (int i = 0; i < server.args(); i++)
	{
		Serial.println(server.argName(i) + " = " + server.arg(i));
	}
	*/

	if (server.hasArg("submit")) 
  {
    gameCommand cmd = gameCommand::GameCommandNone; 
    if (server.arg("submit") =="right")	{cmd = gameCommand::GameCommandRight;  }
    if (server.arg("submit") =="left") 	{cmd = gameCommand::GameCommandLeft; }
    if (server.arg("submit") =="flip") 	{cmd = gameCommand::GameCommandFire;  }
    if (server.arg("submit") =="down") 	{cmd = gameCommand::GameCommandDown;  }
    if (server.arg("submit") =="back")  {cmd = gameCommand::GameCommandStartStop; }

    Serial.print("Webserver: Known Tetris-SubmittedCommand: ");
    Serial.println(cmd);
    handleTetrisCommand(cmd);
	}
  else
  {
    Serial.print("Webserver: Unrecognized Tetris-SubmittedCommand: ");
    Serial.print(server.uri());
    Serial.println(".");
  }

  
}
void handleRootPath()
{
  managePathArgsRoot();

  String content = getFormHeaderHtml();

  content += "<hr/><br/><div class='tetrislink'><a class='tetrislink' href=http://"+ip+"/tetris>Tetris starten</a></div><br/><hr/>";
  content += "<h1 align=center>WordClockEx Konfiguration</h1>";
  content += "<form class=\"form\" method=\"post\" action=\"\">";

  content += "<div>";
  content += "<p>Es ist jetzt " + pad(g_hour) + ":" + pad(g_minute) + "Uhr am " + pad(g_heute_tag) + "." + pad(g_heute_monat) + "." + pad(g_heute_jahr) + "</p";
  content += "<div>";

  content += "<div>";
  content += getFormRoot();
  content += "</div>";

  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" value=\"save\">Speichern</button>";
  content += "</div>";


  content += "<hr label=\"Zurücksetzen\">";

  content += "<div>";
  content += "<p>Letzter Neustart " + pad(g_reboot_hour) + ":" + pad(g_reboot_minute) + "Uhr am " + pad(g_reboot_heute_tag) + "." + pad(g_reboot_heute_monat) + "." + pad(g_reboot_heute_jahr) + "</p";
  content += "<div>";

  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetConfig\" background-color=\"red\";>Konfiguration zur&uuml;cksetzen</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetWiFi\" background-color: red;>WLAN-Parameter zur&uuml;cksetzen</button>";
  content += "</div>";
  content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"danger\" value=\"ResetAll\" background-color: red;>Komplett zur&uuml;cksetzen und Reboot</button>";
  content += "</div>";
  //
  // content += "<hr label=\"Tests\">";
  //
  // content += "<div>";
  // content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testLocale\">Test Region</button>";
  // content += "</div>";
  // content += "<div>";
  // content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testTemp\">Test Temperatur</button>";
  // content += "</div>";
   content += "<div>";
  content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testLaufschrift\">Test BannerText</button>";
  content += "</div>";
  // content += "<div>";
  // content += "<button name=\"submit\" type=\"submit\" class=\"test\" value=\"testPower\">Test Stromverbrauch</button>";
  // content += "</div>";

  content += "</form>";
  content += "</body></html>";

  server.sendHeader("Location", "http://" + ip);
  server.send(200, "text/html", content);

}

void startServer() {
  server.on("/", handleRootPath);
  server.on("/tetris", handleTetrisGame);
  server.begin();
}

void handleClientServer() {
  server.handleClient();
}
