#include "VariableUpdate.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
void update(JsonDocument &store,const char *json) {
  DynamicJsonDocument request(2048),next(4096);
  assert(!deserializeJson(request,json)); next.set(store.as<JsonObjectConst>());
  for(JsonPairConst pair:request.as<JsonObjectConst>())applyVariableUpdate(next.as<JsonObject>(),pair);
  store.set(next);
} // La peticion y next ya no existen al devolver.
int main(){
  DynamicJsonDocument store(4096);store.to<JsonObject>();
  update(store,"{\"Co2\":650,\"Temp\":23.4,\"Hum\":48,\"TVOC\":32,\"mensaje\":\"Hola\\nMundo\"}");
  std::vector<std::string> churn(1000,std::string(128,'x'));
  assert(store["Co2"]==650&&std::abs(store["Temp"].as<double>()-23.4)<0.0001&&store["Hum"]==48&&store["TVOC"]==32);
  assert(store["mensaje"].as<std::string>()=="Hola\nMundo");
  std::string encoded;serializeJson(store,encoded);DynamicJsonDocument reread(4096);
  assert(!deserializeJson(reread,encoded));assert(reread["Co2"]==650);
  update(store,"{\"Co2\":720,\"Hum\":null,\"nuevo\":true}");
  assert(store["Co2"]==720&&!store.containsKey("Hum")&&store["nuevo"]==true);
  assert(store["mensaje"].as<std::string>()=="Hola\nMundo");
  std::cout<<"Variables: nombres y valores sobreviven a liberar peticiones y se serializan como JSON valido\n";
}
