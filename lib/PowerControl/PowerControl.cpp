#include "PowerControl.h"

PowerControl::PowerControl(KiLL* sys) 
    : system(sys), 
      input(0), 
      output(0), 
      setpoint(0),
      outputMux(portMUX_INITIALIZER_UNLOCKED) {
    
    pid = new PID(&input, &output, &setpoint, 1.5, 0.5, 1.0, DIRECT);
    
    // Configurar límites de salida (0-100%)
    pid->SetOutputLimits(0, 100);
    
    // Configurar tiempo de muestreo (100ms)
    pid->SetSampleTime(100);
    
    // Iniciar en modo automático
    pid->SetMode(AUTOMATIC);
}

void PowerControl::begin() {
    xTaskCreatePinnedToCore(
        pidTask,
        "PIDTask",
        8192,
        this,
        2,
        &pidTaskHandle,
        1
    );
}

void PowerControl::pidTask(void* pvParameters) {
    PowerControl* self = static_cast<PowerControl*>(pvParameters);
    
    for (;;) {
        float waterFlow = self->system->getWaterFlow();
        bool isOn = self->system->getOn();
        
        if (waterFlow >= 3.0 && isOn) {
            // Actualizar valores del PID
            self->input = self->system->getTemperatureOut();
            self->setpoint = self->system->getTarget();

            // Ejecutar el cálculo del PID
            self->pid->Compute();
            Serial.printf("%.2f\n", self->output);
            
        } else {
            portENTER_CRITICAL(&self->outputMux);
            self->output = 0;
            portEXIT_CRITICAL(&self->outputMux);
            
            self->system->setPower(0);
            
            // Reiniciar el PID para evitar windup del integrador
            self->pid->Initialize();
        }
        
        // Ejecutar cada 50ms para respuesta rápida
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

double PowerControl::getPowerOutput() {
    double result;
    portENTER_CRITICAL(&outputMux);
    result = output;
    portEXIT_CRITICAL(&outputMux);
    return result;
}

void PowerControl::setTunings(double Kp, double Ki, double Kd) {
    pid->SetTunings(Kp, Ki, Kd);
}

