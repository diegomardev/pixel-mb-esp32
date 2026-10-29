#pragma once
#include <stdint.h>
#include <stdio.h>
#include "ClockText.h"

constexpr uint32_t CONNECTION_IP_NOTICE_MS = 4000;
class ConnectionIpNotice {
 public:
  void request(uint8_t a,uint8_t b,uint8_t c,uint8_t d) {
    snprintf(text_,sizeof(text_),"%u.%u.%u.%u",a,b,c,d);
    pending_=true;active_=false;
  }
  void cancel(){pending_=active_=false;}
  bool tick(uint32_t now){
    if(pending_){pending_=false;active_=true;started_=now;}
    if(active_ && uint32_t(now-started_)>=CONNECTION_IP_NOTICE_MS){active_=false;return true;}
    return false;
  }
  bool active()const{return active_;}
  bool pending()const{return pending_;}
  uint32_t elapsed(uint32_t now)const{return uint32_t(now-started_);}
  const char *text()const{return text_;}
 private:
  bool pending_=false,active_=false;
  uint32_t started_=0;
  char text_[16]={};
};
inline int16_t connectionIpWidth(const char *text){
  int16_t width=0;
  for(const char *p=text;*p;++p)width+=*p=='.'?2:4;
  return width?width-1:0;
}
template<class Display>
void drawConnectionIp(Display &display,const char *text,uint32_t elapsed,uint16_t color){
  const int16_t width=connectionIpWidth(text),screen=display.width();
  int16_t x=width<=screen?(screen-width)/2:screen-int32_t(uint32_t(width+screen)*elapsed/CONNECTION_IP_NOTICE_MS);
  const int16_t y=(display.height()-5)/2;
  for(const char *p=text;*p;++p){
    if(*p=='.'){
      if(x>=0 && x<screen)display.drawPixel(x,y+4,color);
      x+=2;
    }else if(*p>='0' && *p<='9'){
      for(uint8_t row=0;row<5;++row)for(uint8_t column=0;column<3;++column)
        if(x+column>=0 && x+column<screen && (CLOCK_DIGITS[*p-'0'][row] & (4>>column)))display.drawPixel(x+column,y+row,color);
      x+=4;
    }
  }
}
