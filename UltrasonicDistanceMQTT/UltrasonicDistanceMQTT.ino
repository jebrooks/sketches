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
unsigned int readInterval = 3000;
unsigned int reportInterval = 10000;
int minNormalDistance;

const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

unsigned int lastRead = 0;
unsigned int lastReported = 0;

char configData[500];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|Sensor Read Interval (ms)|Sensor Report Interval (ms)|Minimum Normal Distance (cm)";

WIFIConfigurator configurator(configLabels);

void connectMQTT() {
  if (!client.connected()) {
    Serial.println(WiFi.status() == WL_CONNECTED);
    Serial.println("Connecting to MQTT...");
    Serial.println(mqttUser);
    Serial.println(mqttPassword);
    if (client.connect("sumpsensor", mqttUser, mqttPassword )) {
      Serial.println("connected to MQTT server");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
    }
  }
}

int getDistance() {
  long duration;
  int distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance= duration*0.034/2;
  return distance;
}

void publish(char* payload) {
  connectMQTT();
  client.publish(mqttTopic, payload);
}


void setup() {
 
 delay(3000);
 Serial.begin(115200);
 Serial.println();

 configurator.begin();

 EEPROM.begin(500);
 EEPROM.get(0, configData);
 Serial.print("size of data: ");
 Serial.println(sizeof(configData));
 Serial.println(configData);

 while (true) {
   char* ssid = strtok(configData, "|");
   if (ssid == NULL) break;
   Serial.println(ssid);
   char* wifipassword = strtok(NULL, "|");
   if (wifipassword == NULL) break;
   Serial.println(wifipassword);
   mqttServer = strtok(NULL, "|");
   if (mqttServer == NULL) break;
   Serial.println(mqttServer);
   mqttPort = atoi(strtok(NULL, "|"));
   if (mqttPort == NULL) break;
   Serial.println(mqttPort);
   mqttUser = strtok(NULL, "|");
   if (mqttUser == NULL) break;
   Serial.println(mqttUser);
   mqttPassword = strtok(NULL, "|");
   if (mqttPassword == NULL) break;
   Serial.println(mqttPassword);
   mqttTopic = strtok(NULL, "|");
   if (mqttTopic == NULL) break;
   Serial.println(mqttTopic);
   readInterval = atoi(strtok(NULL, "|"));
   if (readInterval == NULL) break;
   Serial.println(readInterval);
   reportInterval = atoi(strtok(NULL, "|"));
   if (reportInterval == NULL) break;
   Serial.println(reportInterval);
   minNormalDistance = atoi(strtok(NULL, "|"));
   Serial.println(minNormalDistance);
   break;
 }

 client.setServer(mqttServer, mqttPort);
 pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
 pinMode(echoPin, INPUT); // Sets the echoPin as an Input
 
}

void loop() {
  unsigned int now = millis(); //note millis() will cycle back to zero every 50 days

  configurator.handleClient();
  
  if (now - lastRead > readInterval || now < lastRead) {
    int dist = getDistance();
    Serial.print("water level is ");
    Serial.print(dist);
    Serial.println("cm from sensor");
    lastRead = now;

    //report less frequently than we read, unless distance is below threshold
    if (dist < minNormalDistance || now - lastReported > reportInterval || now < lastReported) {
      char distChar[5];
      String distStr = String(dist);
      distStr.toCharArray(distChar,5);
      publish(distChar);
      lastReported = now;
    }  
  }
} 
