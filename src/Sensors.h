#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <FlowSensor.h>
#include <EmonLib.h> 

class Sensors {
private:
  Adafruit_ADS1115 ads;

  int channelTempIn;
  int channelTempOut;
  int channelFlow;
  int channelPower;

  float seriesResistor;
  float nominalResistance;
  float nominalTemp;
  float betaCoef;

  unsigned long lastFlowUpdate;
  float lastFlowRate;
  EnergyMonitor emon1;  
  int currentPin;
  double currentIrms;
  float voltageAC;  

  float adcToVoltage(int16_t raw);
  float readThermistor(int channel);

public:
  Sensors();

  bool begin();
  float getTempIn();
  float getTempOut();
  float getFlow();
  float getPower();     
  double getCurrent();  
};

#endif
