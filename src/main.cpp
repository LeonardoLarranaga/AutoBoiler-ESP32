#include <Arduino.h>
#include "KiLL.h"
#include "EncoderTask.h"
#include "MqttController.h"
#include "PowerControl.h"

#define ZC_PIN     26
#define TRIAC_PIN  25

const int FREQ = 60;                   
const int HALF_CYCLE_US = 8333;        
const int MIN_DELAY = 0;               
const int MAX_DELAY = HALF_CYCLE_US;

volatile bool zeroCrossDetected = false;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t *timer = NULL;
KiLL systemState;
EncoderTask encoder(&systemState);
MQTTController mqtt(&systemState);
PowerControl powerControl(&systemState);

void IRAM_ATTR fireTriac() {
  digitalWrite(TRIAC_PIN, HIGH);
  delayMicroseconds(50);
  digitalWrite(TRIAC_PIN, LOW);
}

void IRAM_ATTR onZeroCross() {
  zeroCrossDetected = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando sistema...");
  systemState.begin();
  encoder.begin();
  powerControl.begin();
  mqtt.begin();
 
  if (Memory::verifyContent()) mqtt.connectGlobal(Memory::getSSID().c_str(), Memory::getPassword().c_str());
  else mqtt.connectLocal();

  pinMode(ZC_PIN, INPUT);
  pinMode(TRIAC_PIN, OUTPUT);
  digitalWrite(TRIAC_PIN, LOW);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &fireTriac, true);

  attachInterrupt(digitalPinToInterrupt(ZC_PIN), onZeroCross, RISING);

}

void loop() {
  if (!zeroCrossDetected) return;
  
  portENTER_CRITICAL(&mux);
  zeroCrossDetected = false;
  portEXIT_CRITICAL(&mux);

  // Si no hay flujo de agua o el calentador no está encendido, se desactiva el triac
  float waterFlow = systemState.getWaterFlow();
  bool isOn = systemState.getOn();
  if (waterFlow < 3.0 || !isOn) {
    timerAlarmDisable(timer);
    digitalWrite(TRIAC_PIN, LOW);
    return;
  }

  double pidPower = powerControl.getPowerOutput();
  int firingDelay = map((int)pidPower, 0, 100, MAX_DELAY, MIN_DELAY);
  // Con esta línea se puede ajustar directamente el delay en base a la temperatura objetivo
  // int firingDelay = map(systemState.getTarget(), 0, EncoderTask::MAX_TEMP, MAX_DELAY, MIN_DELAY);

  timerAlarmWrite(timer, firingDelay, false);
  timerAlarmEnable(timer);
  timerRestart(timer);
}