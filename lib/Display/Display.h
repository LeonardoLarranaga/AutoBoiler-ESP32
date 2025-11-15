#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <WiFi.h>
#include "I2CBus.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class OLEDDisplay {
private:
    int sda, scl, rst;
    Adafruit_SSD1306 display;
    TaskHandle_t wifiClockTaskHandle;

    void showCurrentTemperature(float current);   
    void showTargetTemperature(int target);    

public:
    OLEDDisplay();
    void begin();

    // Mostrar
    void showTemperatures(float current, int target);
    void showStatusOnline(int wifiStrength, const char* date, const char* time);
    void showStatusOffline(const char* left);

    // Display control
    void displayOff();
    void displayOn();
    void clearDisplay();

    // Animaciones y mensajes
    void results(const char* label);
    void onConnecting(const char* message);

    void message(const char* title, const char* text);
    void showStartupAnimation();

    void startAutoStatus();
};

#endif
