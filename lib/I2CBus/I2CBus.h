// I2CBus.h
#ifndef I2CBUS_H
#define I2CBUS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Mutex global para transacciones I2C entre tareas/componentes
 * @return
 */
SemaphoreHandle_t getI2CMutex();

#endif


