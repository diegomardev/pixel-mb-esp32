#include <Arduino.h>
#include <memory>
#include <ArduinoOTA.h>
#include "LocalOtaConfig.h"
#include <WiFi.h>
#include <Update.h>
#include "FlashStorage.h"
#include "Buttons.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "NetworkConfig.h"
#include "WifiRecovery.h"
#include "ConnectionIpNotice.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoMatrix.h>
#include <Fonts/TomThumb.h>
#include <LittleFS.h>
#include <time.h>
#include <sys/time.h>
#include "Config.h"
#include "Animation.h"
#include "PixelMatrix.h"
#include "ClockText.h"
#include "BootLogo.h"
#include "BmpSensor.h"
#include "Rotation.h"
#include "DefaultStore.h"
#include "VariableUpdate.h"
#include "WifiScanResults.h"
#include "WebUI.h"
#include "UpdateUI.h"
#include "WebAccess.h"

WebServer server(HTTP_PORT);
WebServer portal(80);
DNSServer dns;
WebAccessConfig webAccess;
String wifiSsid, wifiPassword;
String deviceId, deviceHostname;
ButtonInput buttons[3];
NetworkAddress networkAddress;
IPAddress networkIP(uint32_t ip){return IPAddress(ip>>24,(ip>>16)&255,(ip>>8)&255,ip&255);}
String networkIPText(uint32_t ip){return ip ? networkIP(ip).toString() : String();}
bool applyNetworkAddress(){
  if(networkAddress.enabled)return WiFi.config(networkIP(networkAddress.ip),networkIP(networkAddress.gateway),networkIP(networkAddress.subnet),networkIP(networkAddress.dns1),networkIP(networkAddress.dns2));
  return WiFi.config(IPAddress(0U),IPAddress(0U),IPAddress(0U));
}
volatile uint16_t wifiDisconnectReason=0;
bool wifiChangePending = false;
uint32_t wifiChangeQueuedAt = 0;
bool wifiScanRequested = false;
uint32_t wifiScanStartedAt = 0;
String wifiScanResult;
PixelMatrix matrix;
bool matrixReady = false;
AnimationPlayer player;
BmpSensor bmpSensor;
RotationConfig rotationConfig;
RotationScheduler rotationScheduler;
DynamicJsonDocument variables(4096);
bool fsReady = false, apStarted = false, mdnsStarted = false;
bool otaStarted = false, otaUpdating = false, otaFsClosed = false;
constexpr uint16_t OTA_PORT = 8266;
uint8_t brightness = DEFAULT_BRIGHTNESS;
bool showSeconds = false;
uint8_t lastBrightness = DEFAULT_BRIGHTNESS;
WifiRecovery wifiRecovery;
uint32_t wifiReconnectAttempts = 0;
bool wifiWasConnected = false;
uint32_t lastConnectedIp = 0;
ConnectionIpNotice connectionIpNotice;
uint32_t renderAt = 0, sceneStart = 0;
String renderedText;
int16_t textX = 0, textWidth = 0;
uint32_t scrollAt = 0;
uint16_t scrollLoops = 0;
bool textInitialized = false;
uint8_t rotationRenderIndex = 255;
uint32_t rotationRenderGeneration = 0;
String randomAnimation, previousRandomAnimation;
bool bootLogoActive = false;
uint32_t bootLogoStartedAt = 0;

struct Scene {
  String mode = "clock", text, animation;
  uint32_t color = 0x00FF00;
  uint32_t durationMs = 0; // 0 = indefinido (escena base)
  uint16_t scrollMs = 70;
  uint8_t repeat = 1;
};
Scene baseScene, currentScene;
Scene notificationQueue[NOTIFY_QUEUE_SIZE];
uint8_t queueCount = 0;
bool notifying = false;

void jsonReply(int status, const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  server.send(status, "application/json; charset=utf-8", body);
}
void errorReply(int status, const String &message) {
  StaticJsonDocument<256> doc;
  doc["error"] = message;
  jsonReply(status, doc);
}
void okReply() { server.send(200, "application/json", "{\"ok\":true}"); }
bool webAuthenticated() {
  return !webAccess.enabled || (server.header("Authorization").startsWith("Digest ") && server.authenticate(webAccess.username.c_str(),webAccess.password.c_str()));
}
bool webAuthorized() {
  if(webAuthenticated())return true;
  server.requestAuthentication(DIGEST_AUTH,"Pixel-MB Web");return false;
}
bool authorized() {
  if(!webAuthorized())return false;
  if (!strlen(API_KEY) || server.header("X-API-Key") == API_KEY) return true;
  errorReply(401, "Se requiere la cabecera X-API-Key");
  return false;
}
bool requireFS() {
  if (fsReady) return true;
  errorReply(503, "LittleFS no montado: realizar la carga inicial con pio run -t uploadfs");
  return false;
}
bool readJson(JsonDocument &doc, size_t maxBytes = MAX_JSON_BYTES) {
  if (!server.hasArg("plain") || server.arg("plain").length() > maxBytes) {
    errorReply(413, "JSON ausente o mayor de " + String(maxBytes) + " bytes"); return false;
  }
  auto err = deserializeJson(doc, server.arg("plain"), DeserializationOption::NestingLimit(4));
  if (err || !doc.is<JsonObject>()) { errorReply(400, "Se espera un objeto JSON válido"); return false; }
  return true;
}
bool unsignedIn(JsonVariantConst value, uint32_t min, uint32_t max) {
  return value.is<uint32_t>() && value.as<uint32_t>() >= min && value.as<uint32_t>() <= max;
}
bool parseColor(JsonVariantConst value, uint32_t &out) {
  if (value.is<JsonArrayConst>()) {
    JsonArrayConst a = value.as<JsonArrayConst>();
    if (a.size() != 3) return false;
    for (JsonVariantConst c : a) if (!unsignedIn(c, 0, 255)) return false;
    out = (a[0].as<uint32_t>() << 16) | (a[1].as<uint32_t>() << 8) | a[2].as<uint32_t>();
    return true;
  }
  if (value.is<const char *>()) {
    String s = value.as<String>();
    if (s.startsWith("#")) s.remove(0, 1);
    if (s.length() != 6) return false;
    for (char c : s) if (!isxdigit(static_cast<unsigned char>(c))) return false;
    out = strtoul(s.c_str(), nullptr, 16); return true;
  }
  return false;
}
String primitiveString(JsonVariantConst value) {
  if (value.is<const char *>()) return value.as<String>();
  String result; serializeJson(value, result); return result;
}
// Los marcadores desconocidos quedan visibles; solo una sustitución, sin recursión.
String expandText(const String &input) {
  String out; out.reserve(MAX_TEXT_BYTES + 1);
  for (size_t i = 0; i < input.length() && out.length() < MAX_TEXT_BYTES;) {
    if (input[i] == '{') {
      int end = input.indexOf('}', i + 1);
      if (end > int(i)) {
        String key = input.substring(i + 1, end);
        if (key == "bmpTemp" || key == "bmpPressure" || key == "bmpHumidity") {
          String value = key == "bmpHumidity" ? (bmpSensor.humidityValid()?String(bmpSensor.humidity(),1):String("--")) : (bmpSensor.valid() ? String(key == "bmpTemp" ? bmpSensor.temperatureC() : bmpSensor.pressureHpa(), 1) : String("--"));
          out += value.substring(0, MAX_TEXT_BYTES - out.length());
          i = end + 1; continue;
        }
        if (variables.containsKey(key)) {
          String value = primitiveString(variables[key]);
          out += value.substring(0, MAX_TEXT_BYTES - out.length());
          i = end + 1; continue;
        }
      }
    }
    out += input[i++];
  }
  return out;
}
// Fuente TomThumb: ASCII. Convertimos los acentos españoles de UTF-8.
String displayAscii(String text) {
  const char *from[] = {"á","é","í","ó","ú","ü","ñ","Á","É","Í","Ó","Ú","Ü","Ñ","º","°"};
  const char *to[] =   {"a","e","i","o","u","u","n","A","E","I","O","U","U","N","o","o"};
  for (size_t i = 0; i < sizeof(from) / sizeof(from[0]); ++i) text.replace(from[i], to[i]);
  for (size_t i = 0; i < text.length(); ++i)
    if (static_cast<uint8_t>(text[i]) < 32 || static_cast<uint8_t>(text[i]) > 126) text.setCharAt(i, '?');
  return text;
}
void sceneJson(JsonObject obj, const Scene &s) {
  obj["mode"] = s.mode; obj["text"] = s.text; obj["animation"] = s.animation;
  char c[8]; snprintf(c, sizeof(c), "#%06lX", static_cast<unsigned long>(s.color));
  obj["color"] = c; obj["duration"] = s.durationMs / 1000;
  obj["scrollMs"] = s.scrollMs; obj["repeat"] = s.repeat;
}
bool parseScene(JsonObjectConst obj, Scene &scene, String &err, bool notification) {
  scene = Scene(); scene.mode = notification ? "custom" : String(obj["mode"] | "custom");
  if (scene.mode != "custom" && scene.mode != "clock" && scene.mode != "off" && scene.mode != "rotation") {
    err = "mode debe ser custom, clock, rotation u off"; return false;
  }
  JsonVariantConst data = obj.containsKey("data") ? obj["data"] : obj["text"];
  if (!data.isNull()) {
    if (data.is<JsonArrayConst>() || data.is<JsonObjectConst>()) { err = "data/text debe ser un valor simple"; return false; }
    scene.text = primitiveString(data);
    if (scene.text.length() > MAX_TEXT_BYTES) { err = "Texto mayor de 160 bytes"; return false; }
  }
  JsonVariantConst icon = obj.containsKey("icon") ? obj["icon"] : obj["animation"];
  if (!icon.isNull()) {
    if (!icon.is<const char *>() && !icon.is<uint32_t>()) { err = "icon/animation debe ser nombre o ID positivo"; return false; }
    scene.animation = primitiveString(icon);
    if (scene.animation.length() && !validName(scene.animation)) { err = "Nombre de animación inválido"; return false; }
  }
  if (obj.containsKey("color") && !parseColor(obj["color"], scene.color)) { err = "Color inválido: [r,g,b] o #RRGGBB"; return false; }
  if (obj.containsKey("duration")) {
    if (!unsignedIn(obj["duration"], 0, 3600)) { err = "duration fuera de rango (segundos)"; return false; }
    scene.durationMs = obj["duration"].as<uint32_t>() * 1000UL;
  }
  if (obj.containsKey("scrollMs")) {
    if (!unsignedIn(obj["scrollMs"], 20, 1000)) { err = "scrollMs debe estar entre 20 y 1000"; return false; }
    scene.scrollMs = obj["scrollMs"];
  }
  if (obj.containsKey("repeat")) {
    if (!unsignedIn(obj["repeat"], 1, 20)) { err = "repeat debe estar entre 1 y 20"; return false; }
    scene.repeat = obj["repeat"];
  }
  if (scene.mode == "custom" && !scene.text.length() && !scene.animation.length()) {
    err = "Falta data/text o icon/animation"; return false;
  }
  if (scene.animation.length()) {
    if (!fsReady) { err = "LittleFS no está montado"; return false; }
    File file = LittleFS.open(animationPath(scene.animation), "r"); AnimationInfo info;
    if (!inspectAnimation(file, info)) { err = "Icono/animación local no encontrado o inválido: " + scene.animation; return false; }
    if (scene.text.length() && info.width > 8) { err = "Los iconos con texto deben tener ancho <= 8"; return false; }
  }
  // Sin duration explícita o con duration=0: repeat controla pasadas de texto, con un mínimo
  // de 5 s para mensajes cortos y un límite de 1 hora por notificación.
  return true;
}
void activate(const Scene &scene) {
  currentScene = scene; sceneStart = millis(); player.close();
  if (scene.animation.length()) player.open(scene.animation);
  renderedText = "\x01"; textX = 0; textWidth = 0; scrollAt = 0; scrollLoops = 0;
  textInitialized = false; rotationRenderIndex = 255;
  rotationScheduler.reset(millis());
}
void restoreBase() { notifying = false; activate(baseScene); }
void nextNotification() {
  if (!queueCount) { restoreBase(); return; }
  Scene next = notificationQueue[0];
  for (uint8_t i = 1; i < queueCount; ++i) notificationQueue[i - 1] = notificationQueue[i];
  notificationQueue[--queueCount] = Scene();
  notifying = true; activate(next);
}
void clearQueue() {
  for (auto &s : notificationQueue) s = Scene();
  queueCount = 0;
}
void rotationConfigJson(JsonObject obj, const RotationConfig &config) {
  obj["enabled"] = config.enabled;
  JsonArray pool=obj.createNestedArray("randomAnimations");
  for(uint8_t i=0;i<config.randomCount;++i)pool.add(String(config.randomAnimations[i]));
  JsonArray items = obj.createNestedArray("items");
  for (uint8_t i=0;i<config.count;++i) {
    const auto &item=config.items[i]; JsonObject entry=items.createNestedObject();
    entry["type"]=rotationPageName(item.type); entry["seconds"]=item.seconds;
    if(item.type==RotationPage::Clock){entry["showSeconds"]=item.showSeconds;entry["showWeekdays"]=item.showWeekdays;}
    if(item.type==RotationPage::Animation){
      entry["animation"]=String(item.animation);
    }
    if(item.text[0])entry["text"]=String(item.text);
    char color[8];snprintf(color,sizeof(color),"#%06lx",static_cast<unsigned long>(item.color));
    entry["color"]=String(color);
  }
}
void rotationJson(JsonObject obj) {
  rotationConfigJson(obj,rotationConfig);
  const bool active=!notifying && currentScene.mode=="rotation";
  obj["baseSceneFixed"]=baseScene.mode=="custom" && baseScene.durationMs==0;
  obj["active"]=active; obj["index"]=rotationScheduler.index();
  obj["page"]=rotationPageName(rotationScheduler.page(rotationConfig));
  obj["animation"]=rotationScheduler.page(rotationConfig)==RotationPage::Random?randomAnimation:String(rotationConfig.items[rotationScheduler.index()].animation);
  obj["remainingSeconds"]=active?rotationScheduler.remainingSeconds(millis(),rotationConfig):0;
}
bool parseRotation(JsonObjectConst obj,RotationConfig &next,String &err,bool checkFiles) {
  if(obj.containsKey("enabled")){
    if(!obj["enabled"].is<bool>()){err="enabled debe ser booleano";return false;}
    next.enabled=obj["enabled"];
  }
  if(obj.containsKey("randomAnimations")){
    if(!obj["randomAnimations"].is<JsonArrayConst>() || obj["randomAnimations"].size()>MAX_RANDOM_ANIMATIONS){err="randomAnimations: lista de hasta 48 nombres";return false;}
    next.randomCount=0;
    for(JsonVariantConst v:obj["randomAnimations"].as<JsonArrayConst>()){
      if(!v.is<const char *>() || !validName(v.as<String>())){err="Nombre aleatorio invalido";return false;}
      String name=v.as<String>();
      for(uint8_t i=0;i<next.randomCount;++i)if(name==next.randomAnimations[i]){err="Animacion aleatoria duplicada";return false;}
      if(checkFiles){File f=LittleFS.open(animationPath(name),"r");AnimationInfo info;if(!inspectAnimation(f,info)){err="Animacion no encontrada: "+name;return false;}}
      name.toCharArray(next.randomAnimations[next.randomCount++],21);
    }
  }
  if(obj.containsKey("items")){
    if(!obj["items"].is<JsonArrayConst>()){err="items debe ser una lista";return false;}
    JsonArrayConst items=obj["items"];
    if(!items.size() || items.size()>MAX_ROTATION_ITEMS){err="La rotacion admite de 1 a 16 elementos";return false;}
    next.count=0;
    for(JsonVariantConst value:items){
      if(!value.is<JsonObjectConst>()){err="Elemento de rotacion invalido";return false;}
      const char *type=value["type"]|"";
      RotationItem item;
      if(!strcmp(type,"clock"))item.type=RotationPage::Clock;
      else if(!strcmp(type,"temperature"))item.type=RotationPage::Temperature;
      else if(!strcmp(type,"pressure"))item.type=RotationPage::Pressure;
      else if(!strcmp(type,"animation"))item.type=RotationPage::Animation;
      else if(!strcmp(type,"random"))item.type=RotationPage::Random;
      else if(!strcmp(type,"text"))item.type=RotationPage::Text;
      else {err="Tipo de pantalla desconocido";return false;}
      if(!unsignedIn(value["seconds"],1,3600)){err="Duracion: 1..3600 segundos";return false;}
      item.seconds=value["seconds"];
      if(item.type==RotationPage::Clock)item.showSeconds=showSeconds; // Migrar filas antiguas conservando su aspecto.
      const char *clockKeys[]={"showSeconds","showWeekdays"};
      for(const char *key:clockKeys)if(value.containsKey(key)){
        if(item.type!=RotationPage::Clock || !value[key].is<bool>()){err="showSeconds/showWeekdays requieren una fila clock y valores booleanos";return false;}
      }
      if(value.containsKey("showSeconds"))item.showSeconds=value["showSeconds"];
      if(value.containsKey("showWeekdays"))item.showWeekdays=value["showWeekdays"];

      item.color=defaultRotationColor(item.type);
      if(value.containsKey("color")) {
        if(!parseColor(value["color"],item.color)){
          err="Color invalido: usa #RRGGBB o [r,g,b]";return false;
        }
      }
      if(value.containsKey("text")) {
        if(!value["text"].is<const char *>()){err="text debe ser una cadena";return false;}
        JsonString text=value["text"].as<JsonString>();
        if(!setRotationText(item,text.c_str(),text.size())){err="Texto: hasta 64 bytes UTF-8, sin saltos ni controles";return false;}
        if(item.text[0] && item.type!=RotationPage::Animation && item.type!=RotationPage::Text){err="Texto solo disponible en texto o icono";return false;}
      }
      if(item.type==RotationPage::Text && !item.text[0]){err="La pantalla de texto requiere text";return false;}
      if(item.type==RotationPage::Animation){
        String name=value["animation"]|"";
        if(!validName(name)){err="Nombre de animacion invalido";return false;}
        if(checkFiles){File f=LittleFS.open(animationPath(name),"r");AnimationInfo info;
          if(!inspectAnimation(f,info)){err="Animacion no encontrada o invalida: "+name;return false;}
          if(item.text[0] && (info.width>8 || info.height>8)){err="El texto requiere un icono de hasta 8x8";return false;}
        }
        name.toCharArray(item.animation,sizeof(item.animation));
      }
      next.items[next.count++]=item;
    }
  } else {
    // Migracion y compatibilidad con configuraciones 1.0.7.
    const char *keys[]={"clockSeconds","temperatureSeconds","pressureSeconds"};
    for(uint8_t type=0;type<3;++type)if(obj.containsKey(keys[type])){
      if(!unsignedIn(obj[keys[type]],1,3600)){err="Duracion: 1..3600 segundos";return false;}
      for(uint8_t i=0;i<next.count;++i)if(next.items[i].type==static_cast<RotationPage>(type))next.items[i].seconds=obj[keys[type]];
    }
  }
  return true;
}
bool saveRotation(const RotationConfig &config) {
  File tmp=LittleFS.open("/rotation.tmp","w");if(!tmp)return false;
  // Escribir por elementos evita tener dos documentos grandes en RAM al guardar.
  size_t expected=0,written=0;
  auto part=[&](const String &text){expected+=text.length();written+=tmp.print(text);};
  part(config.enabled?"{\"enabled\":true,\"randomAnimations\":[":"{\"enabled\":false,\"randomAnimations\":[");
  for(uint8_t i=0;i<config.randomCount;++i){if(i)part(",");part(String("\"")+config.randomAnimations[i]+"\"");}
  part("],\"items\":[");
  for(uint8_t i=0;i<config.count;++i){
    if(i)part(",");
    const auto &item=config.items[i];StaticJsonDocument<512> entry;
    entry["type"]=rotationPageName(item.type);entry["seconds"]=item.seconds;
    if(item.type==RotationPage::Clock){entry["showSeconds"]=item.showSeconds;entry["showWeekdays"]=item.showWeekdays;}
    if(item.type==RotationPage::Animation)entry["animation"]=String(item.animation);
    if(item.text[0])entry["text"]=String(item.text);
    char color[8];snprintf(color,sizeof(color),"#%06lx",static_cast<unsigned long>(item.color));entry["color"]=String(color);
    if(entry.overflowed()){tmp.close();LittleFS.remove("/rotation.tmp");return false;}
    expected+=measureJson(entry);written+=serializeJson(entry,tmp);
  }
  part("]}");tmp.flush();tmp.close();
  if(written!=expected || !LittleFS.rename("/rotation.tmp","/rotation.json")){LittleFS.remove("/rotation.tmp");return false;}
  return true;
}
void loadRotation() {
  File file=LittleFS.open("/rotation.json","r");if(!file || file.size()>8192)return;
  DynamicJsonDocument doc(8192);if(deserializeJson(doc,file) || !doc.is<JsonObject>())return;
  std::unique_ptr<RotationConfig> next(new(std::nothrow) RotationConfig);String err;
  if(next && parseRotation(doc.as<JsonObjectConst>(),*next,err,false))rotationConfig=*next;
}
void handleRotation() {
  if(!authorized())return;
  if(server.method()==HTTP_POST){
    if(!requireFS())return;
    DynamicJsonDocument request(8192);if(!readJson(request,8192))return;
    std::unique_ptr<RotationConfig> next(new(std::nothrow) RotationConfig(rotationConfig));String err;
    if(!next){errorReply(503,"Memoria insuficiente");return;}
    if(!parseRotation(request.as<JsonObjectConst>(),*next,err,true)){errorReply(400,err);return;}
    if(!saveRotation(*next)){errorReply(507,"No se pudo guardar la rotacion");return;}
    rotationConfig=*next;
    const bool randomOnly=request.as<JsonObjectConst>().size()==1 && request.containsKey("randomAnimations");
    // Updating the random pool must not unpin a scene or restart the active screen.
    if(!randomOnly){
      if(next->enabled){baseScene=Scene();baseScene.mode="rotation";}
      else if(baseScene.mode=="rotation")baseScene=Scene();
      if(!notifying)activate(baseScene);
    }
  }
  DynamicJsonDocument response(6144);rotationJson(response.to<JsonObject>());jsonReply(200,response);
}
bool saveState() {
  DynamicJsonDocument state(6144);
  state["version"] = 1; state["brightness"] = brightness;
  state["lastBrightness"] = lastBrightness;
  state["showSeconds"] = showSeconds;
  state["variables"].set(variables.as<JsonObjectConst>());
  sceneJson(state.createNestedObject("scene"), baseScene);
  if (state.overflowed()) return false;
  File tmp = LittleFS.open("/state.tmp", "w");
  if (!tmp) return false;
  size_t expected = measureJson(state), written = serializeJson(state, tmp);
  tmp.flush(); tmp.close();
  if (written != expected || !LittleFS.rename("/state.tmp", "/state.json")) {
    LittleFS.remove("/state.tmp"); return false;
  }
  return true;
}
bool validVariables(JsonObjectConst obj) {
  if (obj.size() > 16) return false;
  for (JsonPairConst pair : obj) {
    if (!validName(String(pair.key().c_str()))) return false;
    auto value = pair.value();
    if (value.is<JsonObjectConst>() || value.is<JsonArrayConst>() || value.isNull() ||
        primitiveString(value).length() > 64) return false;
  }
  return true;
}
void loadState() {
  File file = LittleFS.open("/state.json", "r");
  if (!file || file.size() > 4096) return;
  DynamicJsonDocument state(6144);
  if (deserializeJson(state, file) || state["version"] != 1) return;
  if (unsignedIn(state["brightness"], 0, 255)) brightness = state["brightness"];
  if (state["showSeconds"].is<bool>()) showSeconds = state["showSeconds"];
  if (unsignedIn(state["lastBrightness"], 1, 255)) lastBrightness = state["lastBrightness"];
  if (state["variables"].is<JsonObject>() && validVariables(state["variables"]))
    variables.set(state["variables"]);
  Scene scene; String err;
  if (state["scene"].is<JsonObject>() && parseScene(state["scene"], scene, err, false)) baseScene = scene;
}
void handleVariables() {
  if (!authorized()) return;
  if (server.method() == HTTP_GET) {
    DynamicJsonDocument doc(4608); doc.set(variables.as<JsonObjectConst>());
    if (bmpSensor.valid()) { doc["bmpTemp"] = bmpSensor.temperatureC(); doc["bmpPressure"] = bmpSensor.pressureHpa(); }
    else { doc["bmpTemp"] = nullptr; doc["bmpPressure"] = nullptr; }
    if(bmpSensor.humidityValid())doc["bmpHumidity"]=bmpSensor.humidity();else doc["bmpHumidity"]=nullptr;
    jsonReply(200, doc); return;
  }
  DynamicJsonDocument request(4096);
  if (!readJson(request)) return;
  DynamicJsonDocument next(4096);
  if(server.arg("replace")=="true")next.to<JsonObject>();else next.set(variables.as<JsonObjectConst>());
  for (JsonPairConst pair : request.as<JsonObjectConst>()) {
    String key = pair.key().c_str();
    if (key == "bmpTemp" || key == "bmpPressure" || key == "bmpHumidity") {
      errorReply(400, "bmpTemp, bmpPressure y bmpHumidity son variables de solo lectura del sensor"); return;
    }
    applyVariableUpdate(next.as<JsonObject>(), pair);
  }
  if (next.overflowed() || !validVariables(next.as<JsonObjectConst>())) {
    errorReply(400, "Máximo 16 variables, nombres de 1..20 caracteres y valores simples de <=64 bytes"); return;
  }
  variables.set(next); okReply();
}
void handleDisplay(bool notification) {
  if (!authorized()) return;
  DynamicJsonDocument request(4096);
  if (!readJson(request)) return;
  Scene scene; String err;
  if (!parseScene(request.as<JsonObjectConst>(), scene, err, notification)) { errorReply(400, err); return; }
  if (request.containsKey("force") && !request["force"].is<bool>()) { errorReply(400, "force debe ser booleano"); return; }
  if (!notification) {
    clearQueue(); notifying = false; baseScene = scene; activate(scene);
  } else if ((request["force"] | false) || !notifying) {
    // force interrumpe el mensaje actual; conserva los que esperan en cola.
    notifying = true; activate(scene);
  } else {
    if (queueCount == NOTIFY_QUEUE_SIZE) { errorReply(429, "Cola de notificaciones llena"); return; }
    notificationQueue[queueCount++] = scene;
  }
  StaticJsonDocument<384> response; response["ok"] = true; response["queued"] = queueCount;
  JsonArray warnings = response.createNestedArray("warnings");
  if (request.containsKey("soundfile")) warnings.add("soundfile sin efecto: no hay soporte de audio");
  if (request.containsKey("moveIcon")) warnings.add("moveIcon sin efecto: icono fijo con fotogramas animados");
  jsonReply(200, response);
}
void handleSettings() {
  if (!authorized()) return;
  if (server.method() == HTTP_GET) {
    StaticJsonDocument<128> doc; doc["Brightness"] = brightness; doc["on"] = brightness != 0; doc["showSeconds"] = showSeconds; jsonReply(200, doc); return;
  }
  DynamicJsonDocument request(4096);
  if (!readJson(request)) return;
  bool hasBri = request.containsKey("Brightness") || request.containsKey("brightness");
  bool hasSeconds = request.containsKey("showSeconds");
  if (!hasBri && !request.containsKey("on") && !hasSeconds) {
    errorReply(400, "Falta Brightness/brightness, on o showSeconds"); return;
  }
  if (hasSeconds && !request["showSeconds"].is<bool>()) {
    errorReply(400, "showSeconds debe ser booleano"); return;
  }
  uint8_t next = brightness;
  if (hasBri) {
    JsonVariantConst value = request.containsKey("Brightness") ? request["Brightness"] : request["brightness"];
    if (!unsignedIn(value, 0, 255)) { errorReply(400, "Brightness debe estar entre 0 y 255"); return; }
    next = value.as<uint8_t>();
  }
  if (request.containsKey("on")) {
    if (!request["on"].is<bool>()) { errorReply(400, "on debe ser booleano"); return; }
    if (!request["on"].as<bool>()) { if (next) lastBrightness = next; next = 0; }
    else if (!next) next = lastBrightness;
  }
  brightness = next;
  if (hasSeconds) showSeconds = request["showSeconds"].as<bool>();
  if (next) lastBrightness = next;
  matrix.setBrightness(brightness); okReply();
}
uint8_t animationCount() {
  uint8_t count = 0; AnimationDirectory dir("/animations");
  while (dir.next()) if (dir.fileName().endsWith(".awx")) ++count;
  return count;
}
// La carga multipart se escribe por bloques; nunca se carga toda la animación en RAM.
File uploadFile;
String uploadName, uploadError;
uint32_t uploadBytes = 0;
bool uploadAllowed = false, uploadSeen = false;
void handleUploadChunk() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (uploadSeen) {
      uploadFile.close(); LittleFS.remove("/animation.tmp");
      uploadAllowed = false; uploadError = "Enviar un único archivo por petición"; return;
    }
    uploadSeen = true; uploadBytes = 0; uploadError = "";
    uploadName = server.arg("name");
    uploadAllowed = webAuthenticated() && (!strlen(API_KEY) || server.header("X-API-Key") == API_KEY);
    if (!uploadAllowed) { uploadError = "No autorizado"; return; }
    if (!fsReady) { uploadError = "LittleFS no está montado"; return; }
    if (!validName(uploadName)) { uploadError = "name debe tener 1..20 letras, números, - o _"; return; }
    if (!LittleFS.exists(animationPath(uploadName)) && animationCount() >= MAX_ANIMATIONS) {
      uploadError = "Máximo 48 animaciones"; return;
    }
    uploadFile = LittleFS.open("/animation.tmp", "w");
    if (!uploadFile) uploadError = "No se puede abrir el archivo temporal";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!uploadAllowed || uploadError.length()) return;
    uploadBytes += upload.currentSize;
    if (uploadBytes > MAX_ANIMATION_BYTES) { uploadError = "Animación demasiado grande"; uploadFile.close(); return; }
    if (uploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      uploadError = "Sin espacio o error al escribir"; uploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    uploadFile.flush(); uploadFile.close();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    uploadFile.close(); if (uploadAllowed) LittleFS.remove("/animation.tmp");
    uploadError = "Carga cancelada"; uploadSeen = false;
  }
}
void finishUpload() {
  bool seen = uploadSeen; uploadSeen = false;
  uploadFile.close();
  if (!authorized()) return;
  if (!requireFS()) return;
  if (!seen || uploadError.length()) {
    LittleFS.remove("/animation.tmp"); errorReply(400, seen ? uploadError : "Falta el archivo multipart"); return;
  }
  File file = LittleFS.open("/animation.tmp", "r"); AnimationInfo info;
  bool valid = inspectAnimation(file, info); file.close();
  if (!valid) { LittleFS.remove("/animation.tmp"); errorReply(400, "Archivo AWX inválido o truncado"); return; }
  bool usedWithText = (currentScene.animation == uploadName && currentScene.text.length()) ||
    (baseScene.animation == uploadName && baseScene.text.length());
  for (uint8_t i = 0; i < queueCount; ++i)
    usedWithText = usedWithText || (notificationQueue[i].animation == uploadName && notificationQueue[i].text.length());
  if (usedWithText && info.width > 8) {
    LittleFS.remove("/animation.tmp"); errorReply(409, "No se puede ampliar un icono que está en uso con texto"); return;
  }
  if (!ensureAnimationDirectory()) {
    LittleFS.remove("/animation.tmp"); errorReply(507, "No se pudo crear la carpeta de animaciones"); return;
  }
  bool playing = player.name() == uploadName;
  if (playing) player.close();
  bool renamed = LittleFS.rename("/animation.tmp", animationPath(uploadName));
  if (playing) { player.open(uploadName); renderedText = "\x01"; textInitialized = false; }
  if (!renamed) { LittleFS.remove("/animation.tmp"); errorReply(507, "No se pudo guardar la animación"); return; }
  okReply();
}
void handleAnimations() {
  if (!authorized() || !requireFS()) return;
  String name = server.arg("name");
  if (name.length() && !validName(name)) { errorReply(400, "Nombre inválido"); return; }
  if (server.method() == HTTP_DELETE) {
    if (!name.length()) { errorReply(400, "Falta name"); return; }
    for(uint8_t i=0;i<rotationConfig.randomCount;++i)if(name==rotationConfig.randomAnimations[i]){errorReply(409,"Animacion seleccionada para aleatorio; desmarcala primero");return;}
    for(uint8_t i=0;i<rotationConfig.count;++i)if(rotationConfig.items[i].type==RotationPage::Animation && name==rotationConfig.items[i].animation){errorReply(409,"Animacion incluida en la rotacion; retirala primero");return;}
    if (baseScene.animation == name || currentScene.animation == name) { errorReply(409, "Animación en uso"); return; }
    for (uint8_t i = 0; i < queueCount; ++i) if (notificationQueue[i].animation == name) { errorReply(409, "Animación en cola"); return; }
    if (!LittleFS.exists(animationPath(name))) { errorReply(404, "Animación no encontrada"); return; }
    if (!LittleFS.remove(animationPath(name))) { errorReply(500, "Error al borrar"); return; }
    okReply(); return;
  }
  if (name.length()) {
    File file = LittleFS.open(animationPath(name), "r");
    if (!file) { errorReply(404, "Animación no encontrada"); return; }
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + ".awx\"");
    server.streamFile(file, "application/octet-stream"); return;
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,"application/json; charset=utf-8","");server.sendContent("{\"animations\":[");
  bool first=true;AnimationDirectory dir("/animations");
  while(dir.next()){
    if(!dir.fileName().endsWith(".awx"))continue;
    File file=dir.openFile("r");AnimationInfo info;if(!inspectAnimation(file,info))continue;
    StaticJsonDocument<256> item;String filename=dir.fileName();int slash=filename.lastIndexOf('/');
    item["name"]=filename.substring(slash+1,filename.length()-4);item["width"]=info.width;item["height"]=info.height;
    item["frames"]=info.frames;item["frameMs"]=info.frameMs;item["bytes"]=file.size();
    String body;serializeJson(item,body);if(!first)server.sendContent(",");first=false;server.sendContent(body);yield();
  }
  server.sendContent("]}");server.sendContent("");
}
void handleStatus() {
  if (!authorized()) return;
  DynamicJsonDocument doc(8192);
  doc["firmware"] = "Pixel-MB ESP32 1.0.0"; doc["api"] = "v3-subset+local-v1";
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["port"] = HTTP_PORT; doc["hostname"] = deviceHostname;
  doc["wifiSsid"] = wifiSsid;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  if (WiFi.status() == WL_CONNECTED) doc["wifiRssi"] = WiFi.RSSI(); else doc["wifiRssi"] = nullptr;
  doc["wifiStatus"] = int(WiFi.status());doc["wifiDisconnectReason"] = wifiDisconnectReason;
  JsonObject ota = doc.createNestedObject("ota");
  ota["enabled"] = true; ota["ready"] = otaStarted;
  ota["updating"] = otaUpdating; ota["port"] = OTA_PORT;
  ota["hostname"] = deviceHostname; ota["passwordRequired"] = true;
  ota["httpPath"] = "/update"; ota["httpPasswordRequired"] = webAccess.enabled;
  doc["wifiReconnectAttempts"] = wifiReconnectAttempts;
  doc["wifiRetrySeconds"] = WifiRecovery::RETRY_MS / 1000;
  doc["wifiFallbackSeconds"] = WifiRecovery::AP_DELAY_MS / 1000;
  doc["apActive"] = apStarted; doc["fsReady"] = fsReady;
  doc["freeHeap"] = ESP.getFreeHeap(); doc["uptimeSeconds"] = millis() / 1000;
  doc["width"] = MATRIX_WIDTH; doc["height"] = MATRIX_HEIGHT; doc["Brightness"] = brightness;
  doc["matrixPin"] = MATRIX_PIN; doc["ledBufferReady"] = matrix.getPixels() != nullptr;
  doc["ledCount"] = matrix.numPixels(); doc["matrixReady"] = matrixReady;
  doc["actualMatrixPin"] = matrix.getPin();
  doc["showSeconds"] = showSeconds;
  doc["bootLogoActive"] = bootLogoActive;
  doc["wifiIpNoticeActive"] = connectionIpNotice.active() || connectionIpNotice.pending();
  doc["wifiIpNoticeText"] = connectionIpNotice.text();
  doc["wifiIpNoticeDurationMs"] = CONNECTION_IP_NOTICE_MS;
  doc["notifying"] = notifying; doc["queued"] = queueCount;
  doc["animationActive"] = player.active();
  doc["animationFrame"] = player.frameIndex();
  doc["resolvedText"] = renderedText == "\x01" ? String("") : renderedText;
  doc["textX"] = textX; doc["textWidth"] = textWidth;
  rotationJson(doc.createNestedObject("rotation"));
  doc["epoch"] = static_cast<uint32_t>(time(nullptr));
  sceneJson(doc.createNestedObject("scene"), currentScene);
  sceneJson(doc.createNestedObject("baseScene"), baseScene);
  if (fsReady) { doc["fsUsed"] = LittleFS.usedBytes(); doc["fsTotal"] = LittleFS.totalBytes(); }
  jsonReply(200, doc);
}
void handleSensors() {
  if (!authorized()) return;
  StaticJsonDocument<768> doc;
  JsonObject bmp = doc.createNestedObject("bmp280");
  bmp["enabled"] = BMP280_ENABLED; bmp["detected"] = bmpSensor.detected(); bmp["valid"] = bmpSensor.valid();
  bmp["model"] = bmpSensor.isBme()?"BME280":"BMP280";
  if(bmpSensor.humidityValid())bmp["humidityPercent"]=bmpSensor.humidity();else bmp["humidityPercent"]=nullptr;
  bmp["address"] = bmpSensor.address(); bmp["chipId"] = bmpSensor.observedChipId();
  bmp["sdaPin"] = bmpSensor.sdaPin(); bmp["sclPin"] = bmpSensor.sclPin();
  if (bmpSensor.valid()) {
    bmp["temperatureC"] = bmpSensor.temperatureC(); bmp["pressureHpa"] = bmpSensor.pressureHpa();
    bmp["ageMs"] = uint32_t(millis() - bmpSensor.sampledAt());
  } else { bmp["temperatureC"] = nullptr; bmp["pressureHpa"] = nullptr; bmp["ageMs"] = nullptr; }
  bmp["error"] = bmpSensor.error(); jsonReply(200, doc);
}
void handleWifiScan() {
  if (!authorized()) return;
  if (server.method() == HTTP_POST) {
    if (wifiChangePending) { errorReply(409, "Espera a que se aplique la conexion WiFi"); return; }
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
      WiFi.scanDelete(); wifiScanResult = "";
      wifiScanRequested = true; wifiScanStartedAt = millis();
      // El core activa STA conservando AP. El escaneo no bloquea el loop.
      if (WiFi.scanNetworks(true, false) == WIFI_SCAN_FAILED) {
        wifiScanRequested = false; errorReply(503, "No se pudo iniciar el escaneo WiFi"); return;
      }
    }
    server.send(202, "application/json", "{\"scanning\":true}"); return;
  }
  if (wifiScanResult.length()) { server.send(200, "application/json; charset=utf-8", wifiScanResult); return; }
  if (!wifiScanRequested) { errorReply(409, "Inicia un nuevo escaneo WiFi"); return; }
  const int found = WiFi.scanComplete();
  if (found == WIFI_SCAN_RUNNING) { server.send(202, "application/json", "{\"scanning\":true}"); return; }
  wifiScanRequested = false;
  if (found < 0) { WiFi.scanDelete(); errorReply(503, "El escaneo WiFi ha fallado"); return; }
  uint8_t selected[WIFI_SCAN_LIMIT];
  const uint8_t count = selectWifiNetworks(WiFi, found, selected);
  DynamicJsonDocument result(4096); result["scanning"] = false;
  JsonArray networks = result.createNestedArray("networks");
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t index = selected[i]; JsonObject item = networks.createNestedObject();
    item["ssid"] = WiFi.SSID(index); item["rssi"] = WiFi.RSSI(index);
    item["channel"] = WiFi.channel(index); item["secure"] = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete(); // no retener en RAM los BSSID y datos del SDK
  if (result.overflowed()) { errorReply(500, "No se pudo preparar la lista de redes"); return; }
  serializeJson(result, wifiScanResult);
  server.send(200, "application/json; charset=utf-8", wifiScanResult);
}
void handleNetwork() {
  if (!authorized()) return;
  if (server.method() == HTTP_GET) {
    StaticJsonDocument<1024> doc;
    doc["staticIp"] = networkAddress.enabled;
    doc["address"] = networkIPText(networkAddress.ip);
    doc["gateway"] = networkIPText(networkAddress.gateway);
    doc["subnet"] = networkIPText(networkAddress.subnet);
    doc["dns1"] = networkIPText(networkAddress.dns1);
    doc["dns2"] = networkIPText(networkAddress.dns2);
    doc["ssid"] = wifiSsid; doc["connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifiStatus"] = int(WiFi.status());doc["disconnectReason"] = wifiDisconnectReason;
    doc["connecting"] = wifiChangePending || (wifiSsid.length() && WiFi.status() != WL_CONNECTED);
    doc["ip"] = WiFi.localIP().toString(); doc["apActive"] = apStarted;
    doc["apSsid"] = WiFi.softAPSSID(); doc["apIp"] = WiFi.softAPIP().toString();
    jsonReply(200, doc); return;
  }
  DynamicJsonDocument doc(4096); if (!readJson(doc)) return;
  const bool addressOnly=!doc.containsKey("ssid");
  if ((addressOnly && (!doc.containsKey("staticIp") || doc.containsKey("password"))) ||
      (!addressOnly && !doc["ssid"].is<const char *>()) ||
      (doc.containsKey("password") && !doc["password"].is<const char *>())) {
    errorReply(400, "Se requiere ssid o staticIp; password requiere ssid"); return;
  }
  if(addressOnly&&!wifiSsid.length()){errorReply(400,"Configura primero una red WiFi");return;}
  if(wifiChangePending){errorReply(409,"Espera a que se aplique la conexion WiFi");return;}
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    errorReply(409, "Espera a que termine el escaneo antes de conectar"); return;
  }
  String ssid = addressOnly ? wifiSsid : doc["ssid"].as<String>();
  String password = doc.containsKey("password") ? doc["password"].as<String>() : (ssid==wifiSsid ? wifiPassword : String());
  NetworkAddress next=networkAddress;
  if(doc.containsKey("staticIp")){
    if(!doc["staticIp"].is<bool>()){errorReply(400,"staticIp debe ser booleano");return;}
    next.enabled=doc["staticIp"].as<bool>();
  }
  if(next.enabled){
    const char *keys[]={"address","gateway","subnet","dns1","dns2"};
    uint32_t *fields[]={&next.ip,&next.gateway,&next.subnet,&next.dns1,&next.dns2};
    for(uint8_t i=0;i<5;++i){
      if(!doc.containsKey(keys[i]))continue;
      const char *value=doc[keys[i]].as<const char *>();
      if(i==4&&value&&!*value){next.dns2=0;continue;}
      if(!parseNetworkIPv4(value,*fields[i])){errorReply(400,"Direccion IPv4 invalida");return;}
    }
    if(!validNetworkAddress(next)){errorReply(400,"IP fija invalida: revisa IP, mascara, puerta de enlace y DNS");return;}
  }
  if (!ssid.length() || ssid.length() > 32 ||
      (password.length() && (password.length() < 8 || password.length() > 63))) {
    errorReply(400, "SSID: 1..32 bytes; clave WPA: 8..63 bytes, o vacia para red abierta"); return;
  }
  if (!saveNetworkConfig(ssid, password, next)) { errorReply(507, "No se pudo guardar la red"); return; }
  wifiSsid = ssid; wifiPassword = password; networkAddress=next;
  wifiChangePending = true; wifiChangeQueuedAt = millis();
  server.send(202, "application/json", "{\"ok\":true,\"message\":\"Red guardada; iniciando conexion\"}");
}
bool httpOtaUploadStarted = false, httpOtaSuccess = false;
String httpOtaError;
void handleWebAccess() {
  if(!authorized())return;
  if(server.method()==HTTP_GET){
    StaticJsonDocument<192> response;response["enabled"]=webAccess.enabled;response["username"]=webAccess.username;response["passwordSet"]=webAccess.password.length()>0;jsonReply(200,response);return;
  }
  if(!requireFS())return;
  DynamicJsonDocument request(4096);if(!readJson(request,4096))return;
  if(!request["enabled"].is<bool>() || !request["username"].is<const char *>()){errorReply(400,"Usuario y estado de acceso invalidos");return;}
  WebAccessConfig next=webAccess;next.enabled=request["enabled"];next.username=request["username"].as<String>();
  if(request.containsKey("password")){
    if(!request["password"].is<const char *>()){errorReply(400,"Contrasena invalida");return;}
    String password=request["password"].as<String>();if(password.length())next.password=password;
  }
  if(request["clearPassword"]==true && !next.enabled)next.password="";
  if(!validWebUsername(next.username) || (next.password.length()&&!validWebPassword(next.password)) || (next.enabled&&!validWebPassword(next.password))){errorReply(400,"Usuario: 1..32 letras, numeros, . @ _ -. Introduce una contrasena para activar el acceso");return;}
  if(!saveWebAccess(next)){errorReply(507,"No se pudo guardar el acceso web");return;}
  webAccess=next;okReply();
}
void setupHttpOTA() {
  server.on("/update", HTTP_GET, []() {
    if(!webAuthorized())return;
    server.send_P(200, "text/html; charset=utf-8", UPDATE_UI);
  });
  server.on("/update", HTTP_POST, []() {
    if(!webAuthorized())return;
    if (!httpOtaUploadStarted) { errorReply(400, "Falta firmware.bin"); return; }
    if (!httpOtaSuccess) {
      otaUpdating = false;
      server.send(400, "application/json", "{\"error\":\"Firmware update failed; previous firmware retained\"}");
      activate(currentScene); return;
    }
    server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
    delay(150); ESP.restart();
  }, []() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      httpOtaUploadStarted = webAuthenticated();
      httpOtaSuccess = false; httpOtaError = "";
      if(!httpOtaUploadStarted)return;
      if (upload.name != "firmware" || !upload.filename.endsWith(".bin")) {
        httpOtaError = "Only firmware .bin uploads are accepted"; return;
      }
      otaUpdating = true; player.close(); uploadFile.close();
      if (matrixReady) { matrix.clear(); matrix.show(); }
      const size_t maxSpace = UPDATE_SIZE_UNKNOWN;
      if (!Update.begin(maxSpace, U_FLASH)) httpOtaError = "Cannot start firmware update";
      Serial.println("HTTP OTA: actualizacion web iniciada");
    } else if (httpOtaUploadStarted && upload.status == UPLOAD_FILE_WRITE && !httpOtaError.length()) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) httpOtaError = "Flash write failed";
    } else if (httpOtaUploadStarted && upload.status == UPLOAD_FILE_END) {
      if (!httpOtaError.length()) httpOtaSuccess = Update.end(true);
      else Update.end();
      if (!httpOtaSuccess) { otaUpdating = false; activate(currentScene); }
      Serial.printf("HTTP OTA: %s (%u bytes)\n", httpOtaSuccess ? "OK" : "ERROR", unsigned(upload.totalSize));
    } else if (httpOtaUploadStarted && upload.status == UPLOAD_FILE_ABORTED) {
      Update.end(); otaUpdating = false; httpOtaSuccess = false;
      activate(currentScene);
    }
    yield();
  });
}

void setupRoutes() {
  const char *headers[]={"X-API-Key"};server.collectHeaders(headers,1);
  setupHttpOTA();
  server.on("/", HTTP_GET, []() { if(!webAuthorized())return; server.send_P(200, "text/html; charset=utf-8", WEB_UI); });
  server.on("/api/web-access", HTTP_GET, handleWebAccess);
  server.on("/api/web-access", HTTP_POST, handleWebAccess);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/sensors", HTTP_GET, handleSensors);
  server.on("/api/rotation", HTTP_GET, handleRotation);
  server.on("/api/rotation", HTTP_POST, handleRotation);
  server.on("/api/network", HTTP_GET, handleNetwork);
  server.on("/api/network", HTTP_POST, handleNetwork);
  server.on("/api/network/scan", HTTP_POST, handleWifiScan);
  server.on("/api/network/scan", HTTP_GET, handleWifiScan);
  server.on("/api/variables", HTTP_GET, handleVariables);
  server.on("/api/variables", HTTP_POST, handleVariables);
  server.on("/api/display", HTTP_POST, []() { handleDisplay(false); });
  server.on("/api/v3/notify", HTTP_POST, []() { handleDisplay(true); });
  server.on("/api/v3/settings", HTTP_POST, handleSettings);
  server.on("/api/v3/settings", HTTP_GET, handleSettings);
  server.on("/api/animations", HTTP_GET, handleAnimations);
  server.on("/api/animations", HTTP_DELETE, handleAnimations);
  server.on("/api/animations", HTTP_POST, finishUpload, handleUploadChunk);
  server.on("/api/save", HTTP_POST, []() {
    if (!authorized() || !requireFS()) return;
    if (!saveState()) { errorReply(507, "No se pudo guardar el estado"); return; }
    okReply();
  });
  server.on("/api/stop", HTTP_POST, []() { if (!authorized()) return; clearQueue(); restoreBase(); okReply(); });
  server.on("/api/time", HTTP_POST, []() {
    if (!authorized()) return;
    DynamicJsonDocument doc(4096); if (!readJson(doc)) return;
    if (!unsignedIn(doc["epoch"], 1577836800UL, 4102444799UL)) { errorReply(400, "epoch debe ser segundos Unix entre 2020 y 2099"); return; }
    timeval tv = {static_cast<time_t>(doc["epoch"].as<uint32_t>()), 0}; settimeofday(&tv, nullptr); okReply();
  });
  server.onNotFound([]() {
    if(!webAuthorized())return;
    if (apStarted && !server.uri().startsWith("/api/")) {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "Abrir Pixel-MB"); return;
    }
    errorReply(404, "Ruta no encontrada");
  });
  server.begin();
}
void startAP() {
  if (apStarted) return;
  WiFi.mode(wifiSsid.length() ? WIFI_AP_STA : WIFI_AP);
  const IPAddress apIp(192, 168, 4, 1);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  String ssid = "PIXEL-MB-" + deviceId;
  apStarted = WiFi.softAP(ssid.c_str(), SETUP_AP_PASSWORD);
  if (!apStarted) { Serial.println("ERROR al iniciar el punto de acceso"); return; }
  dns.start(53, "*", apIp);
  auto redirect = []() {
    portal.sendHeader("Location", "http://192.168.4.1:" + String(HTTP_PORT) + "/", true);
    portal.send(302, "text/plain", "Abrir el portal WiFi de Pixel-MB");
  };
  if (HTTP_PORT != 80) {
    portal.on("/", HTTP_GET, redirect); portal.onNotFound(redirect); portal.begin();
  }
  Serial.printf("\nPunto de acceso: %s\nClave: %s\n", ssid.c_str(), SETUP_AP_PASSWORD);
  Serial.println("Configuracion inicial: http://192.168.4.1/");
}
void setupOTA() {
  ArduinoOTA.setHostname(deviceHostname.c_str());
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setPasswordHash(OTA_PASSWORD_HASH);
  ArduinoOTA.onStart([]() {
    otaUpdating = true;
    player.close(); uploadFile.close();
    if (matrixReady) { matrix.clear(); matrix.show(); }
    otaFsClosed = ArduinoOTA.getCommand() == U_SPIFFS && fsReady;
    if (otaFsClosed) { LittleFS.end(); fsReady = false; }
    Serial.println("OTA: actualizacion web iniciada");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 101;
    const unsigned int percent = total ? progress * 100UL / total : 0;
    if (percent / 10 != lastPercent / 10) {
      Serial.printf("OTA: %u%%\n", percent); lastPercent = percent;
    }
  });
  ArduinoOTA.onEnd([]() { Serial.println("OTA: carga completada; reiniciando"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA: error %u; se conserva el firmware anterior\n", unsigned(error));
    otaUpdating = false;
    if (otaFsClosed) { fsReady = LittleFS.begin(); otaFsClosed = false; }
    activate(currentScene);
  });
}
void otaLoop() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !otaStarted) {
    // mDNS is shared with the HTTP service; do not restart it inside ArduinoOTA.
    ArduinoOTA.setMdnsEnabled(false); ArduinoOTA.begin();
    otaStarted = true;
    if (mdnsStarted) MDNS.enableArduino(OTA_PORT, true);
    Serial.printf("OTA listo: %s:%u (requiere clave de ota.local.json)\n",
      WiFi.localIP().toString().c_str(), OTA_PORT);
  } else if (!connected && otaStarted) {
    ArduinoOTA.end(); otaStarted = false;
  }
  if (otaStarted) ArduinoOTA.handle();
}
void wifiLoop() {
  if ((wifiScanRequested || wifiScanResult.length()) &&
      uint32_t(millis() - wifiScanStartedAt) > 30000 && WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    WiFi.scanDelete(); wifiScanRequested = false; wifiScanResult = "";
  }
  if (wifiChangePending && uint32_t(millis() - wifiChangeQueuedAt) >= 500) {
    wifiChangePending = false;
    if (!apStarted) startAP(); // conservar acceso mientras se prueba la nueva red
    if (otaStarted) { ArduinoOTA.end(); otaStarted = false; }
    if (mdnsStarted) { MDNS.end(); mdnsStarted = false; }
    WiFi.disconnect(false); WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(false); Serial.printf("WiFi: configuracion IP %s, SSID de %u bytes\n",applyNetworkAddress()?"OK":"ERROR",unsigned(wifiSsid.length())); WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    wifiRecovery.begin(millis());
  }
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (!connected && wifiWasConnected) {
    Serial.println("WiFi: conexion perdida; se intentara recuperar sin detener la pantalla");
    if (mdnsStarted) { MDNS.end(); mdnsStarted = false; }
  }
  if (!connected) connectionIpNotice.cancel();
  if (connected) {
    const IPAddress ip=WiFi.localIP();const uint32_t address=static_cast<uint32_t>(ip);
    if(address && (!wifiWasConnected || address!=lastConnectedIp)){
      connectionIpNotice.request(ip[0],ip[1],ip[2],ip[3]);
      Serial.printf("WiFi conectado: IP %s (se muestra 4 segundos)\n",connectionIpNotice.text());
    }
    lastConnectedIp=address;
  }
  wifiWasConnected = connected;
  const auto recovery = wifiRecovery.tick(millis(), connected, wifiSsid.length(), apStarted,
    wifiChangePending || WiFi.scanComplete() == WIFI_SCAN_RUNNING);
  if (recovery.startAccessPoint) startAP();
  if (recovery.reconnect) {
    ++wifiReconnectAttempts;
    Serial.printf("WiFi: reintento %lu; el portal sigue disponible si esta activo\n", static_cast<unsigned long>(wifiReconnectAttempts));
    WiFi.disconnect(false); // Solo STA: no apagar el punto de acceso.
    WiFi.mode(apStarted ? WIFI_AP_STA : WIFI_STA);
    WiFi.setAutoReconnect(false);
    applyNetworkAddress(); WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  }
  if (apStarted) { dns.processNextRequest(); if (HTTP_PORT != 80) portal.handleClient(); }
  if (WiFi.status() == WL_CONNECTED && !mdnsStarted) {
    Serial.printf("Editor/API: http://%s:%u/\n", WiFi.localIP().toString().c_str(), HTTP_PORT);
    mdnsStarted = MDNS.begin(deviceHostname.c_str());
    if (mdnsStarted) {
      MDNS.addService("http", "tcp", HTTP_PORT);
      if (otaStarted) MDNS.enableArduino(OTA_PORT, true);
    }
  }

}
void drawSceneText(const String &text, int16_t left, uint32_t color, uint16_t scrollMs, uint32_t now) {
  const int16_t available = MATRIX_WIDTH - left;
    if (text != renderedText) {
      const bool wasScrolling = textWidth > available;
      renderedText = text;
      int16_t x, y; uint16_t w, h;
      matrix.getTextBounds(text, 0, 6, &x, &y, &w, &h);
      textWidth = w ? w + max<int16_t>(x, 0) : 0;
      const bool willScroll = textWidth > available;
      if (!textInitialized || wasScrolling != willScroll) {
        textX = willScroll ? left : left + (available - textWidth) / 2;
        scrollAt = now; scrollLoops = 0;
      } else if (!willScroll) textX = left + (available - textWidth) / 2;
      textInitialized = true;
    }
    bool scrolling = textWidth > available;
    if (scrolling && uint32_t(now - scrollAt) >= scrollMs) {
      uint32_t steps = uint32_t(now - scrollAt) / scrollMs;
      scrollAt += steps * scrollMs; textX -= min<uint32_t>(steps, 256);
      if (textX + textWidth < left) { textX = MATRIX_WIDTH; ++scrollLoops; }
    }
    matrix.setTextColor(matrix.Color(color >> 16, color >> 8, color));
    matrix.setCursor(textX, 6); matrix.print(text);
}
void displayLoop() {
  if (!matrixReady) return;
  const uint32_t now = millis();
  if (uint32_t(now - renderAt) < 20) return;
  renderAt = now;
  if(bootLogoActive){
    const uint32_t elapsed=now-bootLogoStartedAt;
    if(elapsed<BOOT_LOGO_DURATION_MS){
      matrix.setBrightness(DEFAULT_BRIGHTNESS);matrix.fillScreen(0);
      drawBootLogo(matrix,elapsed);matrix.show();return;
    }
    bootLogoActive=false;matrix.setBrightness(brightness);activate(currentScene);
    return;
  }
  if(connectionIpNotice.tick(now)){
    activate(currentScene); // Reanudar el contenido y su tiempo completo, sin tocar la cola.
    return;
  }
  if(connectionIpNotice.active()){
    matrix.fillScreen(0);
    drawConnectionIp(matrix,connectionIpNotice.text(),connectionIpNotice.elapsed(now),matrix.Color(0,180,255));
    matrix.show();return;
  }
  if (currentScene.durationMs && uint32_t(now - sceneStart) >= currentScene.durationMs) {
    if (notifying) nextNotification();
    else { baseScene = Scene(); if (rotationConfig.enabled) baseScene.mode = "rotation"; activate(baseScene); }
    return; // activate() puede actualizar sceneStart después del now de este ciclo
  }
  matrix.fillScreen(0);
  if (currentScene.mode == "off") { matrix.show(); return; }
  const bool rotating = currentScene.mode == "rotation";
  RotationPage page = RotationPage::Clock;
  if (rotating) {
    page = rotationScheduler.tick(now, rotationConfig, bmpSensor.valid());
    const bool changed=rotationRenderIndex!=rotationScheduler.index() || rotationRenderGeneration!=rotationScheduler.generation();
    if(changed && page==RotationPage::Random){
      randomAnimation="";
      if(rotationConfig.randomCount){
        uint8_t candidates[MAX_RANDOM_ANIMATIONS],count=0;
        for(uint8_t i=0;i<rotationConfig.randomCount;++i)
          if(rotationConfig.randomCount==1 || previousRandomAnimation!=rotationConfig.randomAnimations[i])candidates[count++]=i;
        randomAnimation=rotationConfig.randomAnimations[candidates[random(count)]];
        previousRandomAnimation=randomAnimation;
      }
    }
    const String animation=page==RotationPage::Random?randomAnimation:page==RotationPage::Animation?String(rotationConfig.items[rotationScheduler.index()].animation):String();
    if(changed || player.name()!=animation){
      rotationRenderIndex=rotationScheduler.index();rotationRenderGeneration=rotationScheduler.generation();
      renderedText="\x01"; textInitialized=false; scrollLoops=0;
      player.close();if(animation.length())player.open(animation);
    }
    if(page==RotationPage::Random && !rotationConfig.randomCount)page=RotationPage::Clock;
  }
  if(rotating && (page==RotationPage::Animation || page==RotationPage::Random)){
    if(player.active()){
      player.tick(now);const AnimationInfo &info=player.info();const uint16_t *pixels=player.pixels();
      const auto &item=rotationConfig.items[rotationScheduler.index()];
      const bool withText=item.text[0] && info.width<=8 && info.height<=8;
      if(withText) {
        drawSceneText(displayAscii(expandText(String(item.text))),info.width+1,item.color,70,now);
        matrix.fillRect(0,0,info.width+1,MATRIX_HEIGHT,0);
      }
      const int16_t ax=withText?0:(MATRIX_WIDTH-info.width)/2;
      for(uint8_t y=0;y<info.height;++y)for(uint8_t x=0;x<info.width;++x)
        matrix.drawPixel(ax+x,(MATRIX_HEIGHT-info.height)/2+y,pixels[y*info.width+x]);
    } else {
      // Archivo perdido/corrupto: mostrar una marca sin bloquear la secuencia.
      matrix.setCursor(6,6);matrix.setTextColor(matrix.Color(255,0,0));matrix.print("AWX?");
    }
  } else if (rotating && page == RotationPage::Text) {
    const auto &item=rotationConfig.items[rotationScheduler.index()];
    drawSceneText(displayAscii(expandText(String(item.text))),0,item.color,70,now);
  } else if (rotating && page != RotationPage::Clock) {
    const bool temperature = page == RotationPage::Temperature;
    String value = String(temperature ? "T:" : "P:") + String(temperature ? bmpSensor.temperatureC() : bmpSensor.pressureHpa(), 1) + (temperature ? "C" : "");
    int16_t x, y; uint16_t w, h; matrix.getTextBounds(value, 0, 6, &x, &y, &w, &h);
    matrix.setCursor((int16_t(MATRIX_WIDTH) - int16_t(w)) / 2 - x, 6);
    const uint32_t color=rotationConfig.items[rotationScheduler.index()].color;
    matrix.setTextColor(matrix.Color(color>>16,color>>8,color));
    matrix.print(value);
  } else if (rotating || currentScene.mode == "clock") {
    char text[9];
    time_t epoch = time(nullptr);
    tm local; const tm *validTime = nullptr;
    if (epoch >= 1577836800 && localtime_r(&epoch, &local)) validTime = &local;
    const bool clockSeconds=rotating?rotationConfig.items[rotationScheduler.index()].showSeconds:showSeconds;
    const bool clockWeekdays=!rotating || rotationConfig.items[rotationScheduler.index()].showWeekdays;
    formatClockText(text, validTime, clockSeconds);
    const uint32_t color=rotating?rotationConfig.items[rotationScheduler.index()].color:0x00b4ff;
    drawFixedClock(matrix, text, clockSeconds, matrix.Color(color>>16,color>>8,color));
    if(clockWeekdays)drawWeekdayIndicators(matrix,validTime,matrix.Color(40,88,176),matrix.Color(255,255,255));
  } else {
    String text = displayAscii(expandText(currentScene.text));
    const int16_t left = player.active() && text.length() ? player.info().width + 1 : 0;
    const int16_t available = MATRIX_WIDTH - left;
    drawSceneText(text, left, currentScene.color, currentScene.scrollMs, now);
    const bool scrolling = textWidth > available;
    if (player.active()) {
      player.tick(now);
      const AnimationInfo &info = player.info();
      if (text.length()) matrix.fillRect(0, 0, left, MATRIX_HEIGHT, 0);
      int16_t ax = text.length() ? 0 : (MATRIX_WIDTH - info.width) / 2;
      int16_t ay = (MATRIX_HEIGHT - info.height) / 2;
      const uint16_t *pixels = player.pixels();
      for (uint8_t y = 0; y < info.height; ++y)
        for (uint8_t x = 0; x < info.width; ++x) matrix.drawPixel(ax + x, ay + y, pixels[y * info.width + x]);
    }
    if (notifying && !currentScene.durationMs) {
      bool complete = scrolling ? scrollLoops >= currentScene.repeat :
        uint32_t(now - sceneStart) >= 5000UL * currentScene.repeat;
      if (complete || uint32_t(now - sceneStart) >= 3600000UL) nextNotification();
    }
  }
  matrix.show();
}
void ledBootTest() {
  if (!LED_BOOT_TEST) return;
  Serial.printf("Prueba LED: GPIO%u, %u pixeles; primeros 8: rojo, verde, azul\n",
    MATRIX_PIN, matrix.numPixels());
  matrix.setBrightness(16);
  const uint32_t colors[] = {0xff0000, 0x00ff00, 0x0000ff};
  for (uint32_t color : colors) {
    matrix.clear();
    for (uint16_t i = 0; i < min<uint16_t>(8, matrix.numPixels()); ++i)
      matrix.setPixelColor(i, color); // Indices fisicos: no depende del zigzag ni fuente.
    matrix.show(); delay(500); // Solo en arranque; delay() atiende el watchdog.
  }
  matrix.clear(); matrix.show();
  Serial.println("Prueba LED enviada; no detecta electricamente si hay luz.");
}
void setup() {
  Serial.begin(115200); Serial.println("\nPixel-MB ESP32 1.0.0 — Arduino / ESP32");
  matrixReady = matrix.configurationValid();
  Serial.printf("Matriz: GPIO real=%d, LED reales=%u, esperados=%u, heap=%u\n",
    matrix.getPin(), matrix.numPixels(), uint16_t(MATRIX_WIDTH) * MATRIX_HEIGHT, ESP.getFreeHeap());
  if (matrixReady) {
    matrix.begin(); ledBootTest();
    if(BOOT_LOGO_ENABLED){
      bootLogoActive=true;bootLogoStartedAt=millis();
      matrix.setBrightness(DEFAULT_BRIGHTNESS);matrix.clear();drawBootLogo(matrix,0);matrix.show();
    }
  }
  else Serial.println("ERROR de configuracion/buffer LED: se omite el control de matriz; WiFi sigue activo");
  variables.to<JsonObject>();
  fsReady = mountAnimationStorage();
  if (fsReady) {
    LittleFS.remove("/animation.tmp"); LittleFS.remove("/state.tmp"); LittleFS.remove("/rotation.tmp");
    if (!installDefaultAnimations()) Serial.println("No se pudieron instalar todos los iconos iniciales");
    loadState(); loadRotation(); loadWebAccess(webAccess);
  }
  else Serial.println("LittleFS no montado. Cargar data/ con pio run -t uploadfs (solo instalación inicial).");
  matrix.setTextWrap(false); matrix.setFont(&TomThumb);
  if (rotationConfig.enabled) { baseScene = Scene(); baseScene.mode = "rotation"; }
  matrix.setBrightness(brightness); activate(baseScene);
  Serial.printf("Estado inicial: brillo=%u, escena=%s, LittleFS=%s\n",
    brightness, baseScene.mode.c_str(), fsReady ? "OK" : "NO");
  WiFi.onEvent([](WiFiEvent_t event,WiFiEventInfo_t info){
    if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED){wifiDisconnectReason=info.wifi_sta_disconnected.reason;Serial.printf("WiFi: desconectado, motivo=%u\n",unsigned(wifiDisconnectReason));}
    else if(event==ARDUINO_EVENT_WIFI_STA_CONNECTED){wifiDisconnectReason=0;Serial.println("WiFi: asociado al router; esperando IP");}
    else if(event==ARDUINO_EVENT_WIFI_STA_GOT_IP){wifiDisconnectReason=0;Serial.println("WiFi: direccion IP obtenida");}
  });
  char id[7];
  snprintf(id, sizeof(id), "%06lx", (unsigned long)(ESP.getEfuseMac() & 0xffffff));
  deviceId = id; deviceHostname = String(HOSTNAME) + "-" + deviceId;
  Serial.printf("Nombre de red: %s.local\n", deviceHostname.c_str());
  WiFi.persistent(false); WiFi.setHostname(deviceHostname.c_str());WiFi.setSleep(false);
  wifiRecovery.begin(millis());
  wifiSsid = WIFI_SSID; wifiPassword = WIFI_PASSWORD;
  loadNetworkConfig(wifiSsid, wifiPassword, networkAddress);
  if (wifiSsid.length()) {
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(false); Serial.printf("WiFi: configuracion IP %s, SSID de %u bytes\n",applyNetworkAddress()?"OK":"ERROR",unsigned(wifiSsid.length())); WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str()); wifiRecovery.begin(millis());
  } else startAP();
  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");
  if(BUTTONS_ENABLED){pinMode(BUTTON_LEFT_PIN,INPUT_PULLUP);pinMode(BUTTON_MIDDLE_PIN,INPUT_PULLUP);pinMode(BUTTON_RIGHT_PIN,INPUT_PULLUP);}
  setupRoutes();
  setupOTA();
  if(bootLogoActive)bootLogoStartedAt=millis();
}
void buttonsLoop(){
  if(!BUTTONS_ENABLED)return;
  const uint8_t pins[]={BUTTON_LEFT_PIN,BUTTON_MIDDLE_PIN,BUTTON_RIGHT_PIN};
  for(uint8_t i=0;i<3;++i){
    if(!buttons[i].pressed(millis(),digitalRead(pins[i])==LOW))continue;
    if(i==1){if(brightness){lastBrightness=brightness;brightness=0;}else brightness=lastBrightness?lastBrightness:DEFAULT_BRIGHTNESS;matrix.setBrightness(brightness);continue;}
    if(notifying){notifying=false;queueCount=0;}
    baseScene=Scene();baseScene.mode="rotation";
    rotationScheduler.step(millis(),rotationConfig,bmpSensor.valid(),i==2?1:-1);
    currentScene=baseScene;player.close();renderedText="\x01";rotationRenderIndex=255;
  }
}
void loop() {
  wifiLoop(); otaLoop();
  if (otaUpdating) { yield(); return; }
  server.handleClient(); buttonsLoop(); displayLoop();
  bmpSensor.tick(millis());
  static uint32_t lastSensorLog = 0;
  if (uint32_t(millis() - lastSensorLog) >= 10000) {
    lastSensorLog = millis();
    if (bmpSensor.valid()) Serial.printf("BMP280: SDA=GPIO%u SCL=GPIO%u addr=0x%02x T=%.1f C P=%.1f hPa\n",
      bmpSensor.sdaPin(), bmpSensor.sclPin(), bmpSensor.address(), bmpSensor.temperatureC(), bmpSensor.pressureHpa());
    else Serial.printf("BMP280: %s (ID=0x%02x)\n", bmpSensor.error(), bmpSensor.observedChipId());
  }
  yield();
}
