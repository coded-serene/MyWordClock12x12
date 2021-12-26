
#include <Arduino.h>
#include <MyWC12x12.h>
#include <ArduinoOTA.h>         // OTA library
#include "main.h"

void mqttReceiveCallback(char* topic, byte* payload, unsigned int length); 
bool mqttReconnect(int maxTrails=10);
void mqttMaintain(void);
void wifiReconnect(void);
void setupMQTT(void);


String runtimeBannerHintText;
String runtimeBannerText;

WiFiClient espClient;
PubSubClient m_mqttClient(espClient);
bool mqttEnabled = true;

TetrisGame m_tetrisGame;
bool clockModeActive = true;
bool wishClockMode = false;
bool firstStartup=true;
volatile int autoClockActivateTime=0;

void setup(void) 
{
	Serial.begin(115200);
	SetupMyWc();

	// mqtt konfigurieren
	setupMQTT();

	m_tetrisGame.InitTetris();
}
void loop(void) 
{
	ArduinoOTA.handle();
	handleClientServer();
	mqttMaintain();

	if((clockModeActive==false) && (millis() > autoClockActivateTime))
	{
		Serial.println("Switch Back to ClockMode due to Tetris idle.");
		Serial.print("currentTime2=");Serial.println(millis());
		Serial.print("autoClockActivateTime2=");Serial.println(autoClockActivateTime);
		
		autoClockActivateTime=millis()+60000;
		wishClockMode=true;
	}
	if(clockModeActive==false && wishClockMode==true)
	{
		clockModeActive=true;
	}
	if(clockModeActive)
	{
		// wir sind im Clock-Modus
		LoopMyWc();
		wishClockMode=false;
	}
	else
	{
		// wir sind im Tetris-Modus
		//Serial.println("TetrisGame.");
		m_tetrisGame.MaintainTetris(leds/*from myWC12x12.cpp*/);
	}
}

void setupMQTT(void)
{
	Serial.println("*** Setup MQTT....");
	m_mqttClient.setServer(CONFIG.mqttServerName.c_str(), CONFIG.mqttPort);
	m_mqttClient.setCallback(mqttReceiveCallback);
	if(mqttReconnect(-1) == false)
	{
		// der erste Aufruf hat schon nicht geklappt. -> ab jetzt kein mqtt mehr!
		mqttEnabled=false;
		Serial.println("*** Setup MQTT failed. Disabled.");
	}
	else
	{
		Serial.println("*** Setup MQTT succeeded.");
	}
}
void handleTetrisCommand(gameCommand cmd)
{
	userAction action = userAction::None;
	autoClockActivateTime=millis()+60000;
	switch(cmd)
	{
		case gameCommand::GameCommandStartStop: //clockModeActive=true;
					clockModeActive = !clockModeActive;
					wishClockMode = clockModeActive;

					if(clockModeActive==true)
					{
						// wir sind derzeit im Uhr-Modus --> wechsel zum Tetris-Modus
						m_tetrisGame.InitTetris();
						Serial.println("Tetris initialized");
					}
					else
					{
						Serial.println("*********** EXIT TETRIS. Back to Clock!");
					}
					break;

		case gameCommand::GameCommandLeft: action = userAction::MoveLeft; break;
		case gameCommand::GameCommandRight: action = userAction::MoveRight; break;
		case gameCommand::GameCommandDown: action = userAction::MoveDown; break;
		case gameCommand::GameCommandFire: action = userAction::Fire; break;
		
		default: break;
	}
	if(action != userAction::None)
	{
		m_tetrisGame.OnUserAction(leds,action);
	}
}
void mqttReceiveCallback(char* topic, byte* payload, unsigned int length) 
{
	if(payload == 0x00 || topic == 0x00)
    {
      return;
    }
    
    payload[length] = '\0';
    String topicString = String(topic);
    Serial.println("");
    Serial.print("[MQTT-main] Message arrived. Topic: "); Serial.print(topicString.c_str()); Serial.print(" - payload length: "); Serial.println(length);
    if(firstStartup)
    {
      firstStartup=false;
      Serial.println("Message ignored due to first startup.");
      return;
    }

	// extract payload
	Serial.print(F("Payload (string): "));
	Serial.println((char*)payload);
	gameCommand cmd = gameCommand::GameCommandNone; 
	switch(payload[0])
	{
		case 'r': cmd = gameCommand::GameCommandStartStop; break;
		case 'a': cmd = gameCommand::GameCommandLeft;  break;
		case 's': cmd = gameCommand::GameCommandRight; break;
		case 'y': cmd = gameCommand::GameCommandDown; break;
		case ' ': cmd = gameCommand::GameCommandFire; break;
		
		default: break;
	}
	handleTetrisCommand(cmd);
}



void wifiReconnect(void)
{

}
void mqttMaintain(void)
{
	if (!m_mqttClient.connected()) 
    {
        mqttReconnect();
    }
    m_mqttClient.loop();
}

bool mqttReconnect(int maxTrails/*=10*/) 
{
	if(!mqttEnabled)
		return false;

  	// Loop until we're reconnected
  	unsigned long nextTrial = 0;
	int trailCount=0;
	bool succeeded = false;

	do
	{
		if(m_mqttClient.connected())
		{
			return true;
		}

		if(millis() < nextTrial)
			continue;
			
		if(WiFi.status() != WL_CONNECTED)
		{
			// Es gibt keine Wifi-Connection mehr....
			wifiReconnect();
		}

		Serial.println("Attempting MQTT connection: ");
		String clientId = "espMqtt";
		clientId += CONFIG.networkHostname;
		clientId += String(random(0xffff), HEX);
		
		// Attempt to connect
		Serial.print("ClientID: "+clientId);
		Serial.print(" - ");
		if (m_mqttClient.connect(clientId.c_str(),CONFIG.mqttUserName.c_str(),CONFIG.mqttPassword.c_str())) 
		{
			// Once connected, publish an announcement...
			String iotDeviceBaseTopic(CONFIG.networkHostname);
			String startupTopic = iotDeviceBaseTopic+("/StartupClient");
			String mqttTopic2Subscribe=iotDeviceBaseTopic+"/Config/#";

			String welcomeMessage("ClientID: ");
			welcomeMessage += clientId+"\n\rSubribe2Message: "+mqttTopic2Subscribe;
			m_mqttClient.publish(startupTopic.c_str(), welcomeMessage.c_str());

			
			if(iotDeviceBaseTopic != NULL)
			{
				Serial.print("- Subscribed Topic: ");
				Serial.print(mqttTopic2Subscribe);
				m_mqttClient.subscribe(mqttTopic2Subscribe.c_str());
			}
			Serial.println("- MQTT connected.");
			return true;
		} 
		else 
		{
			Serial.print("failed, rc=");
			Serial.print(m_mqttClient.state());
			Serial.println(" try again in 3 seconds");
			// Wait 5 seconds before retrying
			nextTrial = millis()+3000;
			trailCount++;
		}
  	}
	while (trailCount < maxTrails);

    return false;
}
