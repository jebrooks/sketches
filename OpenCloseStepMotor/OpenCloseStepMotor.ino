#include <Stepper.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WIFIConfigurator.h>
#include <EEPROM.h>

char* mqttServer;
int mqttPort;
char* mqttUser;
char* mqttPassword;
char* mqttTopic;
char* myhostname;

const int stepsPerRev = 2048;
int closePosition = 0;
int desiredPosition = 0;
int position = 0;

char configData[300];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|Close Position";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
WIFIConfigurator configurator(configLabels);

Stepper myStepper = Stepper(stepsPerRev, 5, 0, 4, 2);

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
  if (!strncmp((char *)payload, "OPEN", length)) {
    open();
  }
  if (!strncmp((char *)payload, "CLOSE", length)) {
    close();
  }
  
}

void open() {
  desiredPosition = 0;
}

void close() {
  desiredPosition = closePosition;
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
   myhostname = strtok(NULL, "|");
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
   closePosition = atoi(strtok(NULL, "|"));
   if (closePosition == NULL) break;

   Serial.print("subscribing to: ");
   Serial.println(mqttTopic);
   Serial.print("closePosition: ");
   Serial.println(closePosition);
   
   client.setServer(mqttServer, mqttPort);

   myStepper.setSpeed(15);

   break;
 }
  
}

void loop() {
  configurator.handleClient();
  connectMQTT();
  client.loop();
  
  if (position < desiredPosition) {
    myStepper.step(100);
    position = position + 100;
    if (position >= desiredPosition) {
      turnOffMotor();
      position = desiredPosition;
    }
  }

  if (position > desiredPosition) {
    myStepper.step(-100);
    position = position - 100;
    if (position <= desiredPosition) {
      turnOffMotor();
      position = desiredPosition;
    }    
  }

}

void turnOffMotor() {
  digitalWrite(5,LOW);
  digitalWrite(0,LOW);
  digitalWrite(4,LOW);
  digitalWrite(2,LOW);     
}

void connectMQTT() {
  if (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(myhostname, mqttUser, mqttPassword )) {
      Serial.println("connected to MQTT server");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
    }
    client.subscribe(mqttTopic);   
    client.setCallback(handleCommand); 
  }
}
