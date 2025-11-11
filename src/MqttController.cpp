#include "esp32-hal.h"
#include "HardwareSerial.h"
#include "MqttController.h"

MQTTController::MQTTController(KiLL* sys) : client(espClient), system(sys){}

void MQTTController::begin() {
    baseTopic = "kill/boiler_" + system->getBoilerId(); 
    topicTarget = baseTopic + "/target";
    topicPower = baseTopic + "/power";
    topicWaterFlow = baseTopic + "/flow";
    topicTempIn = baseTopic + "/tempIn";
    topicTempOut = baseTopic + "/tempOut";
    topicIsOn = baseTopic + "/powerState";
    topicTasteWifi = baseTopic + "/tasteWifi";
    topicSaveCredentials = baseTopic + "/saveCredentials";
    topicConfirm = baseTopic + "/confirm";
}

void MQTTController::connectGlobal(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        system->onConnecting("Conectando WiFi");
    }

    if (WiFi.status() == WL_CONNECTED) {
        system->results("Conectado");
        delay(1000);
    } else {
        system->results("No conectado");
        delay(1000);
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

    xTaskCreatePinnedToCore(mqttTask, "MqttTask", 8192, this, 1, &mqttTaskHandle, 0);    
}

void MQTTController::connectLocal() {

    WiFi.mode(WIFI_AP_STA);
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
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, msg);
      
        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        bool connected = connectToWifiTemp(ssid, password, 10000);
        publishFloat(topicConfirm, connected ? 0 : 1, false, false);
        
    }
    else if (topicStr == topicSaveCredentials) {
        StaticJsonDocument<256> doc;
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
    else if (topicStr == topicTarget){
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

void MQTTController::brokerTask(void* pvParameters) {
    MQTTController* self = static_cast<MQTTController*>(pvParameters);
    unsigned long lastPublish = 0;

    for (;;) {
        self->broker.loop();

        unsigned long now = millis();
        if (now - lastPublish >= 3000) {
            lastPublish = now;
            self->publishFloat(self->topicTempOut, self->system->getTemperatureOut(), false, false);
            self->publishFloat(self->topicTempIn, self->system->getTemperatureIn(), false, false);
            self->publishFloat(self->topicPower, self->system->getPower(), false, false);
            self->publishFloat(self->topicWaterFlow, self->system->getWaterFlow(), false, false);
            self->publishFloat(self->topicTarget, self->system->getTarget(), false, true);
            if(self->system->getOn())
                self->publishFloat(self->topicIsOn, 1, false, true);
            else 
                self->publishFloat(self->topicIsOn, 0, false, true);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void MQTTController::mqttTask(void* pvParameters) {
    MQTTController* self = static_cast<MQTTController*>(pvParameters);
    unsigned long lastPublish = 0;

    for (;;) {
        self->client.loop();

        unsigned long now = millis();
        if (now - lastPublish >= 3000) {
           
            self->publishFloat(self->topicTempOut, self->system->getTemperatureOut(), true, false);
            self->publishFloat(self->topicTempIn, self->system->getTemperatureIn(), true, false);
            self->publishFloat(self->topicPower, self->system->getPower(), true, false);
            self->publishFloat(self->topicWaterFlow, self->system->getWaterFlow(), true, false);
            self->publishFloat(self->topicTarget, self->system->getTarget(), true, true);

            if (self->system->getOn()) {
                self->publishInt(self->topicIsOn, 0, true, true);
            } else {
                self->publishInt(self->topicIsOn, 1, true, true);
            }

            lastPublish = now;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
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




