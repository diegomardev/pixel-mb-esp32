#pragma once
#include <Arduino.h>
constexpr uint8_t WIFI_SCAN_LIMIT = 16;

// Conservar los SSID unicos mas potentes y devolverlos ordenados por RSSI.
// Sin arrays del tamano total del escaneo: solo 16 indices.
template<class Scan>
uint8_t selectWifiNetworks(Scan &scan, int found, uint8_t (&selected)[WIFI_SCAN_LIMIT]) {
  uint8_t count = 0;
  for (int i = 0; i < found; ++i) {
    String ssid = scan.SSID(i);
    if (!ssid.length()) continue;
    bool duplicate = false;
    for (uint8_t j = 0; j < count; ++j) {
      if (scan.SSID(selected[j]) == ssid) {
        if (scan.RSSI(i) > scan.RSSI(selected[j])) selected[j] = i;
        duplicate = true; break;
      }
    }
    if (duplicate) continue;
    if (count < WIFI_SCAN_LIMIT) selected[count++] = i;
    else {
      uint8_t weakest = 0;
      for (uint8_t j = 1; j < count; ++j)
        if (scan.RSSI(selected[j]) < scan.RSSI(selected[weakest])) weakest = j;
      if (scan.RSSI(i) > scan.RSSI(selected[weakest])) selected[weakest] = i;
    }
  }
  for (uint8_t i = 0; i < count; ++i)
    for (uint8_t j = i + 1; j < count; ++j)
      if (scan.RSSI(selected[j]) > scan.RSSI(selected[i])) {
        uint8_t tmp = selected[i]; selected[i] = selected[j]; selected[j] = tmp;
      }
  return count;
}
