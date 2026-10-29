#pragma once
#include <stdint.h>
class ButtonInput {
 public:
  bool pressed(uint32_t now,bool down){
    if(down!=raw_){raw_=down;changedAt_=now;}
    if(down==stable_||uint32_t(now-changedAt_)<35)return false;
    stable_=down;return stable_;
  }
 private: bool raw_=false,stable_=false;uint32_t changedAt_=0;
};
