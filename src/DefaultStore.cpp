#include <Arduino.h>
#include <LittleFS.h>
#include "DefaultStore.h"
#include "DefaultAnimations.h"
#include <esp32/rom/miniz.h>
#include <memory>

bool ensureAnimationDirectory() {
  if (LittleFS.exists("/animations")) {
    File dir = LittleFS.open("/animations", "r"); return dir && dir.isDirectory();
  }
  return LittleFS.mkdir("/animations");
}
bool installDefaultAnimations() {
  if (!ensureAnimationDirectory()) return false;
  uint8_t installed = 0;
  const bool previousInstallation = LittleFS.exists("/defaults-v1.done") || LittleFS.exists("/defaults-v2.done") || LittleFS.exists("/defaults-v3.done");
  for (const auto &entry : DEFAULT_ANIMATIONS) {
    if(entry.pack == 4 && previousInstallation) continue;
    if(LittleFS.exists(String("/defaults-v")+entry.pack+".done"))continue;
    String target = String("/animations/") + entry.name + ".awx";
    if (LittleFS.exists(target)) continue; // Nunca sustituir dibujos del usuario.
    File tmp = LittleFS.open("/seed.tmp", "w");
    if (!tmp) return false;
    std::unique_ptr<uint8_t[]> decoded(new(std::nothrow) uint8_t[entry.rawSize]);
    if (!decoded) { tmp.close(); LittleFS.remove("/seed.tmp"); return false; }
    // El estado del descompresor supera la pila de loopTask: reservarlo en heap.
    std::unique_ptr<tinfl_decompressor> inflater(new(std::nothrow) tinfl_decompressor);
    if (!inflater) { tmp.close(); LittleFS.remove("/seed.tmp"); return false; }
    tinfl_init(inflater.get());
    size_t inputSize = entry.size, outputSize = entry.rawSize;
    const auto result = tinfl_decompress(inflater.get(), entry.data, &inputSize,
      decoded.get(), decoded.get(), &outputSize,
      TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (result != TINFL_STATUS_DONE || outputSize != entry.rawSize) {
      tmp.close(); LittleFS.remove("/seed.tmp"); return false;
    }
    bool written = true;
    for (size_t offset = 0; offset < entry.rawSize; offset += 1024) {
      const size_t count = min(size_t(1024), entry.rawSize - offset);
      if (tmp.write(decoded.get() + offset, count) != count) { written = false; break; }
      yield();
    }
    tmp.flush(); tmp.close();
    if (!written || !LittleFS.rename("/seed.tmp", target)) { LittleFS.remove("/seed.tmp"); return false; }
    ++installed;
  }
  for(uint8_t pack=1;pack<=4;++pack){
    const String path=String("/defaults-v")+pack+".done";
    if(LittleFS.exists(path))continue;
    File marker=LittleFS.open("/defaults.tmp","w");if(!marker)return false;
    bool written=marker.write(uint8_t(1))==1;marker.flush();marker.close();
    if(!written || !LittleFS.rename("/defaults.tmp",path)){LittleFS.remove("/defaults.tmp");return false;}
  }
  if (installed) Serial.printf("Animaciones predeterminadas instaladas: %u\n", installed);
  return true;
}
