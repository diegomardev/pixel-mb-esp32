#include "Animation.h"
#include "AnimationRle.h"

static uint16_t le16(const uint8_t *p) { return p[0] | (uint16_t(p[1]) << 8); }
bool validName(const String &name) {
  if (!name.length() || name.length() > 20) return false;
  for (size_t i = 0; i < name.length(); ++i) {
    const char c = name[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return true;
}
String animationPath(const String &name) { return "/animations/" + name + ".awx"; }
bool inspectAnimation(File &file, AnimationInfo &info) {
  uint8_t header[12];
  if (!file || file.size() < sizeof(header) || !file.seek(0) ||
      file.read(header, sizeof(header)) != sizeof(header)) return false;
  info.compressed=!memcmp(header,"AWX2",4);
  if ((!info.compressed && memcmp(header,"AWX1",4)) || header[10] || header[11]) return false;
  info.width = header[4]; info.height = header[5];
  info.frames = le16(header + 6); info.frameMs = le16(header + 8);
  if (!info.width || info.width > MATRIX_WIDTH || !info.height ||
      info.height > MATRIX_HEIGHT || !info.frames || info.frames > MAX_FRAMES ||
      info.frameMs < 20 || info.frameMs > 5000) return false;
  const uint32_t expected = 12UL + uint32_t(info.width) * info.height * info.frames * 2UL;
  if(!info.compressed)return expected==file.size();
  uint32_t offset=12;uint8_t buffer[MATRIX_WIDTH*MATRIX_HEIGHT*3];
  const size_t pixels=size_t(info.width)*info.height;
  for(uint16_t frame=0;frame<info.frames;++frame){
    uint8_t prefix[2];if(!file.seek(offset) || file.read(prefix,2)!=2)return false;
    const uint16_t length=le16(prefix);const size_t bytes=length?length:pixels*2;
    offset+=2;if(bytes>sizeof(buffer) || offset+bytes>file.size())return false;
    if(length && (file.read(buffer,bytes)!=int(bytes) || !decodeAnimationRuns(buffer,bytes,nullptr,pixels)))return false;
    offset+=bytes;yield();
  }
  return offset==file.size();
}
bool AnimationPlayer::open(const String &name) {
  // Validar antes de sustituir el archivo activo.
  File next = LittleFS.open(animationPath(name), "r");
  AnimationInfo nextInfo;
  if (!inspectAnimation(next, nextInfo)) return false;
  close();
  file_ = next; info_ = nextInfo; name_ = name;
  if(info_.compressed){
    uint32_t offset=12;
    for(uint16_t frame=0;frame<info_.frames;++frame){
      offsets_[frame]=offset;uint8_t prefix[2];
      if(!file_.seek(offset) || file_.read(prefix,2)!=2){close();return false;}
      const uint16_t length=le16(prefix);offset+=2+(length?length:uint32_t(info_.width)*info_.height*2);
    }
  }
  frame_ = 0; last_ = 0; first_ = true;
  return true;
}
void AnimationPlayer::close() { file_.close(); name_ = ""; }
bool AnimationPlayer::tick(uint32_t now) {
  if (!file_) return false;
  if (!first_ && uint32_t(now - last_) < info_.frameMs) return false;
  if (!first_) frame_ = (frame_ + uint32_t(now - last_) / info_.frameMs) % info_.frames;
  const size_t count = size_t(info_.width) * info_.height;
  bool read=false;
  if(info_.compressed){
    uint8_t prefix[2];
    if(file_.seek(offsets_[frame_]) && file_.read(prefix,2)==2){
      const uint16_t length=le16(prefix);
      if(!length)read=file_.read(reinterpret_cast<uint8_t *>(pixels_),count*2)==int(count*2);
      else if(length<=MATRIX_WIDTH*MATRIX_HEIGHT*3){
        uint8_t buffer[MATRIX_WIDTH*MATRIX_HEIGHT*3];
        read=file_.read(buffer,length)==length && decodeAnimationRuns(buffer,length,pixels_,count);
      }
    }
  }else read=file_.seek(12UL+uint32_t(frame_)*count*2) && file_.read(reinterpret_cast<uint8_t *>(pixels_),count*2)==int(count*2);
  if(!read){close();return false;}
  // ESP32 es little-endian como el archivo.
  if (first_) last_ = now;
  else last_ += uint32_t(now - last_) / info_.frameMs * info_.frameMs;
  first_ = false;
  return true;
}
