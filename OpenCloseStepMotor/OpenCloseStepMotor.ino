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
const int rpm = 25;
int closePosition = 0;
int openPosition = 0;
int desiredPosition = 0;
int position = 0;

char configData[300];
const String configLabels = "MQTT Server Host|MQTT Port|MQTT User|MQTT Password|MQTT Topic|Open Position|Close Position|Current Position";

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
  desiredPosition = openPosition;
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
   openPosition = atoi(strtok(NULL, "|"));
   closePosition = atoi(strtok(NULL, "|"));
   position = atoi(strtok(NULL, "|"));
   desiredPosition = position;

   Serial.print("subscribing to: ");
   Serial.println(mqttTopic);
   Serial.print("closePosition: ");
   Serial.println(closePosition);
   Serial.print("openPosition: ");
   Serial.println(openPosition);

   client.setServer(mqttServer, mqttPort);

   myStepper.setSpeed(rpm);
 
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
      savePosition();
    }
  }

  if (position > desiredPosition) {
    myStepper.step(-100);
    position = position - 100;
    if (position <= desiredPosition) {
      turnOffMotor();
      position = desiredPosition;
      savePosition();      
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
  if (!client.connected() && client.connect(myhostname, mqttUser, mqttPassword )) {
    Serial.println("connected to MQTT server");
    client.subscribe(mqttTopic);   
    client.setCallback(handleCommand); 
  }
}

void savePosition() {

  //currentPosition is the last element stored in EEPROM, find it and overwrite
  char data[300];
  EEPROM.begin(300);
  EEPROM.get(0, data);
  int lastPipePos = 0;
  for (int i=0; i < 300; i++) {
     if (data[i] == '|') {
        lastPipePos = i;
     }
  }
  String posStr = String(position);
  Serial.println(posStr);
  int strLen = posStr.length() + 1;
  char posArr[strLen];
  posStr.toCharArray(posArr,strLen);

  Serial.println(posArr);
  for (int i=0; i < 300 - lastPipePos; i++) {
    if (i < strLen) {
      data[lastPipePos + i + 1] = posArr[i];
    } else {
      data[lastPipePos + i + 1] = 0;
    }
  }

  Serial.println(data);
  
  EEPROM.put(0,data);
  EEPROM.commit();
}
