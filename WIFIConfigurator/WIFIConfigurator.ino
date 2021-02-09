#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

bool configReady=true;

WIFIConfigurator::WIFIConfigurator(String labels[], int eePromSize)
{
  _labels = labels;
  _eePromSize = eePromSize; 
}


void handleRoot() {
  String html = "<P>Configure device at MAC ");
  html.concat(WiFi.macAddress());
  html.concat("</P>");

  if (! configReady) {
    html.concat("<P>Enter your SSID & PASSWORD.  After submit, device will restart and attempt to join.</P><P>If it can join, you can continue configuration at that network (you will need to join it) at http://[ip assigned to device].</P><P>Otherwise the device will start up its own WiFi AP again and you can retry SSID/PASSWORD entry.</P>");
    html.concat("<FORM ACTION='wifi' METHOD='POST'>");
    html.concat("<LABEL FOR='ssid'>SSID:</LABEL><INPUT TYPE='text' ID='ssid' NAME='ssid'><BR>");
    html.concat("<LABEL FOR='password'>Password:</LABEL><INPUT TYPE='password' ID='password' NAME='password'><BR>");
  } else {
    html.concat("<P>Enter your device specific config.  Device will restart after submit.</P>");
    html.concat("<FORM ACTION='config' METHOD='POST'>");
    for (int i=0; i < _labels.size(); i++) {
      html.concat("<LABEL FOR='field");
      html.concat(i);
      html.concat("'>");
      html.concat(_labels[i]);
      html.concat(": </LABEL><INPUT TYPE='text' ID='field");
      html.concat(i);
      html.concat("' NAME='field");
      html.concat(i);
      html.concat("'><BR>");
    }
  }
  html.concat("<INPUT TYPE='submit' VALUE='Submit'></FORM>");
  server.send(200, "text/html", html);
}

void handleWifiSetup() {
  Serial.println("handleWifiSetup()");
  //write wifi info to eeprom and restart
  char data[100];
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

  Serial.println("writing: ");
  Serial.println(data);
  Serial.println("write data of length ");
  Serial.println(sizeof(data));
  delay(2000);
  EEPROM.put(0,data);
  EEPROM.commit();
  server.send(200, "text/html", "Attempting to connect...");
  ESP.restart();
}

void handleConfigChange() {

  Serial.println("handleConfigChange()");
  char data[EEPROM_DATA_SIZE];
  char newData[EEPROM_DATA_SIZE];
  EEPROM.begin(EEPROM_DATA_SIZE);
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
  for (int i=0; i < NUM_CONFIG_FIELDS; i++) {
    strcat(newData, "|");
    String valStr = server.arg(i);
    int strLen = valStr.length() + 1;
    char val[strLen];
    valStr.toCharArray(val,strLen);
    Serial.print("writing field");
    Serial.print(i);
    Serial.print(" ");
    Serial.println(val);
    strcat(newData, val);
  }
  EEPROM.put(0,newData);
  EEPROM.commit();
  server.send(200, "text/html", "Restarting...");
  ESP.restart();
}


void startAP() {
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

char[] WIFIConfigurator::get() {
  
  char configData[_eePromSize];
  EEPROM.begin(_eePromSize);
  EEPROM.get(0, configData);
  bool configReady = sizeof(configData) > 4;
  Serial.print("size of data: ");
  Serial.println(sizeof(configData));
  Serial.println(configData);

  if (configReady) {
    //char* myssid = strtok(configData, "|");
    char* myssid = "badsid";
    char* mypassword = strtok(NULL, "|");

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
  
  server.on("/", handleRoot);
  server.on("/wifi", handleWifiSetup);
  server.on("/config", handleConfigChange);
  server.begin();
  
  Serial.println("HTTP server started");
  return configData;
}
