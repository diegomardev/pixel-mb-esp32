# Pixel-MB — Manual de la API

Referencia del firmware **Pixel-MB ESP32 1.0.0**, para ESP32 y matriz de 32×8.

## 1. Conexión

Dirección actual de tu pantalla:

```text
http://192.168.1.50
```

El editor y la API usan el puerto HTTP 80; no añadas `:7000`. Sustituye la IP en los ejemplos si cambia. Todos los ejemplos se dirigen a una sola pantalla.

Las peticiones con un cuerpo JSON deben llevar `Content-Type: application/json`. Los nombres distinguen mayúsculas y minúsculas, incluidos los nombres de iconos.

Si se configura `API_KEY` en el firmware, añade esta cabecera a todas las peticiones de la API:

```text
X-API-Key: TU_CLAVE
```

Con la configuración actual del proyecto, la clave está vacía y no hace falta esa cabecera. La autenticación de OTA es independiente.

## 2. Lista de rutas

| Método | Ruta | Uso |
| --- | --- | --- |
| GET | `/api/status` | Estado general, escena, memoria, almacenamiento y OTA |
| GET | `/api/v3/settings` | Brillo, encendido y segundos del reloj |
| POST | `/api/v3/settings` | Cambiar brillo, encendido o segundos |
| POST | `/api/v3/notify` | Mostrar una notificación temporal |
| POST | `/api/display` | Cambiar la escena base |
| POST | `/api/stop` | Cancelar notificaciones y volver a la escena base |
| GET | `/api/variables` | Leer variables externas y del BMP280 |
| POST | `/api/variables` | Actualizar o borrar variables externas |
| GET | `/api/rotation` | Leer la secuencia y su estado |
| POST | `/api/rotation` | Configurar y guardar la secuencia |
| GET | `/api/animations` | Listar animaciones o descargar una |
| POST | `/api/animations?name=NOMBRE` | Cargar un archivo AWX mediante multipart |
| DELETE | `/api/animations?name=NOMBRE` | Borrar una animación |
| GET | `/api/sensors` | Estado y mediciones del BMP280 |
| POST | `/api/time` | Ajustar la hora mediante segundos Unix |
| POST | `/api/save` | Guardar brillo, segundos, variables y escena base |
| GET | `/api/network` | Consultar la conexión WiFi |
| POST | `/api/network` | Guardar y aplicar una red WiFi |
| POST | `/api/network/scan` | Iniciar un escaneo WiFi |
| GET | `/api/network/scan` | Consultar el resultado del escaneo |

## 3. Encendido, brillo y segundos

### Consultar ajustes

```bash
curl 'http://192.168.1.50/api/v3/settings'
```

Ejemplo de respuesta:

```json
{"Brightness":30,"on":true,"showSeconds":false}
```

### Cambiar brillo

Rango: **0 a 255**. El valor 0 apaga visualmente los LED; el ESP continúa funcionando y conectado.

```bash
curl -X POST 'http://192.168.1.50/api/v3/settings' \
  -H 'Content-Type: application/json' \
  -d '{"Brightness":30}'
```

Se aceptan `Brightness` y `brightness`. No se acepta el campo WLED `bri`.

### Apagar y encender

```json
{"on":false}
```

```json
{"on":true}
```

`on:true` recupera el último brillo no nulo si el brillo actual es 0. Para fijar un brillo concreto, envía `{"Brightness":30}`.

El reloj incluye siete marcas en su última fila: lunes a domingo, de izquierda a derecha. El día actual aparece blanco y los demás azules. Depende de la hora local sincronizada; si falta, no se destaca ningún día.

### Mostrar segundos

```json
{"showSeconds":true}
```

Para ocultarlos, usa `false`. Se pueden combinar ajustes:

```json
{"Brightness":30,"showSeconds":true}
```

Estos cambios se aplican inmediatamente. Usa `/api/save` si quieres conservarlos tras reiniciar.

## 4. Notificaciones: texto e iconos

**POST `/api/v3/notify`** muestra un mensaje temporal. Al terminar, se muestra el siguiente de la cola o se recupera la escena base, como la rotación.

### Texto simple

```bash
curl -X POST 'http://192.168.1.50/api/v3/notify' \
  -H 'Content-Type: application/json' \
  -d '{"data":"Hola","color":[0,255,0],"duration":5,"force":true}'
```

### Tus iconos con valores

Los nombres deben coincidir exactamente con los guardados en la pantalla:

```json
{"icon":"adquio","data":"Adquio","color":[255,0,255],"duration":5,"force":true}
```

```json
{"icon":"1027","data":650,"color":[0,255,0],"duration":5,"force":true}
```

```json
{"icon":"609","data":23.5,"color":[0,255,0],"duration":5,"force":true}
```

```json
{"icon":693,"data":"45%","color":[0,255,0],"duration":5,"force":true}
```

### Campos admitidos

| Campo | Tipo / rango | Comportamiento |
| --- | --- | --- |
| `data` o `text` | Valor simple; texto de hasta 160 bytes | Contenido; `data` tiene prioridad si aparecen ambos |
| `icon` o `animation` | Nombre o ID numérico no negativo | Animación ya guardada; `icon` tiene prioridad |
| `color` | `[r,g,b]` con componentes 0–255 o `"#RRGGBB"` | Color del texto; por defecto verde |
| `duration` | Entero 0–3600 | Duración en segundos; 0 u omitido usa finalización automática |
| `repeat` | Entero 1–20 | Repeticiones para la finalización automática; por defecto 1 |
| `scrollMs` | Entero 20–1000 | Milisegundos por paso del texto; por defecto 70, menor valor = más rápido |
| `force` | Booleano | `true` interrumpe la notificación actual; por defecto `false` |

Debe haber texto o una animación. Los iconos acompañados de texto deben tener un ancho máximo de 8 píxeles. Una animación sin texto puede ocupar toda la matriz.

Con `duration` mayor que 0, manda ese tiempo, aunque el texto no haya terminado de desplazarse. Con `duration:0`, un texto desplazable termina tras las pasadas indicadas por `repeat`; un contenido que no se desplaza dura 5 segundos por repetición. Hay un límite de una hora por notificación. Esta duración automática no se calcula a partir del ciclo de la animación.

Si hay una notificación activa y `force` es `false`, la nueva espera en cola. Caben **8 pendientes**, además de la activa. `force:true` sustituye la activa, pero conserva las pendientes.

Ejemplo de respuesta:

```json
{"ok":true,"queued":0,"warnings":[]}
```

`name` no identifica ni sustituye mensajes en esta implementación. `soundfile` y `moveIcon` no tienen efecto y generan advertencias en la respuesta. El icono conserva su posición y sus fotogramas se animan.

El texto usa una fuente ASCII; se simplifican los acentos españoles y otros caracteres no representables pueden aparecer como `?`.

### Cancelar las notificaciones

```bash
curl -X POST 'http://192.168.1.50/api/stop'
```

Vacía la cola y vuelve a la escena base. No cambia el brillo.

## 5. Variables y plantillas

**POST `/api/variables`** mezcla los valores enviados con los existentes:

```bash
curl -X POST 'http://192.168.1.50/api/variables' \
  -H 'Content-Type: application/json' \
  -d '{"Co2":650,"Temp":23.5,"Hum":45,"TVOC":120}'
```

Puedes usarlos en notificaciones y en los textos de iconos de la rotación:

```json
{"icon":"1027","data":"{Co2}","duration":5,"color":"#00FF00"}
```

```json
{"data":"VOC:{TVOC}","duration":5}
```

Los marcadores desconocidos quedan visibles. La sustitución no es recursiva y se limita a 160 bytes de texto resultante.

Consulta las variables:

```bash
curl 'http://192.168.1.50/api/variables'
```

Borra una variable enviando `null`:

```json
{"TVOC":null}
```

Límites: 16 variables externas; nombres de 1–20 caracteres con letras ASCII, números, guion o guion bajo; valores simples de hasta 64 bytes en su representación. No se admiten listas ni objetos como valores.

`bmpTemp` y `bmpPressure` son variables de solo lectura del sensor local, en °C y hPa. Si no hay lectura válida, la consulta devuelve `null` y las plantillas muestran `--`.

Actualizar variables no las guarda automáticamente en flash. Para conservarlas, usa `/api/save`.

## 6. Escena base

**POST `/api/display`** sustituye la escena base, cancela las notificaciones y vacía su cola. Acepta los campos de contenido de las notificaciones y el campo `mode`:

| `mode` | Resultado |
| --- | --- |
| `clock` | Reloj |
| `rotation` | Secuencia configurada |
| `custom` | Texto, icono o animación; valor por defecto |
| `off` | Pantalla en negro sin cambiar el valor del brillo |

Volver a la rotación:

```bash
curl -X POST 'http://192.168.1.50/api/display' \
  -H 'Content-Type: application/json' \
  -d '{"mode":"rotation"}'
```

Mostrar una animación continuamente:

```json
{"mode":"custom","animation":"pixel-mb"}
```

Mostrar un icono con texto:

```json
{"mode":"custom","icon":"1027","data":"{Co2}","color":"#00FF00"}
```

Con `duration` omitido o 0, la escena base permanece hasta otro cambio. Con duración positiva, al terminar pasa a la rotación si está habilitada o al reloj si no lo está. Elegir una escena no enciende los LED si el brillo está en 0.

En el editor, «Fijar como escena base» cambia a «Desfijar escena base» en rojo mientras la escena base sea personalizada y sin límite de tiempo. Desfijar vuelve a la rotación si está habilitada o al reloj si no lo está. Las notificaciones temporales no cambian ese estado.

## 7. Rotación: orden, duración y colores

### Leer

```bash
curl 'http://192.168.1.50/api/rotation'
```

La respuesta incluye `enabled`, `items` y el estado de ejecución: `active`, `index`, `page`, `animation` y `remainingSeconds`. El índice empieza en 0.

### Configurar

**POST `/api/rotation`** guarda la configuración en flash automáticamente. El orden de `items` es el orden de reproducción. Enviar `items` sustituye la lista completa.

```bash
curl -X POST 'http://192.168.1.50/api/rotation' \
  -H 'Content-Type: application/json' \
  -d '{
    "enabled":true,
    "items":[
      {"type":"clock","seconds":10,"color":"#00B4FF"},
      {"type":"animation","animation":"adquio","text":"Adquio","seconds":7,"color":"#FF00FF"},
      {"type":"temperature","seconds":5,"color":"#FF8020"},
      {"type":"pressure","seconds":5,"color":"#00C8FF"},
      {"type":"animation","animation":"pixel-mb","seconds":4}
    ]
  }'
```

De 1 a 16 elementos, cada uno con `seconds` entero entre 1 y 3600. Tipos válidos: `clock`, `temperature`, `pressure`, `animation`, `random`, `text`.

Las páginas de temperatura y presión leen el BMP280 local. No muestran las variables externas `Temp` o `Co2`.

En filas de tipo `animation`, `animation` es obligatorio. El campo opcional `text` admite hasta 64 bytes UTF-8 sin saltos de línea ni caracteres de control y requiere un icono de hasta 8×8. Puede incluir plantillas como `{Co2}`. El color afecta al texto, al reloj o a la lectura del sensor; no recolorea los píxeles de la animación.

Si omites `color`, los valores por defecto son: reloj `#00B4FF`, temperatura `#FF8020`, presión `#00C8FF` y texto de animación `#00FF00`.

El editor calcula una propuesta de duración al añadir una animación:

```text
segundos = máximo(1, mínimo(3600, redondear_hacia_arriba(frames × frameMs / 1000)))
```

En la API debes enviar `seconds` explícitamente. Puedes elegir otro valor; al terminar ese tiempo la rotación cambia de fila, aunque la animación no haya completado su ciclo.

Deshabilitar la rotación conservando la lista:

```json
{"enabled":false}
```

Volver a habilitarla conservando la lista:

```json
{"enabled":true}
```

Al habilitarla mediante esta ruta se selecciona como escena base. Si hay una notificación activa, esta continúa hasta terminar.

### Segundos y días en cada fila de reloj

Cada elemento `clock` admite `showSeconds` y `showWeekdays` booleanos e independientes:

```json
{"type":"clock","seconds":10,"color":"#00B4FF","showSeconds":true,"showWeekdays":false}
```

Estos campos solo se admiten en filas `clock` y se guardan automáticamente al aplicar la rotación. El editor muestra los dos checks en cada fila Hora. Las filas antiguas sin campos conservan los segundos heredados de la configuración general y los días activados; las nuevas filas del editor incluyen sus valores explícitos. El ajuste histórico `showSeconds` de `/api/v3/settings` permanece para el reloj manual y como valor de migración, pero no sobrescribe las opciones explícitas de las filas de rotación.

### Texto sin icono

Añade una fila como esta a `items` de la rotación:

```json
{"type":"text","text":"CO2: {Co2}","seconds":10,"color":"#00FF00"}
```

`text` es obligatorio y admite de 1 a 64 bytes UTF-8 sin caracteres de control. Se usan los 32 píxeles de ancho; los textos largos se desplazan. Los valores entre llaves se resuelven desde las variables del ESP. Se mantiene el límite general de 16 filas y 1–3600 segundos por fila. La actualización sustituye la lista `items`: incluye también las filas existentes que quieras conservar.

### Animación aleatoria

Añade una fila `{"type":"random","seconds":10}` en `items` y envía una lista `randomAnimations` con los nombres que puede elegir:

```json
{
  "enabled":true,
  "randomAnimations":["amongus","frog","yoshi"],
  "items":[
    {"type":"clock","seconds":10},
    {"type":"random","seconds":10}
  ]
}
```

Cada turno elige una de las marcadas al azar, sin repetir la anterior cuando hay varias. La duración la establece `seconds`. Si la lista está vacía, muestra la hora durante ese turno. La lista admite hasta 48 nombres únicos de animaciones guardadas. Omitir `randomAnimations` conserva la selección anterior; enviar `[]` la vacía. Se guarda junto con la rotación. En la web, «Usar en aleatorio» se guarda automáticamente al marcar o desmarcar. Un POST que contiene solo `randomAnimations` actualiza y guarda esa selección sin reiniciar la pantalla actual ni modificar la escena base. El orden y las duraciones se siguen confirmando con «Aplicar y guardar rotación».

## 8. Animaciones: listar, cargar, descargar y borrar

### Listar

```bash
curl 'http://192.168.1.50/api/animations'
```

Ejemplo de estructura, con datos ilustrativos:

```json
{"animations":[{"name":"mi-icono","width":8,"height":8,"frames":2,"frameMs":100}]}
```

Usa el `name` exacto, sin extensión, al enviar notificaciones o configurar la rotación.

### Importar JSON desde el editor

Abre `http://192.168.1.50/`, importa el JSON y guárdalo en el ESP. El editor lo convierte a AWX, con compresión RLE sin pérdida cuando ocupa menos. **La ruta de carga de la API no recibe directamente JSON de animaciones.**

Formato JSON normalizado; este ejemplo mínimo contiene un único píxel:

```json
{
  "width":1,
  "height":1,
  "frameMs":100,
  "frames":[["#FF0000"]]
}
```

Cada fotograma contiene `width × height` colores `#RRGGBB`, por filas, de izquierda a derecha y de arriba abajo. Dimensiones: ancho 1–32, alto 1–8; 1–120 fotogramas; intervalo constante de 20–5000 ms.

### Convertir JSON y cargar por API

Desde la carpeta del proyecto, con Python 3:

```bash
python3 tools/awx.py mi-animacion.json mi-animacion.awx --compact
```

Después:

```bash
curl -X POST 'http://192.168.1.50/api/animations?name=mi-animacion' \
  -F 'file=@mi-animacion.awx'
```

Envía un único archivo por petición. Deja que curl establezca la cabecera multipart y su separador; no uses `Content-Type: application/json` en esta carga.

Los nombres admiten 1–20 letras ASCII, números, `-` y `_`. Máximo 48 animaciones y 61 452 bytes por archivo; también debe quedar espacio libre para el archivo temporal. Cargar con un nombre existente sustituye ese archivo si pasa la validación.

### Descargar

El botón «Descargar» del editor convierte la animación guardada a un archivo `.json`, tanto si internamente está comprimida como si no. La descarga directa mediante esta ruta de la API devuelve AWX binario:

```bash
curl 'http://192.168.1.50/api/animations?name=mi-animacion' -o mi-animacion.awx
```

Puedes convertirlo a JSON:

```bash
python3 tools/awx.py mi-animacion.awx mi-animacion.json
```

El firmware admite AWX1 (RGB565 sin comprimir) y AWX2 (RGB565 con compresión RLE por fotograma y fotogramas crudos cuando conviene). Ambos mantienen los mismos colores RGB565. AWX usa RGB565: al convertir desde RGB de 24 bits se reduce la precisión del color.

### Borrar

```bash
curl -X DELETE 'http://192.168.1.50/api/animations?name=mi-animacion'
```

Primero desmárcala de la selección aleatoria y retírala de la rotación y de cualquier escena o notificación que la use. Si está en uso o en cola, la API devuelve 409.

## 9. Sensor BMP280 y estado general

```bash
curl 'http://192.168.1.50/api/sensors'
```

El objeto `bmp280` informa de `enabled`, `detected`, `valid`, `address`, `chipId`, `sdaPin`, `sclPin`, `temperatureC`, `pressureHpa`, `ageMs` y `error`.

La temperatura está en °C y la presión en hPa. Cuando no hay lectura válida, las mediciones y su antigüedad son `null`.

Estado completo:

```bash
curl 'http://192.168.1.50/api/status'
```

Campos útiles: `firmware`, `ip`, `port`, `wifiConnected`, `Brightness`, `showSeconds`, `notifying`, `queued`, `scene`, `baseScene`, `rotation`, `resolvedText`, `freeHeap`, `fsReady`, `fsUsed`, `fsTotal` y `ota`.

## 10. Ajustar la hora

**POST `/api/time`**, con segundos Unix, no milisegundos:

```js
axios.post('http://192.168.1.50/api/time', {
  epoch: Math.floor(Date.now() / 1000)
});
```

Rango admitido: 1577836800–4102444799. La zona horaria del firmware está configurada para España peninsular, con cambio estacional. Esta ruta ajusta el reloj, no configura el horario laboral ni modifica la zona horaria.

## 11. Guardar cambios tras reiniciar

```bash
curl -X POST 'http://192.168.1.50/api/save'
```

Guarda brillo, último brillo no nulo, opción de segundos, variables externas y escena base. No guarda notificaciones ni su cola. La rotación, la red WiFi y los archivos de animaciones se guardan por sus propias rutas.

Para cambios frecuentes de sensores no es necesario llamar a `/api/save` cada minuto; puedes mantener los valores en RAM y volver a enviarlos en la siguiente ejecución.

## 12. Configuración WiFi

Consultar:

```bash
curl 'http://192.168.1.50/api/network'
```

Iniciar escaneo:

```bash
curl -X POST 'http://192.168.1.50/api/network/scan'
```

Consultar el resultado repitiendo esta petición mientras devuelva 202:

```bash
curl 'http://192.168.1.50/api/network/scan'
```

Al terminar devuelve `scanning:false` y `networks`, con `ssid`, `rssi`, `channel` y `secure` para cada red seleccionada.

Guardar y aplicar una red:

```bash
curl -X POST 'http://192.168.1.50/api/network' \
  -H 'Content-Type: application/json' \
  -d '{"ssid":"MI_RED","password":"MI_CONTRASENA"}'
```

SSID de 1–32 bytes; contraseña WPA de 8–63 bytes, o cadena vacía para una red abierta. Devuelve 202 y comienza la conexión. La IP puede cambiar. Espera a que termine un escaneo antes de cambiar la red.

### IP fija o DHCP

En el editor, activa el interruptor «IP fija» dentro de WiFi y completa IP, puerta de enlace, máscara y DNS principal. DNS secundario es opcional. Pulsa «Aplicar IP fija» para guardar solo la dirección. Desactiva el interruptor y pulsa «Aplicar DHCP» para volver a la dirección automática. «Guardar y conectar» guarda conjuntamente SSID, contraseña y la configuración IP fija/DHCP seleccionada, también durante el primer arranque. El botón adicional de IP aplica solo el direccionamiento y conserva las credenciales guardadas. Elige una IP libre fuera del rango DHCP del router. La configuración persiste tras reiniciar y se aplica también en los reintentos de conexión.

```bash
curl -X POST 'http://IP_ACTUAL/api/network' \
  -H 'Content-Type: application/json' \
  -d '{"ssid":"MI_RED","staticIp":true,"address":"192.168.1.50","gateway":"192.168.1.1","subnet":"255.255.255.0","dns1":"192.168.1.1","dns2":"1.1.1.1"}'
```

Después abre `http://192.168.1.50/`. Para cambiar solo la IP puedes omitir `ssid` y `password`; el ESP conserva ambos. Para DHCP envía `{"staticIp":false}`. Si omites `password` conservando el SSID, se conserva la contraseña guardada. Para una red nueva, omitirla significa red abierta; enviarla explícitamente como `""` también configura red abierta. La interfaz conserva la contraseña al dejar el campo vacío con el mismo SSID.

`GET /api/network` añade `staticIp`, `address`, `gateway`, `subnet`, `dns1` y `dns2` (configuración guardada). `ip` sigue siendo la IP actual. La API valida IPv4, máscara contigua y direcciones de equipo/puerta de enlace distintas y válidas dentro de la misma subred. Devuelve 400 sin guardar si son inválidas. El hotspot permanece disponible al aplicar cambios. Las redes guardadas con versiones anteriores usan DHCP.

### Recuperación automática de red

Si no conecta al arrancar, o pierde el WiFi, tras 15 segundos activa `PIXEL-MB-xxxxxx`. Conéctate con `pixel-mb-local` y abre `http://192.168.4.1/`. Mientras tanto reintenta la red guardada cada 30 segundos sin cerrar el hotspot; los escaneos en curso posponen el reintento. Si recupera el router, el hotspot permanece disponible hasta reiniciar.

Al conectarse, la matriz muestra su IP durante 4 segundos, con desplazamiento si no cabe; después vuelve al contenido. También ocurre al reconectar. El brillo configurado se respeta. `/api/status` incluye `wifiIpNoticeActive`, `wifiIpNoticeText` y `wifiIpNoticeDurationMs` para consultar este aviso.

`/api/status` incluye `wifiReconnectAttempts` (reintentos explícitos desde el arranque), `wifiRetrySeconds`, `wifiFallbackSeconds`, `wifiConnected` y `apActive`. Sin credenciales guardadas se inicia directamente en modo hotspot.

## 13. Ejemplo de envío con Axios / Adquio

Ejemplo independiente, usando los nombres de tus iconos. No contiene `?.` ni object spread:

```js
const axios = require('axios');
const PIXEL_MB = 'http://192.168.1.50';

axios.post(PIXEL_MB + '/api/v3/notify', {
  name: 'TestNotification',
  force: true,
  icon: '1027',
  repeat: 1,
  duration: 5,
  data: 650,
  color: [0, 255, 0]
}, { timeout: 5000 }).catch(function (error) {
  const detalle =
    (error && error.response && error.response.data && error.response.data.error) ||
    (error && error.message) || String(error);
  logger.error('Pixel-MB: ' + detalle);
});
```

Puedes sustituir `650` por un valor obtenido con `devices.readFuture`. Espera a que se resuelva esa lectura antes de enviar el mensaje.

En tu automatización actual, el horario de lunes a viernes de 08:00 a 18:00 se comprueba en **Adquio**. Fuera de él se envía `{"Brightness":0}`; dentro, `{"Brightness":30}` y los mensajes. El firmware no almacena ese horario. El cambio depende de que la ejecución de cada minuto tenga lugar y alcance la pantalla, y usa la hora del entorno donde se ejecuta el script.

## 14. Respuestas y errores

Muchas operaciones devuelven:

```json
{"ok":true}
```

Los errores de la API suelen tener esta estructura:

```json
{"error":"Descripción del problema"}
```

| Código | Significado habitual |
| --- | --- |
| 200 | Operación completada |
| 202 | Escaneo o conexión WiFi iniciados o en curso |
| 400 | JSON o parámetros inválidos; icono inexistente; archivo AWX inválido |
| 401 | Falta la clave correcta en `X-API-Key` |
| 404 | Ruta o animación no encontrada |
| 409 | Animación en uso o conflicto con el estado actual |
| 413 | Cuerpo JSON ausente o demasiado grande |
| 429 | Cola de notificaciones llena |
| 500 | Error interno o de almacenamiento |
| 503 | Almacenamiento no disponible o fallo del escaneo WiFi |
| 507 | No se pudo guardar un archivo o configuración |

El tamaño general máximo del cuerpo JSON es 2048 bytes. La configuración de rotación admite hasta 8192 bytes. La carga multipart de AWX tiene su límite independiente.

Si aparece «Icono/animación local no encontrado o inválido», consulta `/api/animations` y revisa el nombre exacto. Si la pantalla sigue negra, consulta el brillo y el modo en `/api/status`.

## 15. Compatibilidad y actualización OTA

Esta es una implementación independiente con compatibilidad parcial de mensajes AWTRIX v3. No implementa toda la API oficial ni los comandos WLED `/json`, `ps` o `bri`. Tampoco existe `/api/v3/draw` en esta versión.

La actualización de firmware se realiza mediante `/update`, con autenticación HTTP Digest y usuario `admin`, o con las herramientas OTA del proyecto. Sus credenciales son distintas de `X-API-Key`. Para PlatformIO, desde la carpeta del proyecto y con `ota.local.json` ya configurado:

```bash
pio run -e d1_mini_ota -t upload
```

La carga OTA de firmware conserva las animaciones y configuraciones de LittleFS. No es necesario cargar el sistema de archivos para actualizar el firmware.

## Sensor ESP32 BMP280 / BME280

I²C SDA GPIO21, SCL GPIO22; matriz GPIO32. `/api/sensors` mantiene el objeto `bmp280` y añade `model` (`BMP280` o `BME280`) y `humidityPercent` (nulo si no hay BME280 válido). `/api/variables` y los textos admiten la variable de solo lectura `bmpHumidity`, además de `bmpTemp` y `bmpPressure`. Usa `Humedad: {bmpHumidity}%` en una fila Texto.

### Firmware y copias desde la web

En «Firmware y copias de seguridad», abre el actualizador y carga `firmware.bin` de la plataforma correspondiente. Por defecto, el actualizador web no pide usuario ni contraseña. Mientras la protección esté desactivada, cualquier equipo con acceso a la web de la placa puede actualizarla. La actualización conserva WiFi, configuración y animaciones.

«Descargar copia de seguridad» genera un JSON con las animaciones (AWX en base64 y CRC32), la rotación, las variables, el brillo y la escena base. No contiene WiFi, contraseñas ni una imagen del firmware. Puede restaurarse en ESP8266 o ESP32: reemplaza ajustes y animaciones con el mismo nombre y conserva otras animaciones. No es una restauración atómica: si se interrumpe, se informa del error y puede reintentarse. El navegador valida la copia antes de escribir y verifica cada animación enviada.

`POST /api/variables?replace=true` sustituye las variables de usuario en lugar de combinarlas con las existentes; conserva la validación habitual.

El actualizador HTTP `/update` permite cargar el `.bin` sin usuario ni contraseña. La contraseña del archivo privado `ota.local.json` se conserva únicamente para la OTA clásica de Arduino, no para el actualizador web.

### Protección web opcional

En «Acceso a la web» puedes activar un usuario y una contraseña para toda la interfaz, API y actualizador. **Deshabilitada por defecto**: mientras esté apagada puedes cargar el `.bin` directamente. Se usa autenticación HTTP Digest y los ajustes se guardan en la placa. La contraseña no se devuelve por API ni se incluye en las copias. Si la olvidas, debes borrar la configuración por USB. `GET/POST /api/web-access` consulta o cambia `enabled`, `username` y `password`; dejar la contraseña vacía conserva la anterior. Una vez activada, las llamadas API también requieren autenticación Digest. La OTA clásica conserva su clave independiente.

Si activas la protección web y quieres usar el cargador HTTP de PlatformIO, añade `webUsername` y `webPassword` al archivo privado `ota.local.json`. Sin protección web no hacen falta estos campos.
