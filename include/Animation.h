#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "Config.h"

// Formatos AWX1/AWX2 little-endian: magic[4], w:u8, h:u8, frames:u16,
// frameMs:u16, reserved:u16(0). AWX1: RGB565 por filas. AWX2: cada frame
// lleva length:u16 (0 = crudo; otro valor = bytes de runs cantidad:u8/color:u16).
struct AnimationInfo {
  bool compressed = false;
  uint8_t width = 0, height = 0;
  uint16_t frames = 0, frameMs = 0;
};
bool validName(const String &name);
String animationPath(const String &name);
bool inspectAnimation(File &file, AnimationInfo &info);
class AnimationPlayer {
 public:
  bool open(const String &name);
  void close();
  bool tick(uint32_t now);
  const AnimationInfo &info() const { return info_; }
  const uint16_t *pixels() const { return pixels_; }
  const String &name() const { return name_; }
  bool active() const { return bool(file_); }
  uint16_t frameIndex() const { return frame_; }
 private:
  File file_;
  AnimationInfo info_;
  String name_;
  uint16_t pixels_[MATRIX_WIDTH * MATRIX_HEIGHT] = {};
  uint32_t offsets_[MAX_FRAMES] = {};
  uint16_t frame_ = 0;
  uint32_t last_ = 0;
  bool first_ = true;
};
