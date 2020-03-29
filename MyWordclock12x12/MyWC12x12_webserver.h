#define WEBSERVER_H
//#include "MyWC12x12_config.h"

#include <ESP8266WebServer.h>

#ifdef WEBSERVER_CPP
#else
extern void handleRootPath();
extern void handleClientServer();
extern void startServer(String);
#endif
