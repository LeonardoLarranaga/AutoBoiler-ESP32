#include "esp32-hal.h"
#include "HardwareSerial.h"
#include "MqttController.h"

MQTTController::MQTTController(KiLL* sys) : client(espClient), system(sys){}

void MQTTController::begin() {
    topicUpdates = "kill/updates/" + system->getBoilerId();
    baseCommandTopic = "kill/commands/" + system->getBoilerId(); 

    topicTarget          = baseCommandTopic + "/target";
    topicIsOn            = baseCommandTopic + "/powerState";
    topicTestWifi        = baseCommandTopic + "/testWifi";
    topicSaveCredentials = baseCommandTopic + "/saveCredentials";
    topicConfirm         = baseCommandTopic + "/confirm";
    topicEspId           = "kill/espId";
}

void MQTTController::connectGlobal(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    // Setup local AP while connecting to global WiFi
    IPAddress localIp(192, 168, 39, 12);
    IPAddress gateway(192, 168, 39, 12);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(localIp, gateway, subnet);
    WiFi.softAP("KiLL-" + system->getBoilerId(), "12345678");
    
    // Setup local broker
    broker.subscribe("#", [this](const char* topic, const char* payload) {
        this->callback((char*)topic, (byte*)payload, strlen(payload));
    });
    broker.begin();
    
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
        system->started();
        xTaskCreatePinnedToCore(
            [](void* param) {
                MQTTController* self = static_cast<MQTTController*>(param);
                self->runTaskLoop();
            }, 
            "BrokerTask", 
            8192, 
            this, 
            1, 
            &brokerTaskHandle, 
            0
        );
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
        [](void* param) {
            MQTTController* self = static_cast<MQTTController*>(param);
            self->runTaskLoop();
        }, 
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

    xTaskCreatePinnedToCore(
        [](void* param) {
            MQTTController* self = static_cast<MQTTController*>(param);
            self->runTaskLoop();
        }, 
        "BrokerTask", 
        8192, 
        this, 
        1, 
        &brokerTaskHandle, 
        0
    );
}

void MQTTController::callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    String topicStr = String(topic);
    Serial.printf("topic: %s, msg: %s\n", topicStr.c_str(), msg.c_str());

    if (topicStr == topicEspId) {
        if (msg.toInt() == 1) publishString(topicEspId, system->getBoilerId());
        broker.loop();
    } else if (topicStr == topicTestWifi) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, msg);
      
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        bool connected = connectToWifiTemp(ssid, password, 10000);
        publishInt(topicConfirm, connected ? 1 : 0);
    } else if (topicStr == topicSaveCredentials) {
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

        if (WiFi.status() != WL_CONNECTED) {
            delay(5000);
            publishInt(topicConfirm, 0);
            broker.loop();
            delay(100);
            return;
        }

        HTTPClient http;
        http.begin("http://170.9.22.130:3000/app/kill/create");
        http.addHeader("Content-Type", "application/json");
        String jsonData = "{";
        jsonData += "\"name\":\"" + String(name) + "\",";
        jsonData += "\"token\":\"" + String(token) + "\",";
        jsonData += "\"killId\":\"" + String(system->getBoilerId()) + "\"}";

        int httpResponseCode = http.POST(jsonData);

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (httpResponseCode >= 200 && httpResponseCode < 300) {
            publishInt(topicConfirm, 1);
            broker.loop();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            Memory::write(String(ssid), String(password));
            system->setOn(true);
        } else {
            publishInt(topicConfirm, 0);
            broker.loop();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        http.end();
    } else if (topicStr == topicTarget) {
        system->setTarget(msg.toInt());
    } else if (topicStr == topicIsOn) {
       
        float value = msg.toInt();

        if (value == 1) {
            system->setOn(true);
        }  else if (value == 0) {
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

void MQTTController::runTaskLoop() {
    unsigned long lastPublish = 0;

    for (;;) {
        client.loop();
        broker.loop();

        unsigned long now = millis();
        if (now - lastPublish >= 3000) {
            lastPublish = now;
            publishCombined();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void MQTTController::publishFloat(const String& topic, float value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    client.publish(topic.c_str(), buffer, false);
    broker.publish(topic.c_str(), buffer, false);
}

void MQTTController::publishInt(const String& topic, int value) {
    char buffer[12]; 
    snprintf(buffer, sizeof(buffer), "%d", value);

    client.publish(topic.c_str(), buffer, false);
    broker.publish(topic.c_str(), buffer, false);
}

void MQTTController::publishString(const String& topic, const String& value) {
    client.publish(topic.c_str(), value.c_str(), false);
    broker.publish(topic.c_str(), value.c_str(), false);
}

void MQTTController::publishCombined() {
    if (!Memory::verifyContent()) return;
    system->updateDisplayTemperatures();

    float power = system->getPower();
    float flow = system->getWaterFlow();
    float tempOut = system->getTemperatureOut();
    float tempIn = system->getTemperatureIn();
    int target = system->getTarget();
    int isOn = system->getOn();

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f,%.2f,%.2f,%.2f,%d,%d", power, flow, tempOut, tempIn, target, isOn);

    client.publish(topicUpdates.c_str(), buffer, false);
    broker.publish(topicUpdates.c_str(), buffer, false);
}