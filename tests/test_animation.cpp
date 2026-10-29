// Prueba el mismo src/Animation.cpp que se compila para ESP32.
#include "Animation.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
FakeFS LittleFS;
std::vector<uint8_t> readFile(const char *path) {
  std::ifstream in(path, std::ios::binary); assert(in);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool valid(std::vector<uint8_t> bytes) { File f(bytes); AnimationInfo info; return inspectAnimation(f, info); }
int main(int argc, char **argv) {
  assert(argc == 2);
  auto bytes = readFile(argv[1]); assert(valid(bytes));
  assert(validName("609") && validName("mi_icono-2"));
  assert(!validName("") && !validName("../file") && !validName("a/b") && !validName(std::string(21,'a')));
  for (size_t len=0; len<12; ++len) assert(!valid({bytes.begin(),bytes.begin()+len}));
  auto bad=bytes; bad[0]='X'; assert(!valid(bad));
  for (int w : {0,33,255}) {bad=bytes;bad[4]=w;assert(!valid(bad));}
  for (int h : {0,9,255}) {bad=bytes;bad[5]=h;assert(!valid(bad));}
  for (int count : {0,121,65535}) {bad=bytes;bad[6]=count&255;bad[7]=count>>8;assert(!valid(bad));}
  for (int ms : {0,19,5001,65535}) {bad=bytes;bad[8]=ms&255;bad[9]=ms>>8;assert(!valid(bad));}
  bad=bytes;bad[10]=1;assert(!valid(bad));
  bad=bytes;bad.pop_back();assert(!valid(bad));
  bad=bytes;bad.push_back(0);assert(!valid(bad));
  LittleFS.files["/animations/609.awx"] = bytes;
  AnimationPlayer p; assert(p.open("609")); assert(p.tick(100));
  auto first = p.pixels()[3]; assert(!p.tick(499)); assert(p.tick(500));
  assert(p.pixels()[3] != first); assert(!p.open("absent")); assert(p.active());
  assert(p.tick(900)); assert(p.pixels()[3] == first);
  // Salto de fotogramas si ha pasado más de un periodo.
  assert(p.tick(2100)); assert(p.pixels()[3] != first);
  p.close(); assert(!p.active() && !p.tick(2300));
  assert(p.open("609")); assert(p.tick(0xffffff00U));
  assert(p.tick(0x90U)); // desbordamiento de millis(): delta 400 ms
  assert(p.pixels()[3] != first);
  std::cout << "Animation.cpp: validación, reproducción, salto y overflow correctos\n";
}
