#pragma once
#include <Wire.h>
#define BMP280_CHIPID 0x58
struct BmpReadings { float temp=23.46F,pressurePa=100234; bool beginOK=true; };
extern BmpReadings readings;
class Adafruit_BMP280 {
 public:
  enum sensor_mode{MODE_NORMAL};enum sensor_sampling{SAMPLING_X2,SAMPLING_X4};
  enum sensor_filter{FILTER_X4};enum standby_duration{STANDBY_MS_500};
  explicit Adafruit_BMP280(TwoWire*){}
  bool begin(uint8_t a,uint8_t id){return readings.beginOK&&Wire.responds(a)&&Wire.id==id;}
  void setSampling(sensor_mode,sensor_sampling,sensor_sampling,sensor_filter,standby_duration){}
  float readTemperature(){return readings.temp;}float readPressure(){return readings.pressurePa;}
};
