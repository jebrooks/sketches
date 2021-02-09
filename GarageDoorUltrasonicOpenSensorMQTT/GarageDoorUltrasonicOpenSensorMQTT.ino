#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <SimpleTimer.h>

/* Set these to your desired credentials. */
const char *ssid = "N9I73"; //Enter your WIFI ssid
const char *password = "1801523b3b"; //Enter your WIFI password
const char* mqttServer = "192.168.1.200";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "cutieserver";
const char* mqttCmndTopic = "garagedoor/cmd";
const char* mqttStateTopic = "garagedoor/state";

const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
const int relayPin = 14; //D5

unsigned long lastChecked = 0;
const int checkIntervalMillis = 5000; //level check Interval
unsigned long lastHeartbeat = 0;
const int heartbeatIntervalMillis = 10000; //distance always reported to MQTT topic on heartbeat interval
unsigned long now = 0;
const int openMaxDistance = 50;
bool isOpen = false;

//SimpleTimer timer;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("garagedooropener", mqttUser, mqttPassword )) {
      Serial.println("connected to MQTT server");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
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
  client.publish(mqttStateTopic, payload);
}

void checkIfItWorked(bool desireOpen) {
  checkDoor(true);
  if (isOpen != desireOpen) {
    pushButton();
    //timer.setTimeout(10000, isOpen(true)); 
  }
}

void handleCommand(char* topic, byte* payload, unsigned int length) {
  Serial.println("handleCommand()");
  
  if (!strncmp((char *)payload, "OPEN", length)) {
    openDoor();
  }
  if (!strncmp((char *)payload, "CLOSE", length)) {
    closeDoor();
  }
}

bool checkDoor(bool reportIt) {
  int dist = getDistance();
  isOpen = dist <= openMaxDistance;
  Serial.print("in checkDoor(); isOpen: ");
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
  delay(2000);
  digitalWrite(relayPin, LOW);
}

void openDoor() {
  Serial.println("openDoor()");
  checkDoor(true);
  if (! isOpen) {
    Serial.println("door is closed, pushing button");
    pushButton();
    //timer.setTimeout(10000, checkIfItWorked(true)); 
  }
}

void closeDoor() {
  Serial.println("closeDoor()");
  checkDoor(true);
  if (isOpen) {
    Serial.println("door is open, pushing button");
    pushButton();
    //timer.setTimeout(10000, checkIfItWorked(false)); 
  }
}

void setup() {
 
 delay(3000);
 Serial.begin(115200);
 Serial.println();
 Serial.print("Configuring access point...");
 WiFi.begin(ssid, password);

 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }

 Serial.println("");
 Serial.println("WiFi connected");
 Serial.println("IP address: ");
 Serial.println(WiFi.localIP()); 

 client.setServer(mqttServer, mqttPort);
 client.setCallback(handleCommand);

 pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
 pinMode(echoPin, INPUT); // Sets the echoPin as an Input
 pinMode(relayPin, OUTPUT);

 connectMQTT();
 checkDoor(true);
 client.subscribe(mqttCmndTopic);
}

void loop() {

 client.loop(); //required to push msgs to handleCommand() callback
 
 now = millis();

 if (now - lastChecked > checkIntervalMillis || lastChecked > now) { //millis() will cycle back to zero every 50 days
   Serial.println("regular check of door state...");
   bool lastState = isOpen;
   checkDoor(false);
   client.subscribe(mqttCmndTopic);
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
