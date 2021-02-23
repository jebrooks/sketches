#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <WIFIConfigurator.h>

const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Button Topic|MQTT State Topic|Sensor Read Interval (ms)|Max Open Distance (cm)";

char* mqttServer;
int mqttPort;
char* mqttUser;
char* mqttPassword;
char* mqttButtonTopic;
char* mqttStateTopic;
int checkIntervalMillis;
int maxOpenDistance;

const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
const int relayPin = 4; //D2

unsigned long now = 0;
unsigned long lastChecked = 0;
bool isOpen = false;
bool configReady = false;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

WIFIConfigurator configurator(configLabels);
char configData[500];

void connectMQTT() {
  if (! client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("garagedooropener", mqttUser, mqttPassword )) {
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
  distance = duration*0.034/2;
  return distance;
}

void publish(char* payload) {
  publish(mqttStateTopic, payload);
}

void publish(char* topic, char* payload) {
  connectMQTT();
  Serial.print("publish ");
  Serial.print(topic);
  Serial.print(" ");
  Serial.println(payload);
  client.publish(topic, payload);
}

void handleCommand(char* topic, byte* payload, unsigned int length) {
  Serial.println("handleCommand()");
  Serial.println(topic);
  char myNewArray[length+1];
  for (int i=0;i<length;i++)
  {
    myNewArray[i] = (char)payload[i];
  }
  myNewArray[length] = NULL;
  Serial.print("Payload: ");
  Serial.println(myNewArray);  // null terminated array
  if (!strncmp((char *)payload, "PUSHBUTTON", length)) {
    pushButton();
  }
}

bool checkDoor(bool reportIt) {
  int dist = getDistance();
  isOpen = dist <= maxOpenDistance;
  Serial.println("checkDoor()");
  Serial.print("isOpen: ");
  Serial.print(isOpen);
  Serial.print("  Distance: ");
  Serial.println(dist);
  if (reportIt) {
    if (isOpen) {
      publish("OPEN");
    } else {
      publish("CLOSED");
    }
  }
  
  return isOpen;
}

void pushButton() {
  Serial.println("pushButton()");
  digitalWrite(relayPin, HIGH);
  delay(500);
  digitalWrite(relayPin, LOW);
}

void setup() {
 
  delay(3000);
  Serial.begin(115200);
  Serial.println(); 

  configurator.begin();

  EEPROM.begin(500);
  EEPROM.get(0, configData);
  Serial.println(configData);

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
    mqttButtonTopic = strtok(NULL, "|");
    if (mqttButtonTopic == NULL) break;
    mqttStateTopic = strtok(NULL, "|");
    if (mqttStateTopic == NULL) break;
    checkIntervalMillis = atoi(strtok(NULL, "|"));
    if (checkIntervalMillis == NULL) break;
    maxOpenDistance = atoi(strtok(NULL, "|"));
    if (maxOpenDistance == NULL) break; 

    client.setServer(mqttServer, mqttPort);
    client.setCallback(handleCommand);

    connectMQTT();
    checkDoor(true);
    client.subscribe(mqttButtonTopic);

    configReady = true;
    Serial.println("config ready!");
    break;
  } 

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(relayPin, OUTPUT);
}

void loop() {

  configurator.handleClient();
 
  now = millis();

  if (configReady) {
    client.loop(); //required to push msgs to handleCommand() callback

    if (now - lastChecked > checkIntervalMillis || lastChecked > now) { //millis() will cycle back to zero every 50 days
      Serial.println("regular check of door state...");
      bool lastState = isOpen;
      checkDoor(false);
      char rssiChar[5];
      String rssiStr = String(WiFi.RSSI());
      rssiStr.toCharArray(rssiChar,5);
      publish("rssi/garage2",rssiChar);
      //client.subscribe(mqttButtonTopic);
      if (isOpen != lastState) {
        Serial.println("door changed state, probably because of manual button press");
        if (isOpen) {
          publish("OPEN");
        } else {
          publish("CLOSED");
        }
      }
      lastChecked = now;
    }
  }
} 
