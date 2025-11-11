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
    static void brokerTask(void* pvParameters);
    static void mqttTask(void* pvParameters);
    void publishFloat(const String& topic, float value, bool server, bool retain);
    void publishInt(const String& topic, int value, bool server, bool retain);
    bool postAddBoiler(const char* token, const char* killId, const char* name);
    bool connectToWifiTemp(const char* ssid, const char* password, unsigned long timeout);
    void callback(char* topic, byte* payload, unsigned int length);
    

    WiFiClient espClient;
    PubSubClient client;
    PicoMQTT::Server broker;  
    PicoMQTT::Client clientPico;     

    KiLL* system;
    TaskHandle_t mqttTaskHandle;
    TaskHandle_t brokerTaskHandle;

    String baseTopic;
    String topicTarget;
    String topicPower;
    String topicWaterFlow;
    String topicTempIn;
    String topicTempOut;
    String topicIsOn;

    String topicTasteWifi;
    String topicSaveCredentials;
    String topicConfirm;
};
