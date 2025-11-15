#include "KiLL.h"

KiLL::KiLL() :
  waterFlow(0), 
  temperatureIn(0),
  temperatureOut(0), 
  power(0), 
  isOn(false),
  isOnline(true){}

// MARK: SETTERS
void KiLL::setTarget(int value) { 
  if (!Memory::verifyContent()) return;
  if (value < static_cast<int>(getTemperatureIn())) return;
  target = value; 
}

void KiLL::toggleOn() { 
  if (!Memory::verifyContent()) return;
  isOn = !isOn; 
  if (isOn) display.displayOn(); else display.displayOff();
}

void KiLL::setOn(bool state) { 
  isOn = state; 
  if (isOn) display.displayOn(); else display.displayOff();
}

void KiLL::setOnline(bool state) { 
  isOnline = state; 
}

void KiLL::setPower(float value) { 
  power = value; 
}

// MARK: GETTERS
bool KiLL::getOn() { return isOn; }
String KiLL::getBoilerId() { return boilerId; }
int KiLL::getTarget() { return target; }

float KiLL::getWaterFlow() { 
  waterFlow = sensors.getFlow();
  return waterFlow; 
}
float KiLL::getTemperatureIn() { 
  temperatureIn = sensors.getTempIn();
  return temperatureIn; 
}

float KiLL::getTemperatureOut() { 
  temperatureOut = sensors.getTempOut();
  return temperatureOut; 
}

float KiLL::getPower() { 
  return power; 
}

void KiLL::begin() {
  Memory::initialize();
  
  Memory::writeBoilerId("3912");

  boilerId = Memory::getBoilerId();

  sensors.begin();
  display.begin();
}

void KiLL::onConnecting(String label) {
  display.onConnecting(label.c_str());
}

void KiLL::results(String label) {
  display.results(label.c_str());
}

void KiLL::onRestarted() {
  display.message("Reiniciado", "Todas las configuraciones fueron borradas");
  delay(1000);
  Memory::clear();
  ESP.restart();
}

void KiLL::onAdvice() {
  display.message("Advertencia", "Se reiniciara el dispositivo si no suelta el boton");
}

void KiLL::started() {
  display.showStartupAnimation();
  updateDisplayTemperatures();
  if(isOnline) {
    display.startAutoStatus();
  } else if (Memory::verifyContent()) {
    display.showStatusOffline(("KiLL-" + boilerId).c_str());
  }
}

void KiLL::updateDisplayTemperatures() {
  if (isOn) display.showTemperatures(temperatureOut, target);
}