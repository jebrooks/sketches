#include <EEPROM.h>
#include <WIFIConfigurator.h>

String configLabels = "myinteger,mystring";

WIFIConfigurator configurator(configLabels, 100);

void setup() {
  
  delay(3000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Reading config...");
  configurator.begin();
  
  //WifiConfigurator.init(configLabels, deviceName); //this will start AP if ssid and password not configured or can't connect to wifi
  //if (WifiConfigurator.configReady()) { //will be false if AP is started
    //do your normal app setup
  //}
  //String config[] WifiConfigurator.getConfig();
  // ... set your config, values correspond to config lable order, and are all String so you will need to convert numbers manually
  
  //read config from EEPROM
  //configData = ....
  //if no ssid and password, start AP and set ap=true
  char configData[100];
  EEPROM.begin(100);
  EEPROM.get(0, configData);
  Serial.print("size of data: ");
  Serial.println(sizeof(configData));
  Serial.println(configData);

}

void loop() {
  configurator.handleClient();
}
