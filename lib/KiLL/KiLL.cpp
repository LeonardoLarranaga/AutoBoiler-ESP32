#include "KiLL.h"

KiLL::KiLL() :
  waterFlow(0), 
  temperatureIn(0),
  temperatureOut(0), 
  power(0), 
  isOn(false),
  isOnline(true){}

// ======= SETTERS =======
void KiLL::setTarget(int value) { 
  target = value; 
  display.showTargetTemperature(value);
}

void KiLL::toggleOn() { 
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

// ======= GETTERS =======
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
  display.showCurrentTemperature(temperatureOut);
  return temperatureOut; 
}

float KiLL::getPower() { 
  return power; 
}

void KiLL::begin(){
  Memory::initialize();
  Memory::clear();
  Memory::writeBoilerId("3912");
  Memory::write("IZZI-D64E", "DCA63395D64E");

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
}

void KiLL::onAdvice() {
  display.message("Advertencia", "Se reiniciara el dispositivo si no suelta el boton");
}

void KiLL::started() {
  display.showStartupAnimation();
  display.showCurrentTemperature(temperatureIn);
  display.showTargetTemperature(target);
  if(isOnline) {
    display.startAutoStatus();
  } else {
    display.showStatusOffline(("KiLL-" + boilerId).c_str());
  }
}


