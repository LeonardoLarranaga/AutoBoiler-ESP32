#include "EncoderTask.h"

EncoderTask::EncoderTask(KiLL* sys)
  : state(sys), button(13)
{
  encoder.attachHalfQuad(14, 27);
  encoder.setCount(0);

  button.setDebounceTime(50);  
  button.setCountMode(COUNT_FALLING); 
}

void EncoderTask::begin() {
  xTaskCreatePinnedToCore(
    task,
    "EncoderTask",
    8192,
    this,
    1,
    nullptr,
    1
  );
}

void EncoderTask::task(void* pvParameters) {
  EncoderTask* self = static_cast<EncoderTask*>(pvParameters);

  long lastEncoderValue = 0;
  long encoderOffset = 0;
  unsigned long pressStartTime = 0;
  bool longPressTriggered = false;
  bool warningTriggered = false;
  bool firstLoop = true;
  long minTemp = 0;
  long maxTemp = minTemp + 100;

  for (;;) {
    // ======== Lectura del encoder ========
    long rawEncoder = self->encoder.getCount() / 2;  // tu encoder es half quad

    if (firstLoop) {
      encoderOffset = self->state->getTarget() - rawEncoder;
      firstLoop = false;
    }

    long newValue = rawEncoder + encoderOffset;    

    if (newValue < minTemp) {
      newValue = minTemp;
      encoderOffset = newValue - rawEncoder;
    }
    if (newValue > maxTemp) {
      newValue = maxTemp;
      encoderOffset = newValue - rawEncoder;
    }

    // Actualizar valor si cambió
    if (newValue != lastEncoderValue) {
      lastEncoderValue = newValue;
      self->state->setTarget(newValue);
    }

    // ======== Lectura del botón ========
    self->button.loop();

    if (self->button.isPressed()) {
      pressStartTime = millis();
      longPressTriggered = false;
      warningTriggered = false;
    }

    if (self->button.getState() == LOW) {
      unsigned long pressDuration = millis() - pressStartTime;

      // Advertencia a los 3 segundos
      if (pressDuration > 3000 && !warningTriggered) {
        self->state->onAdvice();
        warningTriggered = true;
      }

      // Presión larga a los 6 segundos
      if (pressDuration > 6000 && !longPressTriggered) {
        self->state->onRestarted();
        longPressTriggered = true;
      }
    }

    if (self->button.isReleased()) {
      unsigned long pressDuration = millis() - pressStartTime;

      if (!longPressTriggered) {
        if (warningTriggered) {
          self->state->onRestarted();
        } else {
          self->state->toggleOn();
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}
