// I2CBus.cpp
#include "I2CBus.h"

static SemaphoreHandle_t i2cMutexHandle = nullptr;

SemaphoreHandle_t getI2CMutex() {
  if (i2cMutexHandle == nullptr) i2cMutexHandle = xSemaphoreCreateMutex();
  return i2cMutexHandle;
}


