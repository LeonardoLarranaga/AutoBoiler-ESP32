#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <PicoMQTT.h>   
#include "KiLL.h"
#include "Display.h"
#include "Memory.h"
#include <ArduinoJson.h> 
#include <HTTPClient.h>
#include <WiFiClient.h>

class MQTTController {
public:
    MQTTController(KiLL* sys);

    void begin();
    void connectGlobal(const char* ssid, const char* password);
    void connectLocal();   

private:
    void runTaskLoop();
    void publishFloat(const String& topic, float value);
    void publishInt(const String& topic, int value);
    void publishString(const String& topic, const String& value);
    bool postAddBoiler(const char* token, const char* killId, const char* name);
    bool connectToWifiTemp(const char* ssid, const char* password, unsigned long timeout);
    void callback(char* topic, byte* payload, unsigned int length);
    void publishCombined();

    
    WiFiClient espClient;
    PubSubClient client;
    PicoMQTT::Server broker;  
    PicoMQTT::Client clientPico;     

    KiLL* system;
    TaskHandle_t mqttTaskHandle;
    TaskHandle_t brokerTaskHandle;

    String baseCommandTopic;
    String topicUpdates;
    String topicTarget;
    String topicIsOn;

    String topicTestWifi;
    String topicSaveCredentials;
    String topicConfirm;
    String topicEspId;
};
