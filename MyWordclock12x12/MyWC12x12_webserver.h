#define WEBSERVER_H
//#include "MyWC12x12_config.h"

#include <ESP8266WebServer.h>

#ifdef WEBSERVER_CPP
//
// Webserver für die Konfiguration
ESP8266WebServer server(80);

#else
	
extern ESP8266WebServer server;

extern void handleRootPath();
extern void handleClientServer();
extern void startServer();
#endif
