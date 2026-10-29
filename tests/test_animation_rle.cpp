#include "AnimationRle.h"
#include <assert.h>
#include <iostream>
int main(){
 uint16_t pixels[256]={};
 uint8_t runs[]={255,0x34,0x12,1,0xcd,0xab};
 assert(decodeAnimationRuns(runs,sizeof(runs),pixels,256));
 assert(pixels[0]==0x1234 && pixels[254]==0x1234 && pixels[255]==0xabcd);
 assert(decodeAnimationRuns(runs,sizeof(runs),nullptr,256));
 assert(!decodeAnimationRuns(runs,5,pixels,256));
 assert(!decodeAnimationRuns(runs,6,pixels,255));
 assert(!decodeAnimationRuns(runs,3,pixels,256));
 uint8_t zero[]={0,1,2};assert(!decodeAnimationRuns(zero,3,pixels,1));
 std::cout<<"RLE: decodificacion, limite 256, truncado, exceso y run vacio correctos\n";
}
