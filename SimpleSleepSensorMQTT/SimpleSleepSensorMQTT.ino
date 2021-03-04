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

unsigned long sleepSec = 30;
unsigned int delayBeforeSleepMs = 300000;
unsigned long startTime;

const int sensorPin = 3;  //use RX for input pin

char configData[300];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|Sleep Duration (s)";
bool powerCycled;

WIFIConfigurator configurator(configLabels);

void connectMQTT() {
  if (!client.connected()) {
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
 delay(2000);
 Serial.begin(115200);
  
 configurator.begin();

 EEPROM.begin(300);
 EEPROM.get(0, configData);

 Serial.println("reading config from EEPROM...");
 while (true) {
   char* ssid = strtok(configData, "|");
   if (ssid == NULL) break;
   char* wifipassword = strtok(NULL, "|");
   if (wifipassword == NULL) break;
   char* myhostname = strtok(NULL, "|");
   if (myhostname == NULL) break;
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

   if (ESP.getResetReason().equals("Power on") || ESP.getResetReason().equals("External System") ) {
     Serial.println("External reset or power cycle, starting in config mode.  You have 5 min to reconfigure via web");
     delayBeforeSleepMs = 300000;
   } else {
     Serial.println("Started from scheduled deep sleep reset, going back to sleep in 2 seconds");
     delayBeforeSleepMs = 2000;
   }
   startTime = millis();
   break;
 }
 
}

void loop() {
  configurator.handleClient();
  unsigned long now = millis();
  if (now - startTime > delayBeforeSleepMs || now < startTime) {
    unsigned long sleepMicroS = sleepSec * 1000000;
    Serial.print("sleeping for ");
    Serial.print(sleepMicroS);
    Serial.println(" micro sec");    
    ESP.deepSleep(sleepMicroS);
    delay(10);
  }
} 
