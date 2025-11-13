#ifndef KILL_H
#define KILL_H

#include <Arduino.h>
#include "Display.h"
#include "Memory.h"
#include <iterator>
#include "WString.h"
#include "Sensors.h"

class KiLL {
private:
    int target;
    float waterFlow;
    float temperatureIn;
    float temperatureOut;
    float power;
    bool isOn;
    bool isOnline;
    String boilerId;
    Sensors sensors;
    OLEDDisplay display;

public:
    
    KiLL();

    // ======= SETTERS =======
    void setTarget(int value);
    void setOn(bool state);
    void setOnline(bool state);
    void setPower(float value);      
    void toggleOn();         

    // ======= GETTERS =======
    int getTarget();
    float getWaterFlow();
    float getTemperatureIn();
    float getTemperatureOut();
    float getPower();
    bool getOn();       
    String getBoilerId();

    void begin();

    void onConnecting(String label);
    void results(String label);

    void onRestarted();
    void onAdvice();

    void started();

};

#endif
