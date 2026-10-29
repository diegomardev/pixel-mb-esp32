#pragma once
#include <Adafruit_NeoMatrix.h>

// Red inicial opcional. Se puede configurar desde el portal WiFi; lo guardado allí tiene prioridad.
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char SETUP_AP_PASSWORD[] = "pixel-mb-local"; // Clave del WiFi PIXEL-MB-xxxxxx (cambiar si lo deseas)
constexpr char HOSTNAME[] = "pixel-mb";
// Si no está vacía, protege TODA la API con la cabecera X-API-Key.
// Los clientes externos la envían en X-API-Key. El editor integrado no solicita claves API.
constexpr char API_KEY[] = "";
constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3"; // España peninsular

constexpr uint8_t MATRIX_PIN = 32; // ESP32-WROOM: GPIO32, DIN de la matriz
constexpr uint8_t MATRIX_WIDTH = 32;
constexpr uint8_t MATRIX_HEIGHT = 8;
constexpr uint8_t MATRIX_LAYOUT = NEO_MATRIX_TOP | NEO_MATRIX_LEFT |
    NEO_MATRIX_COLUMNS | NEO_MATRIX_ZIGZAG;
constexpr uint8_t MATRIX_TYPE = NEO_GRB | NEO_KHZ800;
constexpr bool LED_BOOT_TEST = false; // Prueba los primeros 8 LED al arrancar, antes de WiFi/archivos
constexpr bool BOOT_LOGO_ENABLED = true; // Pixel-MB con onda de colores al arrancar
constexpr uint8_t DEFAULT_BRIGHTNESS = 24; // 0..255; empezar con brillo bajo
constexpr uint16_t MAX_FRAMES = 120;
constexpr uint8_t MAX_ANIMATIONS = 48;
constexpr uint32_t MAX_ANIMATION_BYTES = 12UL + MAX_FRAMES * MATRIX_WIDTH * MATRIX_HEIGHT * 2UL;
constexpr uint16_t MAX_JSON_BYTES = 2048;
constexpr uint16_t MAX_TEXT_BYTES = 160;
constexpr uint16_t HTTP_PORT = 80; // Editor/API directamente en http://IP_DEL_ESP/
constexpr uint8_t NOTIFY_QUEUE_SIZE = 8;

// BMP280 o BME280 opcional: SDA GPIO21, SCL GPIO22 (3,3 V).
constexpr bool BMP280_ENABLED = true;
constexpr uint8_t BMP280_SDA_PIN = 21;
constexpr uint8_t BMP280_SCL_PIN = 22;
constexpr bool BMP280_TRY_REVERSED_PINS = false;
constexpr uint32_t BMP280_SAMPLE_MS = 2000;
constexpr uint32_t BMP280_RETRY_MS = 10000;

// Pulsadores opcionales: cada GPIO a GND; resistencias pull-up internas.
constexpr bool BUTTONS_ENABLED = true;
constexpr uint8_t BUTTON_LEFT_PIN = 26;
constexpr uint8_t BUTTON_MIDDLE_PIN = 27;
constexpr uint8_t BUTTON_RIGHT_PIN = 14;
