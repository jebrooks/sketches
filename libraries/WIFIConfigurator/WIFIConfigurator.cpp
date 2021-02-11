#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WIFIConfigurator.h>
#define MAXDATASIZE 500

ESP8266WebServer server(80);
String _labels;
char data[MAXDATASIZE];

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
  html.concat("<LABEL FOR='ssid'>SSID:</LABEL><INPUT TYPE='text' ID='ssid' NAME='ssid'><BR>");
  html.concat("<LABEL FOR='password'>Password:</LABEL><INPUT TYPE='password' ID='password' NAME='password'><BR>");
  html.concat("<INPUT TYPE='submit' VALUE='Submit'></FORM>");
  server.send(200, "text/html", html);
}

void WIFIConfigurator::sendConfigForm() {
  Serial.println("sendConfigForm()");
  String html = "<P>Configure device at MAC ";
  html.concat(WiFi.macAddress());
  html.concat("</P>");
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
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  char* value = strtok(data, "|"); //ssid
  value = strtok(NULL, "|"); //pword
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

void WIFIConfigurator::handleWifiSetup() {
  Serial.println("handleWifiSetup()");
  //write wifi info to eeprom and restart
  Serial.print("configured data size is: ");
  Serial.println(MAXDATASIZE);
  String ssidStr = server.arg("ssid");
  String passwordStr = server.arg("password");
  int strLen = ssidStr.length() + 1;
  char newSsid[strLen];
  ssidStr.toCharArray(newSsid, strLen);
  strLen = passwordStr.length() + 1;
  char newPassword[strLen];
  passwordStr.toCharArray(newPassword, strLen);
  
  Serial.println("received connection info for SSID ");
  Serial.println(newSsid);
  strcpy(data, newSsid);
  strcat(data, "|");
  strcat(data, newPassword);
  strcat(data, "|");

  Serial.println("writing: ");
  Serial.println(data);
  Serial.println("write data of length ");
  Serial.println(sizeof(data));
  delay(2000);
  EEPROM.put(0,data);
  EEPROM.commit();
  server.send(200, "text/html", "Restarting...");
  delay(1000);
  ESP.restart();
}

void WIFIConfigurator::handleConfigChange() {

  Serial.println("handleConfigChange()");
  char newData[MAXDATASIZE];
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, data);
  char* myssid = strtok(data, "|");
  char* mypassword = strtok(NULL, "|");
  
  Serial.print("writing ssid ");
  Serial.println(myssid);
  strcpy(newData, myssid);
  strcat(newData, "|");
  Serial.print("writing password ");
  Serial.println(mypassword);
  strcat(newData, mypassword);
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
  WiFi.softAP(apName);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP SSID: ");
  Serial.println(apName);
  Serial.print("AP IP address: "); //should be 192.168.4.1
  Serial.println(myIP);  
}

void WIFIConfigurator::begin() {
  
  char configData[MAXDATASIZE];
  EEPROM.begin(MAXDATASIZE);
  EEPROM.get(0, configData);

  char* myssid = strtok(configData, "|");
  char* mypassword = strtok(NULL, "|");
  bool configReady = myssid != NULL && mypassword != NULL;

  if (configReady) {
    Serial.print("connecting to ");
    Serial.println(myssid);
    WiFi.begin(myssid, mypassword);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 20) {
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
  server.on("/wifiaction", [&](){ handleWifiSetup(); } );
  server.on("/configaction", [&](){ handleConfigChange(); } );
  server.begin();
  Serial.println("HTTP server started");
}

void WIFIConfigurator::handleClient() {
  server.handleClient();
}
