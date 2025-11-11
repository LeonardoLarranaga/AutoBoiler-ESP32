#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "Display.h"
#include "KiLL.h"
#include <ezButton.h>

class EncoderTask {
public:
  EncoderTask(KiLL* sys);
  void begin();
  static void task(void* pvParameters);

private:
  ESP32Encoder encoder;
  KiLL* state;
  ezButton button; 
};
