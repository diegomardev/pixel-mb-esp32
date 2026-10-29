#pragma once
#include <stdint.h>
#include <stddef.h>
// AWX2: pares (cantidad:u8, color:u16 little-endian). No altera RGB565.
inline bool decodeAnimationRuns(const uint8_t *data,size_t size,uint16_t *pixels,size_t count){
  if(size%3)return false;
  size_t position=0;
  for(size_t i=0;i<size;i+=3){
    const uint8_t run=data[i];if(!run || position+run>count)return false;
    const uint16_t color=data[i+1] | uint16_t(data[i+2])<<8;
    for(uint8_t n=0;n<run;++n){if(pixels)pixels[position]=color;++position;}
  }
  return position==count;
}
