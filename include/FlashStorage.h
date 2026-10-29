#pragma once
#include <LittleFS.h>
#include <esp_partition.h>
class AnimationDirectory {
 public:
  explicit AnimationDirectory(const char *path):root_(LittleFS.open(path)){}
  bool next(){if(!root_||!root_.isDirectory())return false;current_=root_.openNextFile();return bool(current_);}
  String fileName()const{return current_?String(current_.name()):String();}
  File openFile(const char *)const{return current_;}
 private: File root_,current_;
};
inline bool mountAnimationStorage(){
  if(LittleFS.begin(false))return true;
  const esp_partition_t *part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
  if(!part)return false;
  uint8_t block[256];
  for(size_t offset=0;offset<part->size;offset+=sizeof(block)){
    const size_t count=part->size-offset<sizeof(block)?part->size-offset:sizeof(block);
    if(esp_partition_read(part,offset,block,count)!=ESP_OK)return false;
    for(size_t i=0;i<count;++i)if(block[i]!=0xff)return false;
    yield();
  }
  // Virgin partition only. Existing or damaged files are never autoformatted.
  return LittleFS.format()&&LittleFS.begin(false);
}
