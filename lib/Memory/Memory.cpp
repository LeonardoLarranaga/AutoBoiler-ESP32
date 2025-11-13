#include "Memory.h"

void Memory::initialize() {
  EEPROM.begin(EEPROM_SIZE);
}

bool Memory::verifyContent() {
  return  EEPROM.readString(SSID_ADDRESS).length() != 0 && 
          EEPROM.readString(PASS_ADDRESS).length() != 0; 
}

String Memory::getSSID() {
  return EEPROM.readString(SSID_ADDRESS);
}

String Memory::getPassword() {
  return EEPROM.readString(PASS_ADDRESS);
}

String Memory::getBoilerId() {
  return EEPROM.readString(BOILER_ID_ADDRESS);
}

void Memory::write(const String& ssid, const String& password) {
  EEPROM.begin(EEPROM_SIZE);

  for (int i = 0; i < ssid.length(); i++) EEPROM.write(SSID_ADDRESS + i, ssid[i]);
  EEPROM.write(SSID_ADDRESS + ssid.length(), '\0');

  for (int i = 0; i < password.length(); i++) EEPROM.write(PASS_ADDRESS + i, password[i]);
  EEPROM.write(PASS_ADDRESS + password.length(), '\0');

  EEPROM.commit();
}

void Memory::writeBoilerId(const String& boilerId) {
  EEPROM.begin(EEPROM_SIZE);

  for (int i = 0; i < boilerId.length(); i++) EEPROM.write(BOILER_ID_ADDRESS + i, boilerId[i]);
  EEPROM.write(BOILER_ID_ADDRESS + boilerId.length(), '\0');

  EEPROM.commit();
}

void Memory::clear() {
  EEPROM.writeString(SSID_ADDRESS, "");
  EEPROM.writeString(PASS_ADDRESS, "");
  EEPROM.writeString(BOILER_ID_ADDRESS, "");
  EEPROM.commit();
}
