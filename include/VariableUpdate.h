#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

inline void applyVariableUpdate(JsonObject target, JsonPairConst pair) {
  // String obliga a ArduinoJson a COPIAR el nombre dentro del documento.
  // pair.key().c_str() es const char*: enlaza memoria de la peticion que se libera.
  const String key = pair.key().c_str();
  if (pair.value().isNull()) target.remove(key);
  else target[key].set(pair.value());
}
