#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <WIFIConfigurator.h>
#define MAXDATASIZE 300
#define DNS_PORT 53

DNSServer dnsServer; 
ESP8266WebServer server(80);
String _labels;
IPAddress apIP(192,168,4,1);

WIFIConfigurator::WIFIConfigurator(String labels)
{
  _labels = labels;
}

void WIFIConfigurator::sendConfigForm() {
  Serial.println("sendConfigForm()");
  String html = "<P>Configure Device</P>MAC: ";
  html.concat(WiFi.macAddress());
  html.concat("<BR>");
  if (WiFi.status() == WL_CONNECTED) {
    long wifiSig = WiFi.RSSI();
    String sigStr = String(wifiSig);
    html.concat("WiFi signal: ");
    html.concat(sigStr);
    html.concat(" dBm<BR>");
  }
  //build the html form based on the supplied field labels
  int strLen = _labels.length() + 1;
  char labelArr[strLen];
  _labels.toCharArray(labelArr, strLen);
  html.concat("<HTML><BODY><P>Enter your device specific config.  Device will restart after submit.</P>");
  html.concat("<FORM ACTION='configaction' METHOD='POST'>");
  html.concat("<LABEL FOR='field0'>SSID: </LABEL><INPUT TYPE='text' ID='field0' NAME='field0'><BR>");
  html.concat("<LABEL FOR='field1'>Password: </LABEL><INPUT TYPE='password' ID='field1' NAME='field1'><BR>");
  html.concat("<LABEL FOR='field2'>Hostname: </LABEL><INPUT TYPE='text' ID='field2' NAME='field2'><BR>");
  char* label;
  label = strtok(labelArr, "|");
  int i=3;
  while (label != NULL) {
    html.concat("<LABEL FOR='field");
    html.concat(i);
    html.concat("'>");
    html.concat(label);
    html.concat(": </LABEL><INPUT TYPE='text' ID='field");
    html.concat(i);
    html.concat("' NAME='field");
    html.concat(i);
    html.concat("'><BR>");
    label = strtok(NULL, "|");
    i++;
  }
  html.concat("<INPUT TYPE='submit' VALUE='Submit'></FORM>");

  char data[MAXDATASIZE];
  //load the current config values from EEPROM and set the form field values with html script
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  if (data[0] != 255) { //assume eeprom is empty if first char is 255 
    char* value = strtok(data, "|"); //first config value
    i=0;
    html.concat("<script>");
    while (value != NULL) {
      if (i != 1) { //skip password population
        html.concat("document.getElementById('field");
        html.concat(i);
        html.concat("').value='");
        html.concat(value);
        html.concat("';");
      }
      value = strtok(NULL,"|");
      i++;
    }
    html.concat("</script>");
  }
  html.concat("</BODY></HTML>");
  server.send(200, "text/html", html);
}

void WIFIConfigurator::handleRoot() {
  Serial.println("handleRoot()");
  sendConfigForm();
}

void WIFIConfigurator::handleConfigChange() {

  Serial.println("handleConfigChange()");
  char data[MAXDATASIZE];
  char newData[MAXDATASIZE];
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  strtok(data, "|");
  char* mypassword = strtok(NULL, "|"); //pword is second item on EEPROM
  strcpy(newData,"|");
  for (int i=0; i < server.args(); i++) { //ignore last arg which is the full querystring
    String argName = server.argName(i);
    if (argName.startsWith("field")) {
      String valStr = server.arg(i);
      int strLen = valStr.length() + 1;
      char val[strLen];
      valStr.toCharArray(val,strLen);
      if (i == 1 && strLen == 1) {
        strcat(newData, mypassword); //if no wifi password entered, use existing one
      } else {
        strcat(newData, val);
      }
      strcat(newData, "|");
    }
  }
  Serial.println("writing:");
  Serial.println(newData);
  EEPROM.put(0,newData);
  EEPROM.commit();
  server.send(200, "text/html", "Restarting...");
  delay(1000);
  ESP.restart();
}

void WIFIConfigurator::startAP() {
  Serial.println("Configuring access point...");
  String macAddrStr = WiFi.macAddress();
  int strLen = macAddrStr.length() + 1;
  IPAddress apIP(192,168,4,1);
  char macAddr[strLen];
  macAddrStr.toCharArray(macAddr, strLen);
  char apName[17];
  strcpy(apName, "jimdevice-");
  strcat(apName, strtok(macAddr, ":"));
  strcat(apName, strtok(NULL, ":"));
  //AP SSID will be jimdevice-<first 4 letters of MAC addr>
  WiFi.softAPConfig(apIP,apIP,IPAddress(255,255,255,0));
  WiFi.softAP(apName);
  dnsServer.start(DNS_PORT,"*",apIP); //force browser to config page
  Serial.print("AP SSID: ");
  Serial.println(apName);
  Serial.println("AP IP address: 192.168.4.1"); 
}

void WIFIConfigurator::begin() {
  
  char myData[MAXDATASIZE];
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, myData);
  char* myssid = strtok(myData, "|");
  char* mypassword = strtok(NULL, "|");
  char* myhostname = strtok(NULL, "|");
  bool configReady = myssid != NULL && mypassword != NULL && myhostname != NULL;
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  if (configReady) {
    Serial.print("connecting to ");
    Serial.println(myssid);
    WiFi.softAPdisconnect(true);
    WiFi.hostname(myhostname);
    WiFi.begin(myssid, mypassword);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 40) {
      delay(500);
      Serial.print(".");
      i++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println();
      Serial.print("Could not connect to WIFI ");
      Serial.println(myssid);
      configReady = false;
    } else {
      Serial.println("connected.");
      Serial.print("local IP: ");
      Serial.println(WiFi.localIP());
    }
  }

  if (! configReady) {
    startAP();
  }
  
  server.on("/", [&](){ handleRoot(); });
  server.on("/configaction", [&](){ handleConfigChange(); } );
  server.onNotFound( [&]() { handleRoot(); } );
  server.begin();
  Serial.println("HTTP server started");
}

void WIFIConfigurator::handleClient() {
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
}
