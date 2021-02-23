#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WIFIConfigurator.h>
#include <EEPROM.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

char* mqttServer;
int mqttPort;
char* mqttUser;
char* mqttPassword;
char* mqttTopic;
unsigned int sleepSec;
unsigned int delayBeforeSleepMs = 300000;
unsigned long startTime;

const int sensorPin = 4;  //D2

char configData[500];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|Sleep Duration (s)";
bool powerCycled;

WIFIConfigurator configurator(configLabels);

void connectMQTT() {
  if (!client.connected()) {
    Serial.println(WiFi.status() == WL_CONNECTED);
    Serial.println("Connecting to MQTT...");
    if (client.connect("sumpsensor", mqttUser, mqttPassword )) {
      Serial.println("connected to MQTT server");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
    }
  }
}

void publish(char* payload) {
  connectMQTT();
  client.publish(mqttTopic, payload);
}


void setup() {
 
 Serial.begin(115200);
 
 EEPROM.begin(500);
 EEPROM.get(0, configData);
 
 delay(3000);
 Serial.println();
 configurator.begin();
 while (true) {
   char* ssid = strtok(configData, "|");
   if (ssid == NULL) break;
   char* wifipassword = strtok(NULL, "|");
   if (wifipassword == NULL) break;
   char* hostname = strtok(NULL, "|");
   if (hostname == NULL) break;
   mqttServer = strtok(NULL, "|");
   if (mqttServer == NULL) break;
   mqttPort = atoi(strtok(NULL, "|"));
   if (mqttPort == NULL) break;
   mqttUser = strtok(NULL, "|");
   if (mqttUser == NULL) break;
   mqttPassword = strtok(NULL, "|");
   if (mqttPassword == NULL) break;
   mqttTopic = strtok(NULL, "|");
   if (mqttTopic == NULL) break;
   sleepSec = atoi(strtok(NULL, "|"));
   if (sleepSec == NULL) break;
   
   //Config looks good, report sensor readimg...
   pinMode(sensorPin, INPUT);
   delay(500);
   client.setServer(mqttServer, mqttPort);
   if (digitalRead(sensorPin) == HIGH) {
     Serial.println("sensor: ON");
     publish("ON");
   } else {
     Serial.println("sensor: OFF");
     publish("OFF");
   }

   Serial.print("reset reason: ");
   Serial.println(ESP.getResetReason());

   if (ESP.getResetReason().equals("Power on")) {
     Serial.println("Restarted because of power cycle, starting in config mode.  You have 5 min to reconfigure via web");
     delayBeforeSleepMs = 300000;
   } else {
     Serial.println("Restarted because of reset from deep sleep, going back to sleep in 3 seconds");
     delayBeforeSleepMs = 3000;
   }
   startTime = millis();
   break;
 }
 
}

void loop() {
  configurator.handleClient();
  unsigned long now = millis();
  if (now - startTime > delayBeforeSleepMs || now < startTime) {
    Serial.print("sleeping for ");
    Serial.print(sleepSec);
    Serial.println(" seconds");    
    ESP.deepSleep(sleepSec * 1000000);
  }
} 
