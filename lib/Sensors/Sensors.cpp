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
    currentIrms(0.0),
    lastValidTempIn(NAN),
    lastValidTempOut(NAN)
{}

bool Sensors::begin() {
  Wire.begin(19, 21);
  Wire.setClock(400000); // I2C a 400 kHz para transacciones más cortas

  // Proteger inicialización I2C
  xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
  bool adsOk = ads.begin(0x48);
  xSemaphoreGive(getI2CMutex());

  if (!adsOk) {
    Serial.println("❌ Error al inicializar ADS1115");
    return false;
  }

  xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
  ads.setGain(GAIN_ONE);  
  xSemaphoreGive(getI2CMutex());

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
  // Lectura simple (se usa por compatibilidad en otros métodos)
  xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
  int16_t adcValue = ads.readADC_SingleEnded(channel);
  xSemaphoreGive(getI2CMutex());

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

float Sensors::readThermistorFiltered(int channel) {
  // Mediana de 5 muestras para eliminar outliers
  const int samples = 5;
  float vals[samples];

  // Bloqueamos todo el grupo de lecturas para consistencia
  xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
  for (int i = 0; i < samples; i++) {
    int16_t adcValue = ads.readADC_SingleEnded(channel);
    vals[i] = adcToVoltage(adcValue);
    // pequeña separación opcional si fuese necesario
    // delayMicroseconds(200);
  }
  xSemaphoreGive(getI2CMutex());

  // Ordenar (burbuja simple por tamaño pequeño)
  for (int i = 0; i < samples - 1; i++) {
    for (int j = i + 1; j < samples; j++) {
      if (vals[j] < vals[i]) {
        float t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
      }
    }
  }

  float voltage = vals[samples / 2];
  if (voltage <= 0.001f) return NAN;

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
  float t = readThermistorFiltered(channelTempIn);
  if (!isnan(t) && t > -20.0f && t < 125.0f) {
    lastValidTempIn = t;
    return t;
  }
  // Devolver último bueno si la lectura es inválida/espuria
  return lastValidTempIn;
}

float Sensors::getTempOut() {
  float t = readThermistorFiltered(channelTempOut);
  if (!isnan(t) && t > -20.0f && t < 125.0f) {
    lastValidTempOut = t;
    return t;
  }
  return lastValidTempOut;
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
  xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
  int16_t adcValue = ads.readADC_SingleEnded(2);
  xSemaphoreGive(getI2CMutex());
  float voltage = adcToVoltage(adcValue);
  return voltage;
}
