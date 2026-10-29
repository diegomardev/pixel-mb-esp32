#include "BootLogo.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
struct Canvas {
  uint16_t pixels[8][32]={};
  int16_t width()const{return 32;}
  int16_t height()const{return 8;}
  static uint16_t Color(uint8_t r,uint8_t g,uint8_t b){return (uint16_t(r&0xf8)<<8)|(uint16_t(g&0xfc)<<3)|(b>>3);}
  void drawPixel(int16_t x,int16_t y,uint16_t color){assert(x>=0&&x<32&&y>=0&&y<8);pixels[y][x]=color;}
};
int main(int argc,char **argv){
  Canvas full;drawBootLogo(full,400);
  for(int letter=0;letter<8;++letter)for(int y=0;y<5;++y)for(int x=0;x<3;++x)
    assert(bool(full.pixels[1+y][letter*4+x])==bool(BOOT_LOGO_GLYPHS[letter][y]&(4>>x)));
  Canvas later;drawBootLogo(later,900);assert(memcmp(full.pixels,later.pixels,sizeof(full.pixels))!=0);
  Canvas ended;drawBootLogo(ended,BOOT_LOGO_DURATION_MS);
  for(const auto &row:ended.pixels)for(auto pixel:row)assert(pixel==0);
  std::ofstream frames;if(argc>1)frames.open(argv[1],std::ios::binary);
  for(uint32_t t=0;t<BOOT_LOGO_DURATION_MS;t+=40){Canvas frame;drawBootLogo(frame,t);if(frames)for(const auto &row:frame.pixels)for(auto pixel:row){frames.put(pixel&255);frames.put(pixel>>8);}}
  std::cout<<"Boot logo: Pixel-MB glyphs, boundaries, color animation, end and all frames passed\n";
}
