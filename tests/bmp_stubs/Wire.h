#pragma once
#include <Arduino.h>
struct TwoWire {
  int sda=-1,scl=-1,expectedSda=21,expectedScl=22;
  uint8_t address=0,expectedAddress=0x76,id=0x58;
  bool online=true;
  void begin(int a,int b){sda=a;scl=b;}
  void setClock(uint32_t){} void setClockStretchLimit(uint32_t){} void setTimeOut(uint32_t){}
  void beginTransmission(uint8_t a){address=a;}
  void write(uint8_t){}
  bool responds(uint8_t a)const{return online&&a==expectedAddress&&sda==expectedSda&&scl==expectedScl;}
  uint8_t endTransmission(bool){return responds(address)?0:2;}
  uint8_t requestFrom(uint8_t a,uint8_t){return responds(a)?1:0;}
  int read(){return id;}
};
extern TwoWire Wire;
