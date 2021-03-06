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
char* mqttPayload;
int sensorPin;

char configData[300];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|MQTT Payload|Sensor GPIO";
bool state;

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
  Serial.print(mqttTopic);
  Serial.print(" ");
  Serial.println(payload);
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
   mqttPayload = strtok(NULL, "|");
   if (mqttPayload == NULL) break;
   sensorPin = atoi(strtok(NULL, "|"));
   if (sensorPin == NULL) break;

   //Config looks good, report sensor readimg...
   pinMode(sensorPin, INPUT);
   client.setServer(mqttServer, mqttPort);

   break;
 }
 
}

void loop() {
  configurator.handleClient();

  bool currState = digitalRead(sensorPin) == HIGH;
  
  if (currState != state && currState) {
    Serial.print("gpio ");
    Serial.print(sensorPin);
    Serial.println(" LOW->HIGH");
    publish(mqttPayload);
  }

  state = currState;

} 
