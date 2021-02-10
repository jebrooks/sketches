#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

/* Set these to your desired credentials. */
const char* ssid = "N9I73";
const char* wifipword = "1801523b3b";
const char* mqttServer = "192.168.1.200";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "cutieserver";
const char* mqttTopic = "sumpsensor";
const int readInterval = 30000; //30s
const int reportInterval = 300000; //5m
const int minNormalDistance = 20; //if distance becomes smaller than this, report on every read

const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

unsigned int lastRead = 0;
unsigned int lastReported = 0;

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
 WiFi.begin(ssid, wifipword);
 int i = 0;
 while (WiFi.status() != WL_CONNECTED && i < 40) {
   delay(500);
   Serial.print(".");
   i++;
 } 

 Serial.print("IP address: ");
 Serial.println(WiFi.localIP());
 client.setServer(mqttServer, mqttPort);
 pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
 pinMode(echoPin, INPUT); // Sets the echoPin as an Input
 
}

void loop() {
  int now = millis(); //note millis() will cycle back to zero every 50 days

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
