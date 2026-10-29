#include "ClockText.h"
#include <assert.h>
#include <fstream>
#include <iostream>
struct Canvas {
  uint16_t pixels[8][32]={};
  int16_t width()const{return 32;}int16_t height()const{return 8;}
  void drawPixel(int16_t x,int16_t y,uint16_t color){assert(x>=0&&x<32&&y>=0&&y<8);pixels[y][x]=color;}
};
int main(int argc,char **argv){
  for(int weekday=0;weekday<7;++weekday){
    tm local={};local.tm_wday=weekday;
    assert(weekdayFromMonday(&local)==(weekday+6)%7);
    Canvas c;drawFixedClock(c,"17:19:50",true,123);
    drawWeekdayIndicators(c,&local,1,2);
    for(int x=0;x<32;++x){
      int day=(x-2)/4;bool mark=x>=2&&x<=28&&((x-2)%4)<3;
      assert(c.pixels[7][x]==(mark?(day==(weekday+6)%7?2:1):0));
      assert(c.pixels[6][x]==0);
    }
  }
  assert(weekdayFromMonday(nullptr)==-1);tm invalid={};invalid.tm_wday=7;assert(weekdayFromMonday(&invalid)==-1);
  Canvas unknown;drawWeekdayIndicators(unknown,nullptr,1,2);
  int count=0;for(auto pixel:unknown.pixels[7]){assert(pixel!=2);count+=pixel==1;}assert(count==21);
  if(argc>1){
    Canvas c;tm wednesday={};wednesday.tm_wday=3;
    drawFixedClock(c,"17:19:50",true,0x07e0);
    drawWeekdayIndicators(c,&wednesday,0x1910,0xffff);
    std::ofstream file(argv[1],std::ios::binary);file<<"P6\n32 8\n255\n";
    for(auto &row:c.pixels)for(auto color:row){file.put(((color>>11)&31)*255/31);file.put(((color>>5)&63)*255/63);file.put((color&31)*255/31);}
  }
  std::cout<<"Dias: lunes primero, domingo ultimo, 7 marcas, fila inferior separada del reloj y sin dia falso antes de NTP\n";
}
