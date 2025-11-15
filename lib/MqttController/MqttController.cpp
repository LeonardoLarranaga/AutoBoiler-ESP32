#include "esp32-hal.h"
#include "HardwareSerial.h"
#include "MqttController.h"

MQTTController::MQTTController(KiLL* sys) : client(espClient), system(sys){}

void MQTTController::begin() {
    topicUpdates = "kill/updates/" + system->getBoilerId();

    baseCommandTopic = "kill/commands/" + system->getBoilerId(); 
    topicTarget          = baseCommandTopic + "/target";
    topicIsOn            = baseCommandTopic + "/powerState";
    topicTasteWifi       = baseCommandTopic + "/tasteWifi";
    topicSaveCredentials = baseCommandTopic + "/saveCredentials";
    topicConfirm         = baseCommandTopic + "/confirm";
}

void MQTTController::connectGlobal(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    Serial.println("Conectando a WiFi global");
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        system->onConnecting("Conectando WiFi");
    }

    if (WiFi.status() == WL_CONNECTED) {
        system->results("Conectado");
        delay(1000);
    } else {
        system->results("No conectado");
        delay(1000);
        Serial.println("No conectado, conectando local");
        connectLocal();
        return;
    }

    client.setServer("170.9.22.130", 1883);

    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        {this->callback(topic, payload, length);}
    });

    start = millis();

    while (!client.connected() && millis() - start < 8000) {
        system->onConnecting("Conectando MQTT");

        if (client.connect("ESP32Client")) {
            system->results("Conectado a servidor");
            delay(1000);
            break;
        }
    }

    if(client.connect("ESP32Client")){
        client.subscribe(topicTarget.c_str());
        client.subscribe(topicIsOn.c_str());
        system->started();
    } 

    xTaskCreatePinnedToCore(
        mqttTask, 
        "MqttTask", 
        8192, 
        this, 
        1, 
        &mqttTaskHandle, 
        0
    );    
}

void MQTTController::connectLocal() {

    WiFi.mode(WIFI_AP_STA);
    IPAddress localIp(192, 168, 39, 12);
    IPAddress gateway(192, 168, 39, 12);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(localIp, gateway, subnet);
    WiFi.softAP("KiLL-" + system->getBoilerId(), "12345678");
    
         
    broker.subscribe("#", [this](const char* topic, const char* payload) {
        this->callback((char*)topic, (byte*)payload, strlen(payload));
    });

    broker.begin();

    system->started();

    xTaskCreatePinnedToCore(brokerTask, "BrokerTask", 8192, this, 1, &brokerTaskHandle, 0);
}

void MQTTController::callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    String topicStr = String(topic);

    if (topicStr == topicTasteWifi) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, msg);
      
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        bool connected = connectToWifiTemp(ssid, password, 10000);
        publishFloat(topicConfirm, connected ? 0 : 1, false, false);
        
    }
    else if (topicStr == topicSaveCredentials) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, msg);
        if (error) {
            system->results("JSON Inválido saveConfirm");
            return;
        }

        const char* ssid = doc["ssid"];
        const char* password = doc["password"];
        const char* name = doc["name"];
        const char* token = doc["token"];

        connectGlobal(ssid, password);

        HTTPClient http;
        http.begin("http://170.9.22.130:3000/app/kill/add");
        http.addHeader("Content-Type", "application/json");
        String jsonData = "{";
        jsonData += "\"name\":\"" + String(name) + "\",";
        jsonData += "\"token\":\"" + String(token) + "\",";
        jsonData += "\"killId\":\"" + String(system->getBoilerId()) + "\"}";

        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0) {
            publishFloat(topicConfirm, 0, false, false);
            Memory::write(String(ssid), String(password));
        } else {
            publishFloat(topicConfirm, 1, false, false);
        }

        http.end();
    }
    else if (topicStr == topicTarget) {
        system->setTarget(msg.toInt());
    }
    else if (topicStr == topicIsOn) {
       
        float value = msg.toFloat();

        if (value == 0.0f) {
            system->setOn(true);
        } 
        else if (value == 1.0f) {
            system->setOn(false);
        }
    }
}

bool MQTTController::connectToWifiTemp(const char* ssid, const char* password, unsigned long timeout) {
    WiFi.disconnect(true);
    WiFi.begin(ssid, password); 

    unsigned long start = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
        delay(200);
    }

    return WiFi.status() == WL_CONNECTED;
}

void MQTTController::runTaskLoop(bool isServer) {
    unsigned long lastPublish = 0;

    for (;;) {
        if (isServer) {
            client.loop();
        } else {
            broker.loop();
        }

        unsigned long now = millis();
        if (now - lastPublish >= 3000) {
            lastPublish = now;
            publishCombined(isServer, false);
            if (system->getOn()) {
                publishInt(topicIsOn, isServer ? 0 : 1, isServer, true);
            } else {
                publishInt(topicIsOn, isServer ? 1 : 0, isServer, true);
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void MQTTController::brokerTask(void* pvParameters) {
    MQTTController* self = static_cast<MQTTController*>(pvParameters);
    self->runTaskLoop(false);
}

void MQTTController::mqttTask(void* pvParameters) {
    MQTTController* self = static_cast<MQTTController*>(pvParameters);
    self->runTaskLoop(true);
}


void MQTTController::publishFloat(const String& topic, float value, bool server, bool retain) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    if (server) client.publish(topic.c_str(), buffer, retain);
    else broker.publish(topic.c_str(), buffer, retain);
}

void MQTTController::publishInt(const String& topic, int value, bool server, bool retain) {
    char buffer[12]; 
    snprintf(buffer, sizeof(buffer), "%d", value);

    if (server) client.publish(topic.c_str(), buffer, retain);
    else broker.publish(topic.c_str(), buffer, retain);
}

void MQTTController::publishCombined(bool server, bool retain) {
    float power = system->getPower();
    float flow = system->getWaterFlow();
    float tempOut = system->getTemperatureOut();
    float tempIn = system->getTemperatureIn();
    int target = system->getTarget();

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f,%.2f,%d", power, flow, tempOut, tempIn, target);

    if (server) client.publish(topicUpdates.c_str(), buffer, retain);
    else broker.publish(topicUpdates.c_str(), buffer, retain);
}