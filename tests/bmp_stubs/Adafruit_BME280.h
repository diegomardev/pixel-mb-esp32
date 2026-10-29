#pragma once
#include <Wire.h>
struct BmeReadings{float temp=21.34F,pressurePa=101234,humidity=46.78F;bool beginOK=true;};
extern BmeReadings bmeReadings;
class Adafruit_BME280 {
 public:
  enum sensor_mode{MODE_NORMAL};enum sensor_sampling{SAMPLING_X2,SAMPLING_X4};
  enum sensor_filter{FILTER_X4};enum standby_duration{STANDBY_MS_500};
  bool begin(uint8_t address,TwoWire *wire){return bmeReadings.beginOK&&wire->responds(address)&&wire->id==0x60;}
  void setSampling(sensor_mode,sensor_sampling,sensor_sampling,sensor_sampling,sensor_filter,standby_duration){}
  float readTemperature(){return bmeReadings.temp;}float readPressure(){return bmeReadings.pressurePa;}float readHumidity(){return bmeReadings.humidity;}
};
