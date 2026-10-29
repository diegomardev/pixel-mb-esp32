#pragma once
#include <Arduino.h>
class Adafruit_GFX {
 protected:
  const int16_t WIDTH, HEIGHT;
  int16_t _width, _height;
  uint8_t rotation = 0;
 public:
  Adafruit_GFX(int16_t w,int16_t h):WIDTH(w),HEIGHT(h),_width(w),_height(h){}
  virtual ~Adafruit_GFX() = default;
  virtual void drawPixel(int16_t,int16_t,uint16_t) = 0;
  int16_t width() const { return _width; }
  int16_t height() const { return _height; }
};

struct GFXglyph {
  uint16_t bitmapOffset;
  uint8_t width, height, xAdvance;
  int8_t xOffset, yOffset;
};
struct GFXfont {
  uint8_t *bitmap;
  GFXglyph *glyph;
  uint16_t first, last;
  uint8_t yAdvance;
};
