// Enlaza el cpp REAL de Adafruit NeoMatrix; solo GFX/NeoPixel se sustituyen.
#include "PixelMatrix.h"
#include <cassert>
#include <iostream>
int main() {
  PixelMatrix matrix;
  assert(matrix.configurationValid()); assert(matrix.numPixels()==256);
  assert(matrix.getPin()==32 && matrix.width()==32 && matrix.height()==8);
  matrix.fillScreen(0);
  matrix.drawPixel(0,0,0xffff); assert(matrix.getPixelColor(0)!=0);
  matrix.drawPixel(1,0,0xffff); assert(matrix.getPixelColor(15)!=0);
  matrix.fillScreen(0);
  for(int y=0;y<8;y++)for(int x=0;x<32;x++)matrix.drawPixel(x,y,0xffff);
  for(int i=0;i<256;i++)assert(matrix.getPixelColor(i)!=0);
  std::cout<<"Matriz ESP32: 256 LED/GPIO32 y mapeo 32x8\n";
}
