#include "Sensors.h"

// === Configuración global del sensor de flujo ===
#define FLOW_PIN 15     
#define FLOW_TYPE YFS201  

FlowSensor flowSensor(FLOW_TYPE, FLOW_PIN);

void IRAM_ATTR flowCountISR() {
  flowSensor.count();
}

Sensors::Sensors()
  : channelTempIn(2),
    channelTempOut(3),
    channelFlow(1),
    channelPower(0),
    seriesResistor(10000.0f),
    nominalResistance(10000.0f),
    nominalTemp(25.0f),
    betaCoef(3950.0f),
    lastFlowUpdate(0),
    lastFlowRate(0.0f),
    currentPin(34),      
    voltageAC(127.0f),  
    currentIrms(0.0)
{}

bool Sensors::begin() {
  Wire.begin(19, 21);

  if (!ads.begin(0x48)) {
    Serial.println("❌ Error al inicializar ADS1115");
    return false;
  }

  ads.setGain(GAIN_ONE);  

  // --- Sensor de flujo ---
  flowSensor.begin(flowCountISR);
  Serial.println("✅ Sensor de flujo YF-B1 iniciado correctamente");

  emon1.current(currentPin, 100.0); 
  Serial.println("✅ Sensor de corriente SCT-013 iniciado correctamente");

  Serial.println("✅ ADS1115 iniciado correctamente");
  return true;
}

float Sensors::adcToVoltage(int16_t raw) {
  return raw * 0.000125f; 
}

float Sensors::readThermistor(int channel) {
  int16_t adcValue = ads.readADC_SingleEnded(channel);
  float voltage = adcToVoltage(adcValue);

  if (voltage <= 0.001f) return NAN;

  // Termistor a Vcc, resistencia a GND
  float resistance = seriesResistor * ((3.3f / voltage) - 1.0f);

  float steinhart;
  steinhart = resistance / nominalResistance;
  steinhart = log(steinhart);
  steinhart /= betaCoef;
  steinhart += 1.0f / (nominalTemp + 273.15f);
  steinhart = 1.0f / steinhart;
  steinhart -= 273.15f;

  return steinhart;
}


float Sensors::getTempIn() {
  return readThermistor(channelTempIn);
}

float Sensors::getTempOut() {
  return readThermistor(channelTempOut);
}

float Sensors::getFlow() {
  if (millis() - lastFlowUpdate >= 1000) {
    flowSensor.read();
    lastFlowRate = flowSensor.getFlowRate_m();
    lastFlowUpdate = millis();
  }
  return lastFlowRate;
}

double Sensors::getCurrent() {
  currentIrms = emon1.calcIrms(1480);
  return currentIrms;
}

float Sensors::getPower() {
  int16_t adcValue = ads.readADC_SingleEnded(2);
  float voltage = adcToVoltage(adcValue);
  return voltage;
}
