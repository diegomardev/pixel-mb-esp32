#include "Buttons.h"
#include "Rotation.h"
#include <cassert>
#include <iostream>
int main(){
  ButtonInput b;assert(!b.pressed(0,false));assert(!b.pressed(100,true));assert(!b.pressed(110,false));assert(!b.pressed(120,true));assert(!b.pressed(154,true));assert(b.pressed(155,true));assert(!b.pressed(1000,true));assert(!b.pressed(1010,false));assert(!b.pressed(1050,false));assert(!b.pressed(1060,true));assert(b.pressed(1095,true));
  ButtonInput wrap;assert(!wrap.pressed(0xfffffff0,true));assert(wrap.pressed(20,true));
  RotationConfig c;RotationScheduler s;s.reset(0);s.tick(0,c,true);s.step(1,c,true,1);assert(s.index()==1);s.step(2,c,true,-1);assert(s.index()==0);s.step(3,c,true,-1);assert(s.index()==2);s.step(4,c,false,1);assert(s.index()==0);s.step(5,c,false,-1);assert(s.index()==0);
  c.count=0;s.step(6,c,true,1);assert(s.index()==0);
  std::cout<<"Buttons: debounce, held presses, wraparound and bidirectional rotation passed\n";
}
