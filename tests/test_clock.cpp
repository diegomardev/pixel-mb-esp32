// Prueba el dibujo REAL del reloj: ancho fijo, separadores y celdas estables.
#include "ClockText.h"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
struct Canvas {
  bool pixels[8][32] = {};
  int16_t width() const { return 32; }
  int16_t height() const { return 8; }
  void drawPixel(int16_t x,int16_t y,uint16_t color) {
    assert(x>=0 && x<32 && y>=0 && y<8); pixels[y][x]=color!=0;
  }
};
static Canvas render(const char *text,bool seconds) {
  Canvas canvas; drawFixedClock(canvas,text,seconds,0xffff);
  int first=32,last=-1,top=8,bottom=-1;
  for(int y=0;y<8;y++)for(int x=0;x<32;x++)if(canvas.pixels[y][x]) {
    first=std::min(first,x);last=std::max(last,x);
    top=std::min(top,y);bottom=std::max(bottom,y);
  }
  const int expectedWidth=seconds?27:17;
  assert(last-first+1==expectedWidth && first==(32-expectedWidth)/2);
  assert(top>=1 && bottom<=5);
  // El separador tras las horas se queda siempre en la misma columna.
  const int colon=(32-expectedWidth)/2+8;
  assert(canvas.pixels[2][colon] && canvas.pixels[4][colon]);
  return canvas;
}
int main() {
  char text[9];
  formatClockText(text,nullptr,false);assert(!strcmp(text,"--:--"));render(text,false);
  formatClockText(text,nullptr,true);assert(!strcmp(text,"--:--:--"));render(text,true);
  tm value={};
  for(int h=0;h<24;++h)for(int m=0;m<60;++m)for(int s=0;s<60;++s) {
    value.tm_hour=h;value.tm_min=m;value.tm_sec=s;
    formatClockText(text,&value,true);assert(strlen(text)==8);render(text,true);
    formatClockText(text,&value,false);assert(strlen(text)==5);render(text,false);
  }
  // Cambiar 00 por 11 segundos no puede mover ni alterar HH:MM ni los puntos.
  auto zero=render("11:11:00",true),one=render("11:11:11",true);
  for(int y=0;y<8;y++)for(int x=0;x<22;x++)assert(zero.pixels[y][x]==one.pixels[y][x]);
  // Cambiar minutos no desplaza las horas ni los segundos.
  auto a=render("11:00:11",true),b=render("11:11:11",true);
  for(int y=0;y<8;y++)for(int x=0;x<32;x++)if(x<12||x>=19)assert(a.pixels[y][x]==b.pixels[y][x]);
  formatClockText(text,&value,true);assert(!strcmp(text,"23:59:59"));
  std::cout<<"Reloj: 86400 horas, ancho constante 17/27 px, digitos y separadores inmoviles\n";
}
