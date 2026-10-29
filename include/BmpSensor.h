#pragma once
#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include "Config.h"

class BmpSensor {
 public:
  void tick(uint32_t now);
  bool detected() const { return detected_; }
  bool valid() const { return valid_; }
  uint8_t address() const { return address_; }
  uint8_t sdaPin() const { return sda_; }
  uint8_t sclPin() const { return scl_; }
  uint8_t observedChipId() const { return observedId_; }
  bool isBme() const { return bmeModel_; }
  bool humidityValid() const { return valid_ && bmeModel_; }
  float humidity() const { return humidity_; }
  float temperatureC() const { return temperature_; }
  float pressureHpa() const { return pressure_; }
  uint32_t sampledAt() const { return sampledAt_; }
  const char *error() const { return error_; }
 private:
  bool probe();
  bool readChipId(uint8_t address, uint8_t &id);
  Adafruit_BMP280 bmp_{&Wire};
  Adafruit_BME280 bme_;
  bool bmeModel_=false;
  float humidity_=0;
  bool detected_ = false, valid_ = false, attempted_ = false;
  uint8_t address_ = 0, sda_ = BMP280_SDA_PIN, scl_ = BMP280_SCL_PIN, observedId_ = 0;
  uint32_t lastAttempt_ = 0, lastRead_ = 0, sampledAt_ = 0;
  float temperature_ = 0, pressure_ = 0;
  const char *error_ = "Todavia no detectado";
};
