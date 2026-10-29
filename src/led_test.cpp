// Prueba independiente. Seleccionar entorno PlatformIO "led-test".
// Usa el mismo GPIO y tipo de LED de include/Config.h.
#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

Adafruit_NeoPixel strip(MATRIX_WIDTH * MATRIX_HEIGHT, MATRIX_PIN, MATRIX_TYPE);
uint8_t phase = 0;
uint16_t pixel = 0;
uint32_t lastStep = 0;
bool firstStep = true;

void setup() {
  Serial.begin(115200);
  WiFi.persistent(false); WiFi.mode(WIFI_OFF);
  strip.begin(); strip.setBrightness(16); strip.clear(); strip.show();
  Serial.printf("\nPRUEBA LED INDEPENDIENTE: GPIO%u, %u LED\n", MATRIX_PIN, strip.numPixels());
  Serial.println("Primeros 8: rojo / verde / azul; despues un pixel blanco recorre toda la tira.");
  Serial.println("Esta prueba no lee el brillo ni la escena guardados y no sirve una pagina web.");
  if (!strip.getPixels()) Serial.println("ERROR: no hay buffer para los LED");
}
void loop() {
  const uint32_t now = millis();
  const uint32_t interval = (phase < 3 || (phase == 3 && pixel == 0)) ? 1500 : 40;
  if (!firstStep && uint32_t(now - lastStep) < interval) { yield(); return; }
  lastStep = now; firstStep = false;
  strip.clear();
  if (phase < 3) {
    const uint32_t colors[] = {0xff0000, 0x00ff00, 0x0000ff};
    for (uint16_t i = 0; i < min<uint16_t>(8, strip.numPixels()); ++i) strip.setPixelColor(i, colors[phase]);
    Serial.printf("Color %u de 3 (rojo, verde, azul)\n", phase + 1);
    ++phase;
  } else {
    strip.setPixelColor(pixel++, 0xffffff);
    if (pixel >= strip.numPixels()) { pixel = 0; phase = 0; }
  }
  strip.show(); yield();
}
