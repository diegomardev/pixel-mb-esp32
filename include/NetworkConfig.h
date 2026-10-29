#pragma once
#include <Arduino.h>
struct NetworkAddress {
  bool enabled = false;
  uint32_t ip = 0, gateway = 0, subnet = 0, dns1 = 0, dns2 = 0;
};
bool parseNetworkIPv4(const char *text, uint32_t &address);
bool validNetworkAddress(const NetworkAddress &address);
bool loadNetworkConfig(String &ssid, String &password, NetworkAddress &address);
bool saveNetworkConfig(const String &ssid, const String &password, const NetworkAddress &address);
bool loadNetworkConfig(String &ssid, String &password);
bool saveNetworkConfig(const String &ssid, const String &password);
