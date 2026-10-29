#pragma once
#include <stdint.h>

constexpr uint32_t BOOT_LOGO_DURATION_MS = 3200;
// Pixel-MB: eight 3x5 glyphs and seven one-pixel gaps, total 31 columns.
constexpr uint8_t BOOT_LOGO_GLYPHS[8][5] = {
  {6,5,6,4,4}, // P
  {2,0,2,2,2}, // i
  {0,5,2,2,5}, // x
  {0,3,5,6,3}, // e
  {2,2,2,2,3}, // l
  {0,0,7,0,0}, // -
  {5,7,7,5,5}, // M
  {6,5,6,5,6}  // B
};
inline uint32_t bootLogoRgb(uint8_t hue, uint8_t level) {
  uint8_t r=0,g=0,b=0;
  if(hue<85){r=255-hue*3;g=hue*3;}
  else if(hue<170){hue-=85;g=255-hue*3;b=hue*3;}
  else {hue-=170;b=255-hue*3;r=hue*3;}
  return (uint32_t(r)*level/255<<16)|(uint32_t(g)*level/255<<8)|(uint32_t(b)*level/255);
}
template<class Display>
void drawBootLogo(Display &display,uint32_t elapsed) {
  if(display.width()<31 || display.height()<7 || elapsed>=BOOT_LOGO_DURATION_MS)return;
  const int16_t left=(display.width()-31)/2,top=(display.height()-5)/2;
  const uint8_t visible=elapsed<400?1+elapsed*30/400:31;
  const uint8_t phase=(elapsed/6)%256;
  uint8_t level=180+(phase<128?phase:255-phase)*75/127;
  if(elapsed>2800)level=uint32_t(level)*(BOOT_LOGO_DURATION_MS-elapsed)/400;
  for(uint8_t letter=0;letter<8;++letter)for(uint8_t y=0;y<5;++y)for(uint8_t x=0;x<3;++x){
    const uint8_t column=letter*4+x;
    if(column<visible && (BOOT_LOGO_GLYPHS[letter][y]&(4>>x))){
      const uint32_t rgb=bootLogoRgb(uint8_t(column*8+elapsed/9),level);
      display.drawPixel(left+column,top+y,display.Color(rgb>>16,rgb>>8,rgb));
    }
  }
  const uint8_t cursor=(elapsed/35)%31;
  const uint32_t rgb=bootLogoRgb(uint8_t(elapsed/5),level);
  display.drawPixel(left+cursor,top+5,display.Color(rgb>>16,rgb>>8,rgb));
}
