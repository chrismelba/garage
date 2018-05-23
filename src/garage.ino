#include <Hash.h>
#include <Arduino.h>
#include <WebSocketsServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ntp.h>
#include <Time.h>
#include <TimeLib.h>
#include "FS.h"

#define DBG_OUTPUT_PORT Serial
#define doorpin 5
#define sensorpin 13 //1 is closed, 0 is open
#define sensordrivepin 14
const char* ssid = "ASIO Secret Base";
const char* password = "pq2macquarie";
const char* host = "garagedoor";
IPAddress localip(192, 168, 1, 201);
IPAddress gateway(192, 168, 1, 1);
IPAddress DNSip(8, 8, 8, 8);
int val;

int wifiStatusOld;

time_t getNTPtime(void);
NTP NTPclient;
Ticker NTPsyncclock;
String macString;
String ipString;
String netmaskString;
String gatewayString;

Ticker doorticker;
Ticker closeticker;
// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t D9   = 3;
// static const uint8_t D10  = 1;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(2655);
//holds the current upload

void setup(void){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);



  pinMode(doorpin, OUTPUT);
  pinMode(sensordrivepin, OUTPUT);
  pinMode(sensorpin, INPUT_PULLUP);

  digitalWrite(sensordrivepin, LOW);
  digitalWrite(doorpin, LOW);

  ArduinoOTA.onStart([]() {

    Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  MDNS.begin(host);




  // //WIFI INIT
  // DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  // if (String(WiFi.SSID()) != String(ssid)) {
  //   WiFi.begin(ssid, password);
  // }
  //
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(50);
  //   DBG_OUTPUT_PORT.print(".");
  // }
  //
  // WiFi.config(localip, gateway, DNSip);
  // DBG_OUTPUT_PORT.println("");
  // DBG_OUTPUT_PORT.print("Connected! IP address: ");
  // DBG_OUTPUT_PORT.println(WiFi.localIP());
  //
  // MDNS.begin(host);
  // DBG_OUTPUT_PORT.print("Open http://");
  // DBG_OUTPUT_PORT.print(host);
  // DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
  WiFiManager wifiManager;
  wifiManager.setSTAStaticIPConfig(IPAddress(192,168,1,201), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  wifiManager.autoConnect("garage door");


  //set time up
  NTPclient.begin("2.au.pool.ntp.org", 10);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);

  //set SPIFFS and log boot time
  SPIFFS.begin();

  time_t t = now();



  // File f = SPIFFS.open("/f.txt", "a");
  //
  // f.println();
  // f.print("Log:");
  // f.print(" ");
  // f.print(day());
  // f.print(" ");
  // f.print(month(t));
  // f.print(" ");
  // f.print(year());
  // f.print(" ");
  // f.print(hour());
  // f.print(":");
  // f.print(minute());
  // f.print(":");
  // f.print(second());
  // f.print(":");
  // f.print(" Booted up");
  // f.close();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/opendoor", webHandleOpenDoor);
  server.on("/closedoor", webHandleCloseDoor);
  server.on("/toggledoor", webHandleToggleDoor);

  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });


  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
  wifiStatusOld = WiFi.status();

}


void loop(void){

// val = digitalRead(sensorpin);
// Serial.println(val);
//
// if (val == HIGH) {
// digitalWrite(doorpin, HIGH);
// }
// else {
// digitalWrite(doorpin, LOW);
// }
  // if (wifiStatusOld != WiFi.status()) {
  //   time_t t = now();
  //
  //
  //   File f = SPIFFS.open("/f.txt", "a");
  //
  //   f.println();
  //   f.print("Log:");
  //   f.print(" ");
  //   f.print(day());
  //   f.print(" ");
  //   f.print(month(t));
  //   f.print(" ");
  //   f.print(year());
  //   f.print(" ");
  //   f.print(hour());
  //   f.print(":");
  //   f.print(minute());
  //   f.print(":");
  //   f.print(second());
  //   f.print(":");
  //   f.print(" Wifi Status Changed: ");
  //   f.print(WiFi.status());
  //
  //   f.close();
  //
  //   wifiStatusOld = WiFi.status();
  //
  //
  // }
  server.handleClient();
  webSocket.loop();
  ArduinoOTA.handle();
}
void openDoor(){

  if (digitalRead(sensorpin)==1) {
    digitalWrite(doorpin, HIGH);
    doorticker.attach_ms(1500, stopPin);
    closeticker.attach(3000, closeDoor);
  }

  DBG_OUTPUT_PORT.println("opendoor connection");
  DBG_OUTPUT_PORT.println(server.arg(1));
}

void closeDoor(){
  if (digitalRead(sensorpin)==0) {
    digitalWrite(doorpin, HIGH);
    doorticker.attach_ms(1500, stopPin);
  }
  closeticker.detach();
}

void toggleDoor(){
  digitalWrite(doorpin, HIGH);
  doorticker.attach_ms(1500, stopPin);
}

void webHandleOpenDoor(){
  server.send(200, "text/html", "<HTML><a href='http://192.168.1.201/opendoor'>opening garage</a>");
  openDoor();

}

void webHandleCloseDoor(){
  server.send(200, "text/html", "<HTML><a href='http://192.168.1.201/opendoor'>closing garage</a>");
  closeDoor();

}

void webHandleToggleDoor(){

  //server.send(200, "text/html", "<HTML><a href='http://192.168.1.201/opendoor'>toggling garage</a>");
  toggleDoor();
  handleFileRead("index");

}
void webHandleRoot(){
  server.send(200, "text/html", "<HTML><title>Tricket Config</title><form>SSID:<select name='ssid'>");
  DBG_OUTPUT_PORT.println("root connection");
}

void webHandleDebug(){
  File f = SPIFFS.open("/f.txt","r");
  String line = f.readString();
  server.send(404, "text/plain", "Garage Door Sensor: " + String(digitalRead(sensorpin)) + "\nTime: "+day()+" "+month()+" "+hour()+":"+minute()+":"+second()+line);
  f.close();



}
String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/html";
}

bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
      //if(contentType="text/html")
      //  path += ".htm";

    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void stopPin(){
  digitalWrite(doorpin, LOW);
  doorticker.detach();
}

time_t getNTPtime(void)
{
  time_t newtime;
  newtime = NTPclient.getNtpTime();
  for (int i = 0; i < 5; i++) {
    if (newtime == 0) {
      Serial.println("Failed NTP Attempt" + i);
      delay(2000);
      newtime = NTPclient.getNtpTime();
    }
  }

  return newtime;
}


//-------------------------------websocket handling----------------------------------------------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

 switch (type) {
   case WStype_DISCONNECTED:
     Serial.printf("[%u] Disconnected!\n", num);
     break;
   case WStype_CONNECTED: {
       IPAddress ip = webSocket.remoteIP(num);
       Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

       // send message to client
       webSocket.sendTXT(num, "Connected");
     }
     break;
   case WStype_TEXT:
      String head = wsHead((char*)payload);
      if (head == "toggledoor"){
        toggleDoor();
      }


    //  String value = wsValue((char*)payload);
    //  String head = wsHead((char*)payload);
     //
    //  if(head=="rcon"){
    //    Serial.print("RCON");
    //    Serial.println(value);
    //  }
    //  if(head=="rcoff"){
    //    Serial.print("RCOFF");
    //    Serial.println(value);
    //  }


     break;
 }

}
String wsHead(String input){
  int headend = find_text("|", input);
  return input.substring(0,headend);
}

String wsValue(String input){
  int valuestart = find_text("|", input)+1;
  return input.substring(valuestart);
}

int find_text(String needle, String haystack) {
  int foundpos = -1;
  for (int i = 0; i <= haystack.length() - needle.length(); i++) {
    if (haystack.substring(i,needle.length()+i) == needle) {
      foundpos = i;
    }
  }
  return foundpos;
}
