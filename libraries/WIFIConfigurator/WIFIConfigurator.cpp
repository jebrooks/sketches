#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <WIFIConfigurator.h>
#define MAXDATASIZE 500
#define DNS_PORT 53

DNSServer dnsServer; 
ESP8266WebServer server(80);
String _labels;
char data[MAXDATASIZE];
IPAddress apIP(192,168,4,1);

WIFIConfigurator::WIFIConfigurator(String labels)
{
  _labels = labels;
}

void WIFIConfigurator::sendWifiForm() {
  String html = "<P>Configure device at MAC ";
  html.concat(WiFi.macAddress());
  html.concat("</P>");
  html.concat("<P>Enter your SSID & PASSWORD.  After submit, device will restart and attempt to join.</P><P>If it can join, you can continue configuration at that network (you will need to join it) at http://[ip assigned to device].</P><P>Otherwise the device will start up its own WiFi AP again and you can retry SSID/PASSWORD entry.</P>");
  html.concat("<FORM ACTION='wifiaction' METHOD='POST'>");
  html.concat("<LABEL FOR='ssid'>SSID: </LABEL><INPUT TYPE='text' ID='ssid' NAME='ssid'><BR>");
  html.concat("<LABEL FOR='password'>Password: </LABEL><INPUT TYPE='password' ID='password' NAME='password'><BR>");
  html.concat("<LABEL FOR='hostname'>Hostname: </LABEL><INPUT TYPE='text' ID='hostname' NAME='hostname'><BR>");
  html.concat("<INPUT TYPE='submit' VALUE='Submit'></FORM>");
  server.send(200, "text/html", html);
}

void WIFIConfigurator::sendConfigForm() {
  Serial.println("sendConfigForm()");
  String html = "<P>Configure Device</P>MAC: ";
  html.concat(WiFi.macAddress());
  html.concat("<BR>Hostname: ");
  html.concat(WiFi.hostname());
  html.concat("<BR>");

  //build the html form based on the supplied field labels
  int strLen = _labels.length() + 1;
  char labelArr[strLen];
  _labels.toCharArray(labelArr, strLen);
  html.concat("<P>Enter your device specific config.  Device will restart after submit.</P>");
  html.concat("<FORM ACTION='configaction' METHOD='POST'>");
  char* label;
  label = strtok(labelArr, "|");
  int i=0;
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

  //load the current config values from EEPROM and set the form field values with html script
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  char* value = strtok(data, "|"); //ssid
  value = strtok(NULL, "|"); //pword
  value = strtok(NULL, "|"); //hostname
  html.concat("<script>");
  value = strtok(NULL, "|"); //first config value
  i=0;
  while (value != NULL) {
    html.concat("document.getElementById('field");
    html.concat(i);
    html.concat("').value='");
    html.concat(value);
    html.concat("';");
    value = strtok(NULL,"|");
    i++;
  }
  html.concat("</script>");
  server.send(200, "text/html", html);
}

void WIFIConfigurator::handleRoot() {
  Serial.println("handleRoot()");
  if (WiFi.status() != WL_CONNECTED) {
    sendWifiForm();
  } else {
    sendConfigForm();
  }
}

void WIFIConfigurator::handleWifiChange() {
  Serial.println("handleWifiChange()");

  char newData[MAXDATASIZE];
 
  //write wifi info to eeprom and restart
  String ssidStr = server.arg("ssid");
  String passwordStr = server.arg("password");
  String hostnameStr = server.arg("hostname");
  int strLen = ssidStr.length() + 1;
  char newSsid[strLen];
  ssidStr.toCharArray(newSsid, strLen);
  strLen = passwordStr.length() + 1;
  char newPassword[strLen];
  passwordStr.toCharArray(newPassword, strLen);
  strLen = hostnameStr.length() + 1;
  char newHostname[strLen];
  hostnameStr.toCharArray(newHostname, strLen);
  
  //put new wifi info into data
  Serial.println("received connection info for SSID ");
  Serial.println(newSsid);
  strcpy(newData, newSsid);
  strcat(newData, "|");
  strcat(newData, newPassword);
  strcat(newData, "|");
  strcat(newData, newHostname);
  strcat(newData, "|");

  memset(data,0,MAXDATASIZE);
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  char* value = strtok(data, "|"); //old ssid
  value = strtok(NULL, "|"); //old pword
  value = strtok(NULL, "|"); //old hostname
  value = strtok(NULL, "|"); //first config val
 
  //put current config into data 
  while (value != NULL) {
    Serial.print("adding ");
    Serial.println(value);
    strcat(newData, value);
    strcat(newData, "|");
    value = strtok(NULL, "|");
  }

  //write data to eeprom - will only be changes to wifi info
  EEPROM.put(0,newData);
  EEPROM.commit();
  server.send(200, "text/html", "Restarting...");
  delay(1000);
  ESP.restart();
}

void WIFIConfigurator::handleConfigChange() {

  Serial.println("handleConfigChange()");
  char newData[MAXDATASIZE];
  memset(data,0,MAXDATASIZE);
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  char* myssid = strtok(data, "|");
  char* mypassword = strtok(NULL, "|");
  char* myhostname = strtok(NULL, "|");
  
  strcpy(newData, myssid);
  strcat(newData, "|");
  strcat(newData, mypassword);
  strcat(newData, "|");
  strcat(newData, myhostname);
  strcat(newData, "|");
  for (int i=0; i < server.args() - 1; i++) { //last arg is the full querystring
    String valStr = server.arg(i);
    int strLen = valStr.length() + 1;
    char val[strLen];
    valStr.toCharArray(val,strLen);
    Serial.print("writing field");
    Serial.print(i);
    Serial.print(" ");
    Serial.println(val);
    strcat(newData, val);
    strcat(newData, "|");
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
  char macAddr[strLen];
  macAddrStr.toCharArray(macAddr, strLen);
  char apName[17];
  strcpy(apName, "configurme-");
  strcat(apName, strtok(macAddr, ":"));
  strcat(apName, strtok(NULL, ":"));
  //AP SSID will be configurme-<first 4 letters of MAC addr>
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
  Serial.println(myData);
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
    while (WiFi.status() != WL_CONNECTED && i < 60) {
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
      Serial.println(WiFi.localIP());
    }
  }

  if (! configReady) {
    startAP();
  } 
  
  server.on("/", [&](){ handleRoot(); });
  server.on("/wifi", [&](){ sendWifiForm(); } );
  server.on("/wifiaction", [&](){ handleWifiChange(); } );
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
