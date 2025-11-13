// I2CBus.h
#ifndef I2CBUS_H
#define I2CBUS_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Mutex global para transacciones I2C entre tareas/componentes 
SemaphoreHandle_t getI2CMutex();

#endif


