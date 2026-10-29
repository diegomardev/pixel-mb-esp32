#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

enum class RotationPage : uint8_t { Clock, Temperature, Pressure, Animation, Random, Text };
constexpr uint8_t MAX_ROTATION_ITEMS = 16;
constexpr size_t MAX_ROTATION_TEXT_BYTES = 64;
inline const char *rotationPageName(RotationPage page) {
  return page == RotationPage::Clock ? "clock" : page == RotationPage::Temperature ? "temperature" :
    page == RotationPage::Pressure ? "pressure" : page == RotationPage::Random ? "random" : page == RotationPage::Text ? "text" : "animation";
}
constexpr uint32_t defaultRotationColor(RotationPage page) {
  return page==RotationPage::Clock?0x00b4ff:page==RotationPage::Temperature?0xff8020:
    page==RotationPage::Pressure?0x00c8ff:0x00ff00;
}
struct RotationItem {
  RotationPage type = RotationPage::Clock;
  uint16_t seconds = 10;
  char animation[21] = {};
  char text[MAX_ROTATION_TEXT_BYTES + 1] = {};
  uint32_t color = 0x00ff00;
  bool showSeconds = false;
  bool showWeekdays = true;
};
inline bool setRotationText(RotationItem &item, const char *text, size_t length) {
  if (!text || length > MAX_ROTATION_TEXT_BYTES) return false;
  for (size_t i = 0; i < length; ++i) if (static_cast<uint8_t>(text[i]) < 32) return false;
  memcpy(item.text, text, length); item.text[length] = 0; return true;
}
constexpr uint8_t MAX_RANDOM_ANIMATIONS = 48;
struct RotationConfig {
  uint8_t randomCount = 0;
  char randomAnimations[MAX_RANDOM_ANIMATIONS][21] = {};
  bool enabled = true;
  uint8_t count = 3;
  RotationItem items[MAX_ROTATION_ITEMS] = {
    {RotationPage::Clock,10,{},{},defaultRotationColor(RotationPage::Clock)},
    {RotationPage::Temperature,5,{},{},defaultRotationColor(RotationPage::Temperature)},
    {RotationPage::Pressure,5,{},{},defaultRotationColor(RotationPage::Pressure)}
  };
};
class RotationScheduler {
 public:
  void reset(uint32_t now) { index_ = 0; started_ = now; page_ = RotationPage::Clock; ++generation_; }
  RotationPage tick(uint32_t now, const RotationConfig &c, bool sensorValid) {
    if (!c.enabled || !c.count) { reset(now); return page_ = RotationPage::Clock; }
    if (index_ >= c.count) reset(now);
    if (!eligible(c.items[index_],sensorValid)) {
      if (!advance(c,sensorValid)) { reset(now); return page_ = RotationPage::Clock; }
      started_ = now;
    }
    uint32_t cycle=0;
    for (uint8_t i=0;i<c.count;++i) if(eligible(c.items[i],sensorValid)) cycle+=uint32_t(c.items[i].seconds)*1000;
    if (!cycle) { reset(now); return page_ = RotationPage::Clock; }
    uint32_t elapsed=now-started_;
    if(elapsed>=cycle){const uint32_t skipped=elapsed/cycle*cycle;started_+=skipped;elapsed-=skipped;++generation_;}
    while(elapsed>=duration(c)){
      const uint32_t period=duration(c);started_+=period;elapsed-=period;++generation_;
      if(!advance(c,sensorValid))return page_ = RotationPage::Clock;
    }
    return page_ = c.items[index_].type;
  }
  void step(uint32_t now,const RotationConfig &c,bool sensorValid,int direction){
    if(!c.enabled||!c.count){reset(now);return;}
    if(index_>=c.count)index_=0;
    for(uint8_t n=0;n<c.count;++n){
      index_=(index_+c.count+(direction<0?-1:1))%c.count;
      if(eligible(c.items[index_],sensorValid)){started_=now;page_=c.items[index_].type;++generation_;return;}
    }
    reset(now);
  }
  uint8_t index() const { return index_; }
  RotationPage page(const RotationConfig &) const { return page_; }
  uint32_t generation() const { return generation_; }
  uint32_t remainingSeconds(uint32_t now,const RotationConfig &c) const {
    const uint32_t elapsed=now-started_,period=duration(c);
    return elapsed>=period?0:(period-elapsed+999)/1000;
  }
 private:
  static bool eligible(const RotationItem &item,bool valid) {
    return valid || (item.type!=RotationPage::Temperature && item.type!=RotationPage::Pressure);
  }
  bool advance(const RotationConfig &c,bool valid) {
    for(uint8_t n=0;n<c.count;++n){index_=(index_+1)%c.count;if(eligible(c.items[index_],valid))return true;}
    return false;
  }
  uint32_t duration(const RotationConfig &c) const { return uint32_t(c.items[index_ < c.count ? index_ : 0].seconds)*1000; }
  RotationPage page_=RotationPage::Clock;
  uint8_t index_=0;
  uint32_t started_=0, generation_=0;
};
