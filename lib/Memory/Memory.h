#ifndef MEMORY_H
#define MEMORY_H

#include <EEPROM.h>

class Memory {
private:
  static constexpr int EEPROM_SIZE = 250;     
  static constexpr int SSID_ADDRESS = 0;     
  static constexpr int PASS_ADDRESS = 75;     
  static constexpr int BOILER_ID_ADDRESS = 150; 

public:
  static void initialize();

  static bool verifyContent();
  static String getSSID();
  static String getPassword();
  static String getBoilerId();

  static void write(const String& ssid, const String& password);
  static void writeBoilerId(const String& boilerId);

  static void clear();
};

#endif
