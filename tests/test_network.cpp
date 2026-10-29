#include "NetworkConfig.h"
#include <EEPROM.h>
#include <cassert>
#include <iostream>
FakeEEPROM EEPROM;
int main() {
  String ssid="fallback",password="original";
  assert(!loadNetworkConfig(ssid,password)); assert(ssid=="fallback" && password=="original");
  assert(saveNetworkConfig("Mi WiFi","secreto123"));
  assert(loadNetworkConfig(ssid,password)); assert(ssid=="Mi WiFi" && password=="secreto123");
  EEPROM.bytes[9]^=1; assert(!loadNetworkConfig(ssid,password)); assert(ssid=="Mi WiFi");
  assert(!saveNetworkConfig("","12345678")); assert(!saveNetworkConfig(String(33,'x'),"12345678"));
  assert(!saveNetworkConfig("Red","short")); assert(!saveNetworkConfig("Red",String(64,'x')));
  assert(saveNetworkConfig("Red abierta","")); assert(loadNetworkConfig(ssid,password)); assert(password.empty());
  assert(saveNetworkConfig(String(32,'s'),String(63,'p'))); assert(loadNetworkConfig(ssid,password));
  assert(ssid.length()==32 && password.length()==63);
  // Old AWN1-only firmware records remain readable and use DHCP.
  for(size_t i=106;i<135;++i)EEPROM.bytes[i]=255;
  NetworkAddress loaded;loaded.enabled=true;
  assert(loadNetworkConfig(ssid,password,loaded));assert(!loaded.enabled);
  NetworkAddress a;a.enabled=true;
  assert(parseNetworkIPv4("10.205.17.152",a.ip));
  assert(parseNetworkIPv4("10.205.17.1",a.gateway));
  assert(parseNetworkIPv4("255.255.255.0",a.subnet));
  assert(parseNetworkIPv4("1.1.1.1",a.dns1));
  assert(validNetworkAddress(a));
  assert(saveNetworkConfig("Mi WiFi","secreto123",a));
  assert(loadNetworkConfig(ssid,password,loaded));
  assert(loaded.enabled&&loaded.ip==a.ip&&loaded.gateway==a.gateway&&loaded.subnet==a.subnet&&loaded.dns1==a.dns1&&loaded.dns2==0);
  uint32_t out=42;
  for(const char *bad:{"","1.2.3","1.2.3.4.5","256.0.0.1","1.-2.3.4","1.2.3.4 "," 1.2.3.4","0000.1.2.3"}){
    assert(!parseNetworkIPv4(bad,out));assert(out==42);
  }
  auto bad=a;bad.subnet=0xFF00FF00;assert(!validNetworkAddress(bad));
  bad=a;bad.ip=0x0ACD1100;assert(!validNetworkAddress(bad)); // network
  bad=a;bad.ip=0x0ACD11FF;assert(!validNetworkAddress(bad)); // broadcast
  bad=a;bad.gateway=0x0ACD1201;assert(!validNetworkAddress(bad)); // other subnet
  bad=a;bad.gateway=a.ip;assert(!validNetworkAddress(bad));
  bad=a;bad.dns1=0;assert(!validNetworkAddress(bad));
  bad=a;bad.dns2=0xE0000001;assert(!validNetworkAddress(bad));
  assert(!saveNetworkConfig("Red","12345678",bad));
  EEPROM.bytes[111]^=1;assert(!loadNetworkConfig(ssid,password,loaded));
  assert(saveNetworkConfig("Mi WiFi","secreto123",NetworkAddress()));
  assert(loadNetworkConfig(ssid,password,loaded));assert(!loaded.enabled); // DHCP again
  EEPROM.commitOK=false; assert(!saveNetworkConfig("Red","12345678"));
  std::cout<<"NetworkConfig.cpp: persistencia, checksum, limites y error de escritura correctos\n";
}
