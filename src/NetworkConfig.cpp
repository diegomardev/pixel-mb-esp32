#include "NetworkConfig.h"
#include <EEPROM.h>
namespace {
constexpr size_t PAYLOAD_SIZE = 102, RECORD_SIZE = PAYLOAD_SIZE + 4;
uint32_t checksum(const uint8_t *data) {
  uint32_t result = 2166136261UL;
  for (size_t i = 0; i < PAYLOAD_SIZE; ++i) result = (result ^ data[i]) * 16777619UL;
  return result;
}
bool valid(const String &ssid, const String &password) {
  return ssid.length() >= 1 && ssid.length() <= 32 &&
    (password.length() == 0 || (password.length() >= 8 && password.length() <= 63));
}
}
bool loadNetworkConfig(String &ssid, String &password, NetworkAddress &address) {
  uint8_t data[RECORD_SIZE]; EEPROM.begin(512);
  for (size_t i = 0; i < RECORD_SIZE; ++i) data[i] = EEPROM.read(i);
  uint8_t extra[29];
  for (size_t i=0;i<sizeof(extra);++i) extra[i]=EEPROM.read(RECORD_SIZE+i);
  EEPROM.end();
  if (memcmp(data, "AWN1", 4) || !memchr(data + 4, 0, 33) || !memchr(data + 37, 0, 65)) return false;
  uint32_t stored = 0;
  for (uint8_t i = 0; i < 4; ++i) stored |= uint32_t(data[PAYLOAD_SIZE + i]) << (i * 8);
  if (checksum(data) != stored) return false;
  String nextSsid(reinterpret_cast<char *>(data + 4)), nextPass(reinterpret_cast<char *>(data + 37));
  if (!valid(nextSsid, nextPass)) return false;
  NetworkAddress next;
  // Separate record keeps existing AWN1 WiFi credentials compatible.
  if (!memcmp(extra,"PIP1",4)) {
    uint32_t hash=2166136261UL, storedExtra=0;
    for(size_t i=0;i<25;++i)hash=(hash^extra[i])*16777619UL;
    for(uint8_t i=0;i<4;++i)storedExtra|=uint32_t(extra[25+i])<<(8*i);
    if(hash!=storedExtra)return false;
    next.enabled=extra[4]!=0;
    uint32_t *fields[]={&next.ip,&next.gateway,&next.subnet,&next.dns1,&next.dns2};
    for(uint8_t f=0;f<5;++f)for(uint8_t i=0;i<4;++i)*fields[f]=(*fields[f]<<8)|extra[5+f*4+i];
    if(!validNetworkAddress(next))return false;
  }
  ssid = nextSsid; password = nextPass; address=next; return true;
}
bool saveNetworkConfig(const String &ssid, const String &password, const NetworkAddress &address) {
  if (!valid(ssid, password) || !validNetworkAddress(address)) return false;
  uint8_t data[RECORD_SIZE] = {};
  memcpy(data, "AWN1", 4); memcpy(data + 4, ssid.c_str(), ssid.length());
  memcpy(data + 37, password.c_str(), password.length());
  uint32_t hash = checksum(data);
  for (uint8_t i = 0; i < 4; ++i) data[PAYLOAD_SIZE + i] = hash >> (i * 8);
  EEPROM.begin(512);
  for (size_t i = 0; i < RECORD_SIZE; ++i) EEPROM.write(i, data[i]);
  uint8_t extra[29]={};memcpy(extra,"PIP1",4);extra[4]=address.enabled;
  const uint32_t fields[]={address.ip,address.gateway,address.subnet,address.dns1,address.dns2};
  for(uint8_t f=0;f<5;++f)for(uint8_t i=0;i<4;++i)extra[5+f*4+i]=fields[f]>>(24-8*i);
  uint32_t extraHash=2166136261UL;
  for(size_t i=0;i<25;++i)extraHash=(extraHash^extra[i])*16777619UL;
  for(uint8_t i=0;i<4;++i)extra[25+i]=extraHash>>(8*i);
  for(size_t i=0;i<sizeof(extra);++i)EEPROM.write(RECORD_SIZE+i,extra[i]);
  bool saved = EEPROM.commit(); EEPROM.end(); return saved;
}

bool parseNetworkIPv4(const char *text,uint32_t &address){
  if(!text||!*text)return false;
  uint32_t value=0;
  for(uint8_t part=0;part<4;++part){
    unsigned octet=0,digits=0;
    while(*text>='0'&&*text<='9'){
      octet=octet*10+(*text++-'0');if(++digits>3||octet>255)return false;
    }
    if(!digits)return false;
    value=(value<<8)|octet;
    if(part<3){if(*text++!='.')return false;}else if(*text)return false;
  }
  address=value;return true;
}
namespace {
bool unicast(uint32_t ip){return (ip>>24)>0&&(ip>>24)<224&&(ip>>24)!=127;}
}
bool validNetworkAddress(const NetworkAddress &a){
  if(!a.enabled)return true;
  const uint32_t hosts=~a.subnet;
  // Contiguous masks, /1 to /30; usable, distinct hosts in the same subnet.
  if(!a.subnet||hosts<3||(hosts&(hosts+1)))return false;
  if(!unicast(a.ip)||!unicast(a.gateway)||!unicast(a.dns1)||(a.dns2&&!unicast(a.dns2)))return false;
  if((a.ip&a.subnet)!=(a.gateway&a.subnet)||a.ip==a.gateway)return false;
  const uint32_t ipHost=a.ip&hosts,gwHost=a.gateway&hosts;
  return ipHost&&ipHost!=hosts&&gwHost&&gwHost!=hosts;
}
bool loadNetworkConfig(String &ssid,String &password){NetworkAddress a;return loadNetworkConfig(ssid,password,a);}
bool saveNetworkConfig(const String &ssid,const String &password){return saveNetworkConfig(ssid,password,NetworkAddress());}
