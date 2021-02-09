#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

/* Set these to your desired credentials. */
const char *ssid = "N9I73"; //Enter your WIFI ssid
const char *password = "1801523b3b"; //Enter your WIFI password
const char* mqttServer = "192.168.1.200";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "cutieserver";
const char* mqttTopic = "sumppumplevel";
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
unsigned long lastChecked = 0;
const int checkIntervalMillis = 180000; //level check Interval
const int minThresholdDistanceFromSensorCM = 30; //distance smaller than this gets reported to MQTT topic
unsigned long lastHeartbeat = 0;
const int heartbeatIntervalMillis = 3600000; //distance always reported to MQTT topic on heartbeat interval
unsigned long now = 0;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
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


void reportDistance(int dist) {
  char distStr[3];
  itoa(dist, distStr, 10);
  connectMQTT();
  client.publish(mqttTopic, distStr);    
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

 pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
 pinMode(echoPin, INPUT); // Sets the echoPin as an Input

 reportDistance(0);
}

void loop() {

 now = millis();

 if (now - lastChecked > checkIntervalMillis || lastChecked > now) { //millis() will cycle back to zero every 50 days
   int dist = getDistance();

   Serial.println("checking distance...");
   //report only if distance < threshold
   if (dist < minThresholdDistanceFromSensorCM) {
     reportDistance(dist);
   }
   lastChecked = now;
   
   //always report distance on the longer heartbeat interval
   if (now - lastHeartbeat > heartbeatIntervalMillis || lastHeartbeat > now) {
     Serial.println("heartbeat report...");
     reportDistance(dist);
     lastHeartbeat = now; 
   }   
 }
 
} 
