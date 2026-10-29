#include <Arduino.h>
#include <LittleFS.h>
#include "DefaultStore.h"
#include "DefaultAnimations.h"

bool ensureAnimationDirectory() {
  if (LittleFS.exists("/animations")) {
    File dir = LittleFS.open("/animations", "r"); return dir && dir.isDirectory();
  }
  return LittleFS.mkdir("/animations");
}
bool installDefaultAnimations() {
  if (!ensureAnimationDirectory()) return false;
  for (const auto &entry : DEFAULT_ANIMATIONS) {
    if(LittleFS.exists(String("/defaults-v")+entry.pack+".done"))continue;
    String target = String("/animations/") + entry.name + ".awx";
    if (LittleFS.exists(target)) continue; // Nunca sustituir dibujos del usuario.
    File tmp = LittleFS.open("/seed.tmp", "w");
    if (!tmp) return false;
    bool written = true;
    uint8_t block[128];
    for (size_t offset = 0; offset < entry.size; offset += sizeof(block)) {
      const size_t count = min(sizeof(block), entry.size - offset);
      memcpy_P(block, entry.data + offset, count);
      if (tmp.write(block, count) != count) { written = false; break; }
      yield();
    }
    tmp.flush(); tmp.close();
    if (!written || !LittleFS.rename("/seed.tmp", target)) { LittleFS.remove("/seed.tmp"); return false; }
  }
  for(uint8_t pack=1;pack<=3;++pack){
    const String path=String("/defaults-v")+pack+".done";
    if(LittleFS.exists(path))continue;
    File marker=LittleFS.open("/defaults.tmp","w");if(!marker)return false;
    bool written=marker.write(uint8_t(1))==1;marker.flush();marker.close();
    if(!written || !LittleFS.rename("/defaults.tmp",path)){LittleFS.remove("/defaults.tmp");return false;}
  }
  return true;
}
