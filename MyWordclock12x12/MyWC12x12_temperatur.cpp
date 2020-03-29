#define TEMPERATUR_CPP
#include "MyWC12x12_config.h"

#ifdef TEMPERATURE

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>            // http://fastled.io      https://github.com/FastLED/FastLED

#include "MyWC12x12_temperatur.h"
#include "MyWC12x12_maske.h"
#include "MyWC12x12.h"

//
// globale Variable
//
int temperature = ERR_TEMP;  			// temperatur
String temperature_real_location;		// von wttr.in tatsächlich genutzter Ort, zur Ausgabe auf der Webseite
										// kann auch eine Fehlermeldung enthalten. Diese beginnt immer mit "Fehler"


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


//
// WETTER, Ort holen
//
String GetTemperatureRealLocation(String city) {
	WiFiClient client;
	HTTPClient http;            //Declare object of class HTTPClient
	int httpCode;	
	String weatherstring;
	String payload;
	String location;
	int location_start;
	int location_end;



	if (WiFi.status() == WL_CONNECTED) {                    						//Check WiFi connection status
		weatherstring = "http://wttr.in/" + city + "?1&lang=en";    //Specify request destination

		Serial.println(weatherstring);

		// Die HTTP-Antwort ist zu groß zum Lesen-In-Einem-Stück >25kB
		// also stückweise lesen und gleich suchen nach Location: ... [
		
		http.begin(client, weatherstring);
		httpCode = http.GET();

		if (httpCode == 200) {

			payload="";
			location = "Fehler: Ort war nicht in der Anfrageantwort enthalten.";
			
			// get lenght of document (is -1 when Server sends no Content-Length header)
			int len = http.getSize();

			// create buffer for read
			char buff[129] = { 0 };
			buff[128] = (char)0;

			// get tcp stream
			WiFiClient *stream = &client;

			location_start=-1;
			location_end=-1;

			// read all data from server
			while (http.connected() && (len > 0 || len == -1)) {
			
				// read up to 128 byte
				int c = stream->readBytes(buff, std::min((size_t)len, sizeof(buff)));
				buff[c]=0;

				payload += String(buff);
				//Serial.println("Lese. Payload=" + payload);

				if (len > 0) {
					len -= c;
				}
			
				if (location_start == -1) {
					// Noch keinen Anfang (=="Location: ") gefunden
					location_start = payload.indexOf("Location: ");
					if (location_start >=0) {
						// aber in diesem frisch gelesenen Puffer
						payload = payload.substring(location_start);
						Serial.println("Los gehts. Payload=" + payload);
					} else {
						// Nicht gefunden. payload kürzen, aber die letzten 11 Zeichen behalten (falls da ein Teil des "Location: " drin steckt
						payload = payload.substring(max((int)0,(int)(payload.length()-11)));
					}
				}
			
				if (location_start >=0 && location_end == -1) {
					// Also der Anfang ist gefunden worden.
					location_end = payload.indexOf("[");
					if (location_end >= 0) {
						// 0 kann nicht sein, da der String definitiv mit "Location:" anfängt
						location = payload.substring(10, location_end -1);
						len = 0;
					}
					// else
					// wenn nicht gefunden, den Puffer nochmal auslesen und an die payload anhängen
				}
			}
		} else {
			Serial.println("Fehler bei der Ortsbestimmung: Anfragefehler httpCode=" +  String(httpCode));
			location = String("Fehler bei der Ortsbestimmung: Anfragefehler httpCode=" + String(httpCode));
		}

		http.end();
	}
  
	Serial.println(location);

	return location;
}

//
// WETTER, Temperatur holen
//
int GetTemperature(String city) {
	int httpCode;	
	int t = ERR_TEMP; // is a kind of error code!
	HTTPClient http;            //Declare object of class HTTPClient
	String weatherstring;
	String payload;
	String temp_temperature;


	// Den Ort holen, den wttr.in dem Wert "city" tatsächlich zuordnet. Es gibt mehrere "Neustadt"!!!
	//
	if (temperature_real_location == "" || temperature_real_location.startsWith("Fehler")) {
		temperature_real_location = GetTemperatureRealLocation(city);
	}

	if (WiFi.status() == WL_CONNECTED) {                    						//Check WiFi connection status
		weatherstring = "http://wttr.in/" + city + "?format=\%t";    //Specify request destination

		Serial.println(weatherstring);

		http.begin(weatherstring);

		httpCode = http.GET();

		if (httpCode >= HTTP_CODE_OK) {
			payload = http.getString();                  						//Get the response payload

			temp_temperature = payload.substring(0, payload.length() - 4); 	//Format is "+x°C\n" wobei ° zwei Bytes einnimmt!

			if ((temp_temperature.charAt(0) == '-') || (temp_temperature.charAt(0) == '+')) { 
				t = temp_temperature.toInt();
			}
		} else {
			Serial.print  ("Fehler bei der Temperaturabfrage: Anfragefehler httpCode=");   Serial.println(httpCode);
			temperature_real_location = String("Fehler bei der Temperaturabfrage: Anfragefehler httpCode=" + httpCode);
		}

		http.end();                                           						//Close connection

	}

	Serial.println(t);

	return t;
}


//
void showTemperature() {

	CRGB c;
	
	if (temperature == ERR_TEMP) {
		resetLEDs();

		setWord(W_GRAD, CRGB::Red);

		FastLED.show();
	}
	else {
		
		if (temperature < -39 || temperature > 39) return;

		resetLEDs();

		c = GetTemperatureColor(temperature);
		
		setWord(W_ES_IST, c);
		setNumber(temperature, c);
		setWord(W_GRAD, c);

		FastLED.show();
	}
}


void testTemperatur() {
	int i;
	CRGB c;
	
	for (i=-39; i<40; i++) {
		resetLEDs();

		c = GetTemperatureColor(i);
		
		setWord(W_ES_IST, c);
		setNumber(i, c);
		setWord(W_GRAD, c);

		FastLED.show();
		
		delay(500);
	}
}

#endif