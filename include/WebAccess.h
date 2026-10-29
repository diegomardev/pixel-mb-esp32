#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

struct WebAccessConfig {
  bool enabled = false;
  String username = "admin";
  String password;
};
inline bool validWebUsername(const String &value) {
  if (!value.length() || value.length() > 32) return false;
  for (size_t i=0;i<value.length();++i) {
    const char c=value[i];
    if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c=='@')) return false;
  }
  return true;
}
inline bool validWebPassword(const String &value) {
  return value.length() > 0;
}
inline bool saveWebAccess(const WebAccessConfig &config) {
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(3)+config.username.length()+config.password.length()+2);
  doc["enabled"]=config.enabled;doc["username"]=config.username;doc["password"]=config.password;
  if(doc.overflowed())return false;
  File file=LittleFS.open("/web-access.tmp","w");if(!file)return false;
  const size_t expected=measureJson(doc),written=serializeJson(doc,file);file.flush();file.close();
  if(written!=expected || !LittleFS.rename("/web-access.tmp","/web-access.json")){LittleFS.remove("/web-access.tmp");return false;}
  return true;
}
inline void loadWebAccess(WebAccessConfig &config) {
  if(!LittleFS.exists("/web-access.json"))return;
  File file=LittleFS.open("/web-access.json","r");if(!file || file.size()>8192)return;
  DynamicJsonDocument doc(file.size()+JSON_OBJECT_SIZE(3)+64);if(deserializeJson(doc,file))return;
  if(!doc["enabled"].is<bool>() || !doc["username"].is<const char *>() || !doc["password"].is<const char *>())return;
  WebAccessConfig next;next.enabled=doc["enabled"];next.username=doc["username"].as<String>();next.password=doc["password"].as<String>();
  if(!validWebUsername(next.username) || (next.enabled&&!validWebPassword(next.password)))return;
  config=next;
}
