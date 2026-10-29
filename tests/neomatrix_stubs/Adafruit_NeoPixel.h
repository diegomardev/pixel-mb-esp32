#pragma once
#include <Arduino.h>
#include <vector>
using neoPixelType = uint16_t;
#define NEO_GRB 82
#define NEO_KHZ800 0
class Adafruit_NeoPixel {
  uint16_t count_;
  int16_t pin_;
  mutable std::vector<uint32_t> pixels_;
 public:
  Adafruit_NeoPixel(uint16_t n,int16_t pin,neoPixelType):count_(n),pin_(pin),pixels_(n){}
  uint16_t numPixels() const { return count_; }
  int16_t getPin() const { return pin_; }
  uint8_t *getPixels() const { return reinterpret_cast<uint8_t*>(pixels_.data()); }
  void setPixelColor(uint16_t i,uint32_t color) { if(i<count_)pixels_[i]=color; }
  uint32_t getPixelColor(uint16_t i) const { return pixels_.at(i); }
};
