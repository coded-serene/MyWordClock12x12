#define TEMPERATUR_CPP
#include "MyWC12x12_config.h"

#ifdef TEMPERATURE

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>                            // http://fastled.io      https://github.com/FastLED/FastLED

#include "MyWC12x12_temperatur.h"
#include "MyWC12x12_maske.h"
#include "MyWC12x12.h"

//
// globale Variable
//
int     mywc_g_temperature 				 = ERR_TEMP; // temperatur
String  mywc_g_temperature_real_location;		     // von wttr.in tatsächlich genutzter Ort, zur Ausgabe auf der Webseite

String  mywc_g_debug_temperature;

// Modulvariable
int     l_last_valid_temperature 		= ERR_TEMP; // Wert der letzten erfolgreichen Temperaturabfrage
int     l_temperature_error_count       = 1;


// Farbabstufung der Temperaturanzeige
//
CRGB GetTemperatureColor(int t) {
  // -39 bis -5	blau
  // -4  bis 5	hellblau
  // 6   bis 15   gruen
  // 16  bis 20   hellgruen
  // 21  bis 25   gelb
  // 26  bis 30   orange
  // 31  bis 39   rot
  if (t<= -40) {
    return CRGB::White;
  }
  else if (t<=-5) {
    return CRGB::Blue;
  }
  else if (t<=5) {
    return CRGB::LightSkyBlue;
  }
  else if (t<=15) {
    return CRGB::SeaGreen;
  }
  else if (t<=20) {
    return CRGB::Green;
  }
  else if (t<=25) {
    return CRGB::Yellow;
  }
  else if (t<=30) {
    return CRGB::Orange;
  }
  else if (t<=39) {
    return CRGB::Red;
  }
  else {
    return CRGB::White;
  }
}

//
void showTemperature(int t) {

    int anz_leds;
    CRGB c;

    resetLEDs();
    anz_leds = 0;

    if (t == ERR_TEMP) {
        // die Temperatur konnte nicht ermittelt werden
        mywc_g_debug_temperature = mywc_g_debug_temperature + "<hr>Fehlerhafte Versuche:" + String(l_temperature_error_count);
        if (mywc_g_debug_temperature.length() > 1024)
            mywc_g_debug_temperature = mywc_g_debug_temperature.substring(mywc_g_debug_temperature.length()  - 1024);

        anz_leds = l_temperature_error_count;	// pro Versuch (=Minute) eine LED

        if (anz_leds < 0) {
            anz_leds = 0;
        }
        else if (anz_leds>4) {
            anz_leds = 4;
        }

        if	(   l_temperature_error_count< 2*TEMPERATURE_REFETCH_MINUTES ) {
            setWord(wordsindex_single_m[anz_leds], CRGB::Red);
            t = l_last_valid_temperature;
        }
    }

    if (t != ERR_TEMP) {
        // es ist eine gültige Temperatur ermittelt worden, oder die temperatur ist auf die letzte Gültige gesetzt worden
        if (t < -39 ) {
            // die aber nicht angezeigt werden kann (zu klein)
            t=-39;
            setWord(W_VIER_PUNKTE, CRGB::Yellow);
        }

        if (t > 39) {
            // die aber nicht angezeigt werden kann (zu groß)
            t= 39;
            setWord(W_VIER_PUNKTE, CRGB::Yellow);
        }

        c = GetTemperatureColor(t);

        setWord(W_ES_IST, c);
        setNumber(t, c);
        setWord(W_GRAD, c);
    }
    else {
        setWord(W_GRAD, CRGB::Red);
    }

    FastLED.show();
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

  if (WiFi.status() == WL_CONNECTED) {                						//Check WiFi connection status
    weatherstring = "http://wttr.in/" + city + "?1&lang=en";    //Specify request destination

    Serial.println(weatherstring);

    // Die HTTP-Antwort ist zu groß zum Lesen-In-Einem-Stück >25kB also stückweise lesen und gleich suchen nach Location: ... [

    http.begin(client, weatherstring);
    httpCode = http.GET();

    if (httpCode >= HTTP_CODE_OK) {

      payload="";

      // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      location = "Ort war nicht in der Anfrageantwort enthalten. Antwortl&auml;nge=" + String(len);

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

        payload += String(buff); //Serial.println("Lese. Payload=" + payload);

        if (len > 0) {
          len -= c;
        }

        if (location_start == -1) { // Noch keinen Anfang (=="Location: ") gefunden
          location_start = payload.indexOf("Location: ");

          if (location_start >=0) {
            // aber in diesem frisch gelesenen Puffer
            payload = payload.substring(location_start);
            Serial.println("Los gehts. Payload=" + payload);
          }
          else {
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
    }
    else {
      Serial.println("ERROR getting TEMPERATURE Location: httpCode=" +  String(httpCode));
      location = GetDatumZeitString() + String(" Fehler bei der Ortsbestimmung: Anfragefehler " + String(httpCode));
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
    int temperature;
    HTTPClient http;            //Declare object of class HTTPClient
    String weatherstring;
    String payload;
    String temp_temperature;
    String errorstring;

    temperature = ERR_TEMP;     // is a kind of error code!
    errorstring = "";           // kein Fehlertext

    if (WiFi.status() == WL_CONNECTED) {                            //Check WiFi connection status
        weatherstring = "http://wttr.in/" + city + "?format=\%t";   //Specify request destination

        Serial.println(weatherstring);

        http.begin(weatherstring);
        httpCode = http.GET();

        // HTTP client errors
        // HTTPC_ERROR_CONNECTION_REFUSED  (-1)
        // HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
        // HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
        // HTTPC_ERROR_NOT_CONNECTED       (-4)
        // HTTPC_ERROR_CONNECTION_LOST     (-5)
        // HTTPC_ERROR_NO_STREAM           (-6)
        // HTTPC_ERROR_NO_HTTP_SERVER      (-7)
        // HTTPC_ERROR_TOO_LESS_RAM        (-8)
        // HTTPC_ERROR_ENCODING            (-9)
        // HTTPC_ERROR_STREAM_WRITE        (-10)
        // HTTPC_ERROR_READ_TIMEOUT        (-11)

        if (httpCode >= HTTP_CODE_OK) {
            payload = http.getString();                  						//Get the response payload

            temp_temperature = payload.substring(0, payload.length() - 4); 	//Format is "+x°C\n" wobei ° zwei Bytes einnimmt!

            if ((temp_temperature.charAt(0) == '-') || (temp_temperature.charAt(0) == '+')) {

                temperature = temp_temperature.toInt();

                // es gibt eine gültige Temperatur
                l_last_valid_temperature        = temperature;
                // l_last_valid_temperature_minute = g_minute;
                // l_last_valid_temperature_hour   = g_hour;
                errorstring = errorstring + "<br>\n" + GetDatumZeitString() + String(" ") + String(temperature) + String("&deg;C");
            }
        }
        else {
            Serial.print  ("ERROR getting TEMPERATURE: httpCode=");   Serial.println(httpCode);
            errorstring = GetDatumZeitString() + String(" Fehler bei der Temperaturabfrage: Anfragefehler " + String(httpCode));
        }

        http.end();                                           						//Close connection
    }
    else {
        errorstring = GetDatumZeitString() + String(" Keine WLAN-Verbindung");
    }

    mywc_g_temperature_real_location = GetTemperatureRealLocation(city);

    if (errorstring != "") {
        mywc_g_temperature_real_location = errorstring + String("<br>") + mywc_g_temperature_real_location;
    }

    mywc_g_debug_temperature = mywc_g_debug_temperature + "<hr>" + mywc_g_temperature_real_location;
    if (mywc_g_debug_temperature.length() > 1024)
        mywc_g_debug_temperature = mywc_g_debug_temperature.substring(mywc_g_debug_temperature.length()  - 1024);

    if (temperature == ERR_TEMP) {
        l_temperature_error_count++;
    }
    else {
        l_temperature_error_count = 0;
    }

    return temperature;
}

void testTemperatur() {
  for (int i=-39; i<40; i++) {
      showTemperature(i);
      delay(500);
  }
}
#endif
