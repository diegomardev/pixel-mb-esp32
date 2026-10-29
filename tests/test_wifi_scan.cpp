#include "WifiScanResults.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Entry { String ssid; int32_t rssi; };
struct Scan {
  std::vector<Entry> entries;
  String SSID(int i) const { return entries.at(i).ssid; }
  int32_t RSSI(int i) const { return entries.at(i).rssi; }
};
int main() {
  uint8_t selected[WIFI_SCAN_LIMIT];Scan scan;
  assert(selectWifiNetworks(scan,0,selected)==0);
  scan.entries={{"",-1},{"Casa",-70},{"Invitados",-50},{"Casa",-30},{"Red <taller>",-60}};
  auto count=selectWifiNetworks(scan,scan.entries.size(),selected);
  assert(count==3 && selected[0]==3 && selected[1]==2 && selected[2]==4);
  scan.entries.clear();
  for(int i=0;i<100;i++)scan.entries.push_back({"red-"+std::to_string(i),-100+i});
  scan.entries.push_back({"red-95",0});scan.entries.push_back({"",1});
  count=selectWifiNetworks(scan,scan.entries.size(),selected);
  assert(count==16 && selected[0]==100);
  for(int i=0;i<count;i++) {
    assert(scan.RSSI(selected[i])>=-16);
    if(i)assert(scan.RSSI(selected[i-1])>=scan.RSSI(selected[i]));
    for(int j=i+1;j<count;j++)assert(scan.SSID(selected[i])!=scan.SSID(selected[j]));
  }
  std::cout<<"WiFi: filtra SSID vacio, deduplica por mejor senal, limita a 16 y ordena\n";
}
