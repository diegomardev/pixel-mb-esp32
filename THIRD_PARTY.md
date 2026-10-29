# Referencias y dependencias

Este firmware es una implementación independiente; no es un producto oficial AWTRIX.

Se consultó [awtrix/awtrix1.0](https://github.com/awtrix/awtrix1.0), de Blueforcer / AWTRIX, copyright 2018. Su repositorio indica [Creative Commons Attribution 4.0](https://creativecommons.org/licenses/by/4.0/). Se usó como referencia de hardware y arquitectura; el firmware antiguo no se incluye en este paquete. El código de esta implementación y los dibujos de demostración se entregan bajo MIT. La forma de las notificaciones v3 procede de los mensajes de ejemplo facilitados por el usuario. Los IDs de los iconos se conservan para facilitar la adaptación, pero los dibujos son nuevos.

PlatformIO descarga las bibliotecas originales con sus propios avisos de licencia:

- [Arduino-ESP32](https://github.com/espressif/arduino-esp32), LGPL-2.1 y componentes con sus respectivas licencias, incluidos SDK/lwIP.
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson), Benoît Blanchon, MIT.
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel), LGPL-3.0.
- [Adafruit NeoMatrix](https://github.com/adafruit/Adafruit_NeoMatrix), LGPL-3.0.
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library), BSD, más atribuciones de fuentes/componentes incluidas por la biblioteca.
- [Adafruit BMP280 Library](https://github.com/adafruit/Adafruit_BMP280_Library), MIT.
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor), Apache-2.0.
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO), MIT.

Los archivos LICENSE originales de las bibliotecas se descargan en `.pio/libdeps/`; los del core están en el paquete del framework Arduino. Antes de redistribuir binarios modificados, conserva los avisos y cumple las obligaciones de cada dependencia. Esta lista no sustituye sus licencias completas.

- [Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library), BSD, usado para temperatura, presión y humedad del BME280.
