#ifndef WIFIConfigurator_H 
#define WIFIConfigurator_H

#include <ESP8266WebServer.h>

class WIFIConfigurator
{
  public:
    WIFIConfigurator(String labels);
    void begin();
    void handleClient();
  private:
    int _eePromSize;
    String _labels;
    ESP8266WebServer server;
    bool configReady;
    void startAP();
    void handleRoot();
    void sendWifiForm();
    void sendConfigForm();
    void handleWifiChange();
    void handleConfigChange();
};

#endif
