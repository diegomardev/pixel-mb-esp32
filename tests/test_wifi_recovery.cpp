#include "WifiRecovery.h"
#include <assert.h>
#include <iostream>
int main() {
  WifiRecovery r;r.begin(100);
  assert(!r.tick(15099,false,true,false,false).startAccessPoint);
  assert(r.tick(15100,false,true,false,false).startAccessPoint);
  assert(!r.tick(15101,false,true,false,false).startAccessPoint);
  assert(r.tick(20100,false,true,false,false).startAccessPoint); // reintento AP si falló
  assert(!r.tick(30099,false,true,true,false).reconnect);
  assert(r.tick(30100,false,true,true,false).reconnect);
  assert(!r.tick(30101,false,true,true,false).reconnect);
  assert(r.tick(60100,false,true,true,false).reconnect);
  assert(!r.tick(90100,false,true,true,true).reconnect); // no interferir con escaneo
  assert(r.tick(90101,false,true,true,false).reconnect);
  assert(!r.tick(120101,true,true,true,false).reconnect);
  assert(!r.tick(180101,true,true,true,false).reconnect);
  assert(!r.tick(200000,false,true,false,false).startAccessPoint); // pérdida tras conectar
  assert(r.tick(215000,false,true,false,false).startAccessPoint);
  assert(r.tick(230000,false,true,true,false).reconnect);
  r.begin(0);assert(r.tick(5000,false,false,false,false).startAccessPoint);
  assert(!r.tick(60000,false,false,true,false).reconnect); // sin credenciales, solo portal
  r.begin(0xfffff000U);
  assert(r.tick(uint32_t(0xfffff000U+15000U),false,true,false,false).startAccessPoint);
  assert(r.tick(uint32_t(0xfffff000U+30000U),false,true,true,false).reconnect);
  std::cout<<"WiFi: arranque sin red, pérdida posterior, AP, reintentos, escaneo y millis correctos\n";
}
