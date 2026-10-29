#include "ConnectionIpNotice.h"
#include <assert.h>
#include <string.h>
#include <iostream>
struct Canvas {
  uint16_t pixels[8][32]={};
  int16_t width()const{return 32;}int16_t height()const{return 8;}
  void drawPixel(int16_t x,int16_t y,uint16_t color){assert(x>=0&&x<32&&y>=0&&y<8);pixels[y][x]=color;}
};
int main(){
  ConnectionIpNotice n;n.request(10,205,17,152);
  assert(n.pending()&&!n.active());assert(!strcmp(n.text(),"192.168.1.50"));
  assert(!n.tick(5000)&&n.active());assert(!n.tick(8999)&&n.active());assert(n.tick(9000)&&!n.active());assert(!n.tick(9001));
  n.request(255,255,255,255);assert(strlen(n.text())==15);n.cancel();assert(!n.active()&&!n.pending());
  n.request(1,1,1,1);assert(!n.tick(0xfffffff0U));assert(n.tick(uint32_t(0xfffffff0U+4000U)));
  assert(connectionIpWidth("1.1.1.1")==21);assert(connectionIpWidth("255.255.255.255")==53);
  for(const char *text:{"192.168.1.50","255.255.255.255","1.1.1.1"}){
    bool seen=false;
    for(uint32_t ms=0;ms<4000;ms+=20){Canvas c;drawConnectionIp(c,text,ms,1);for(auto &row:c.pixels)for(auto pixel:row)seen|=pixel!=0;}
    assert(seen);
  }
  std::cout<<"IP: 4 segundos, reconexion, cancelacion, millis, ancho y todos los fotogramas sin desbordes correctos\n";
}
