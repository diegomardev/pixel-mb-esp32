# Actualización de firmware

## Desde la web

Abre «Firmware y copias de seguridad» → «Actualizar firmware», selecciona el `.bin` compilado para tu placa y pulsa «Cargar firmware». Por defecto no se pide usuario ni contraseña. También puedes abrir directamente `http://IP_DEL_ESP/update`.

El archivo generado por PlatformIO está en `.pio/build/esp32dev/firmware.bin`. La actualización conserva la WiFi, los ajustes y las animaciones. Mantén la alimentación durante la carga y el reinicio. No cargues imágenes de LittleFS, copias completas de flash ni firmware de otra plataforma. Mientras la protección esté desactivada, cualquier equipo con acceso a la web de la placa puede actualizarla.

## Desde PlatformIO

La primera carga se hace por USB:

```sh
pio run -e esp32dev -t upload
```

Para cargas HTTP posteriores, configura el campo `host` del archivo privado `ota.local.json` con la IP de la placa:

```sh
pio run -e esp32dev_ota -t upload
```

El cargador HTTP utiliza únicamente el host; no necesita credenciales ni una conexión de retorno del ESP al ordenador. No uses `uploadfs` para actualizar: sustituiría los archivos guardados.

## OTA clásica de Arduino

La OTA clásica en el puerto 8266 conserva su contraseña del archivo privado `ota.local.json`. Es independiente del actualizador web sin contraseña. Conserva ese archivo y no lo publiques: contiene la clave de la OTA clásica.

### Protección web opcional

En «Acceso a la web» puedes activar un usuario y una contraseña para toda la interfaz, API y actualizador. **Deshabilitada por defecto**: mientras esté apagada puedes cargar el `.bin` directamente. Se usa autenticación HTTP Digest y los ajustes se guardan en la placa. La contraseña no se devuelve por API ni se incluye en las copias. Si la olvidas, debes borrar la configuración por USB. `GET/POST /api/web-access` consulta o cambia `enabled`, `username` y `password`; dejar la contraseña vacía conserva la anterior. Una vez activada, las llamadas API también requieren autenticación Digest. La OTA clásica conserva su clave independiente.

Si activas la protección web y quieres usar el cargador HTTP de PlatformIO, añade `webUsername` y `webPassword` al archivo privado `ota.local.json`. Sin protección web no hacen falta estos campos.
