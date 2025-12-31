/*
 * Sample code for connexion to minitel videotex server via websocket 
 * Requirements: ESP8266 connected to minitel DIN port and a WiFi connexion
 * 
 * created by iodeo - dec 2021 - modifié par guenego dec 2026
 * inspiré de
 * https://github.com/iodeo/Minitel-ESP32/blob/main/arduino/Minitel1B_Websocket_Client/Minitel1B_Websocket_Client.ino
 * https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp8266_pico/WebSocketClient/WebSocketClient.ino
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include <WebSocketsClient.h> // src: https://github.com/Links2004/arduinoWebSockets.git
#include <Minitel1B_Soft.h>   


// ---------------------------------------
// ------ Minitel port configuration

//#define MINITEL_BAUD 4800          // 1200 / 4800 / 9600 depending on minitel type / telic 1b = 4800
#define MINITEL_BAUD 1200          // 1200 / 4800 / 9600 depending on minitel type / telic 1 = 1200
#define MINITEL_DISABLE_ECHO true  // true if characters are repeated when typing

// ---------------------------------------
// ------ Debug port configuration

#define DEBUG true

#if DEBUG
  #define DEBUG_PORT Serial       // for esp8266
  #define DEBUG_BAUD 115200       // set serial monitor accordingly
  #define debugBegin(x)     DEBUG_PORT.begin(x)
  #define debugPrintf(...)    DEBUG_PORT.printf(__VA_ARGS__)
#else // Empty macro functions
  #define debugBegin(x)     
  #define debugPrintf(...)
#endif

// ---------------------------------------
// ------ WiFi credentials

const char* ssid     = "mySsid";        // your wifi network
const char* password = "myPassword";    // your wifi password

// ---------------------------------------
// ------ Websocket server

/****** local  -----------------  connecté le 31 dec 2025 */
// ws://192.168.1.20:8182/
char* host = "192.168.1.20";
int port = 8182;
char* path = "/"; 
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/

/******  MINIPAVI.FR  ---------  connecté le 31 dec 2025 * /
// ws://go.minipavi.fr:8182/
char* host = "go.minipavi.fr";
int port = 8182;
char* path = "/";
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/

/****** labbej  -----------------  connecté le 31 dec 2025 
// wss://minitel.labbej.fr:8182/
char* host = "minitel.labbej.fr";
int port = 8182;
char* path = "/"; 
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/

/****** 3615 code  -----------------  connecté le 31 dec 2025 
// wss://3615co.de/ws
char* host = "3615co.de";
int port = 80;
char* path = "/ws"; 
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/

/******  HACKER  --------------  connecté le 31 dec 2025 
// ws://mntl.joher.com:2018/?echo
// websocket payload length up to 873
char* host = "mntl.joher.com";
int port = 2018;
char* path = "/?echo";
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/

/****** AE  -------------------  connecté le 31 dec 2025 page annuaire seule ?
// ws://3611.re/ws
// websocket payload length of 0
char* host = "3611.re";
int port = 80;
char* path = "/ws";
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/


/****** SM  -------------------  connecté le 2 mar 2022  ;  le 31 dec 2025 : KO OFF
// wss://wss.3615.live:9991/?echo
// websocket payload length up to 128
char* host = "wss.3615.live";
int port = 9991;
char* path = "/?echo"; 
bool ssl = true;
int ping_ms = 0;
char* protocol = "";
/**/

/******  TELETEL.ORG  ---------  connecté le 2 mar 2022  ;  le 31 dec 2025 : KO OFF
// ws://home.teletel.org:9001/
char* host = "home.teletel.org";
int port = 9001;
char* path = "/";
bool ssl = false;
int ping_ms = 0;
char* protocol = "";
/**/


/****** TEASER  ---------------  connecté le 2 mar 2022  ;  le 31 dec 2025 : KO OFF
// ws://minitel.3614teaser.fr:8080/ws
char* host = "minitel.3614teaser.fr";
int port = 8080;
char* path = "/ws"; 
bool ssl = false;
int ping_ms = 10000;
char* protocol = "tty";
/**/


WiFiClient client;
WebSocketsClient webSocket;
//Minitel minitel(MINITEL_PORT); // Minitel1B_Hard
Minitel minitel(5, 16);  // RX, TX : D0/D1 sur Wemos D1 Mini // Minitel1B_Soft


void setup() {

  debugBegin(DEBUG_BAUD);
  debugPrintf("\n-----------------------\n");
  debugPrintf("\n> Debug port ready\n");

  // We initiate minitel communication
  debugPrintf("\n> Minitel setup\n");
  int baud = minitel.searchSpeed();
  if (baud != MINITEL_BAUD) baud = minitel.changeSpeed(MINITEL_BAUD);
  debugPrintf("  - Baud detected: %u\n", baud);
  if (MINITEL_DISABLE_ECHO) {
    minitel.echo(false);
    debugPrintf("  - Echo disabled\n");
  }

  // We connect to WiFi network
  debugPrintf("\n> Wifi setup\n");
  debugPrintf("  Connecting to %s ", ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debugPrintf(".");
  }
  debugPrintf("\n  WiFi connected with IP %s\n", WiFi.localIP().toString().c_str());

  // We connect to Websocket server
  debugPrintf("\n> Websocket connection\n");
	// server address, port and URL 
  webSocket.begin(host, port, path);
    
  webSocket.onEvent(webSocketEvent);

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);  
  
  if (ping_ms != 0) {
    debugPrintf("  - heartbeat ping added\n");
    // start heartbeat (optional)
    // ping server every ping_ms
    // expect pong from server within 3000 ms
    // consider connection disconnected if pong is not received 2 times
    webSocket.enableHeartbeat(ping_ms, 3000, 2);
  }

  debugPrintf("\n> End of setup\n\n");

}

void loop() {

  // Websocket -> Minitel
  webSocket.loop();

  // Minitel -> Websocket
  uint32_t key = minitel.getKeyCode(false);
  if (key != 0) {
    debugPrintf("[KB] got code: %X\n", key);
    // prepare data to send over websocket
    uint8_t payload[4];
    size_t len = 0;
    for (len = 0; key != 0 && len < 4; len++) {
      payload[3-len] = uint8_t(key);
      key = key >> 8;
    }
    webSocket.sendTXT(payload+4-len, len);
  }

}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t len) {
  switch(type) {
    case WStype_DISCONNECTED:
      debugPrintf("[WS] Disconnected!\n");
      break;
      
    case WStype_CONNECTED:
      debugPrintf("[WS] Connected to url: %s\n", payload);
      break;
      
    case WStype_TEXT:
      debugPrintf("[WS] got %u chars\n", len);
      if (len > 0) {
        debugPrintf("  >  %s\n", payload);
        for (size_t i = 0; i < len; i++) {
          minitel.writeByte(payload[i]);
        }
      }
      break;
      
    case WStype_BIN:
      debugPrintf("[WS] got %u binaries - ignored\n", len);
      break;
      
    case WStype_ERROR:
      debugPrintf("[WS] WStype_ERROR\n");
      break;
      
    case WStype_FRAGMENT_TEXT_START:
      debugPrintf("[WS] WStype_FRAGMENT_TEXT_START\n");
      break;
      
    case WStype_FRAGMENT_BIN_START:
      debugPrintf("[WS] WStype_FRAGMENT_BIN_START\n");
      break;
      
    case WStype_FRAGMENT:
      debugPrintf("[WS] WStype_FRAGMENT\n");
      break;
      
    case WStype_FRAGMENT_FIN:
      debugPrintf("[WS] WStype_FRAGMENT_FIN\n");
      break;
  }
}
