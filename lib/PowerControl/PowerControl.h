#pragma once

#include <Arduino.h>
#include "PID_v1_bc.h"
#include "KiLL.h"

class PowerControl {
public:
    PowerControl(KiLL* sys);
    
    void begin();
    
    // Obtiene el valor actual de potencia calculado por el PID (0-100)
    double getPowerOutput();
    
    // Configura los parámetros del PID
    void setTunings(double Kp, double Ki, double Kd);

private:
    static void pidTask(void* pvParameters);
    
    KiLL* system;
    TaskHandle_t pidTaskHandle;
    
    // Variables para el PID
    double input;      // Temperatura actual de salida
    double output;     // Potencia de salida (0-100)
    double setpoint;   // Temperatura objetivo
    
    PID* pid;
    
    portMUX_TYPE outputMux;
};
