# OTA · Pixel-MB ESP32

Primera carga por USB: `pio run -e esp32dev -t upload`.

La compilación genera `ota.local.json` e `include/LocalOtaConfig.h`, ambos privados y excluidos de Git. No publiques esos archivos ni binarios compilados con tus credenciales. Cada clon genera claves distintas; conserva las mismas para seguir actualizando tu dispositivo.

Configura `host` con la IP del ESP32 en `ota.local.json` y ejecuta:

```sh
pio run -e esp32dev_ota -t upload
```

Se usa HTTP Digest en `http://IP_DEL_ESP/update`, usuario `admin`, sin conexión de retorno al ordenador. También puedes abrir esa dirección y seleccionar el `firmware.bin` de `.pio/build/esp32dev/`. Este binario solo sirve para ESP32 clásico y esta tabla de particiones, no para ESP8266, ESP32-C3 ni ESP32-S3.

La actualización conserva LittleFS, animaciones, rotación, WiFi e IP fija. La carga OTA de archivos está desactivada para evitar sustituir datos. No interrumpas la alimentación durante la escritura. La tabla de particiones admite dos aplicaciones OTA de 1408 KiB; no cambies las particiones por OTA.
