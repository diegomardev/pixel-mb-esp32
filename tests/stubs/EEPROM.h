#pragma once
#include <array>
struct FakeEEPROM {
  std::array<uint8_t,512> bytes;
  bool commitOK = true;
  FakeEEPROM() { bytes.fill(255); }
  void begin(size_t) {}
  uint8_t read(size_t i) { return bytes.at(i); }
  void write(size_t i, uint8_t value) { bytes.at(i) = value; }
  bool commit() { return commitOK; }
  bool end() { return true; }
};
extern FakeEEPROM EEPROM;
