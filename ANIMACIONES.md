# Animaciones AWTRIX y archivos AWTRIXER

El paquete incluido contiene cuatro animaciones de 32×8. Se instalan solo si faltan al actualizar a 1.0.7, sin cargar LittleFS ni sustituir dibujos existentes. Después puedes borrarlas si no están en la rotación; no se reinstalan en cada arranque.

| Nombre local | Origen y adaptación | Fotogramas / intervalo |
|---|---|---|
| `aw1_fire` | [FireApp.cpp de AWTRIX v1](https://github.com/awtrix/awtrix1.0/blob/master/lib/Applications/FireApp.cpp), Blueforcer. Difusión original, semilla reproducible, secuencia finita y canales limitados a 255. | 60 / 50 ms |
| `aw1_life` | [GolApp.cpp de AWTRIX v1](https://github.com/awtrix/awtrix1.0/blob/master/lib/Applications/GolApp.cpp), Blueforcer. Reglas sobre toro y colores originales; 85 células iniciales deterministas y 48 generaciones. | 48 / 500 ms |
| `aw2_matrix` | [Matrix.bas de AWTRIX v2](https://github.com/awtrix/AWTRIX2.0-Apps/blob/master/Matrix/Matrix.bas), Stephan Mühl / Blueforcer. Los 35 fotogramas RGB565 originales y su intervalo, convertidos a AWX1. | 35 / 100 ms |
| `aw2_eyes` | [LookingEyes.bas de AWTRIX v2](https://github.com/awtrix/AWTRIX2.0-Apps/blob/master/LookingEyes/LookingEyes.bas), Stephan Mühl / Blueforcer. Sprites originales; mirada y parpadeo adaptados a una secuencia finita. | 64 / 90 ms |

Las adaptaciones v1 usan **CC BY 4.0**; v2 usa **BSD-2-Clause**. Textos completos en `licenses/AWTRIX-v1.txt` y `licenses/AWTRIX-v2.txt`. Estas licencias se aplican a las piezas procedentes de cada repositorio. Se conservan sus fuentes en `tools/awtrix_sources/`. Las adaptaciones fueron realizadas para Pixel-MB en octubre de 2026. El script `python3 tools/make_awtrix_pack.py` reproduce los cuatro archivos AWX y sus JSON editables.

También se ha localizado el [archivo histórico de animaciones](https://awtrix.blueforcer.de/animations/), con KnightRider, Starfield, PacMan y otras. La [app Animations de v2](https://github.com/awtrix/AWTRIX2.0-Apps/blob/master/Animations/Animations.bas) descargaba animaciones de la nube. El archivo histórico no proporcionó una descarga verificable en esta sesión (certificado HTTPS y errores HTTP); su índice tampoco especifica una licencia por archivo. No se ha presentado su contenido como parte del paquete incorporado.

## Importar animaciones propias

El repositorio no incluye logos personales ni el catálogo descargado de AWTRIX Hub. Puedes importar tus propios archivos; las pruebas usan un dibujo mínimo creado para este proyecto.

El editor web acepta **.awtrixer**, **.piskel**, **.json** y **.awx**. Importa el archivo, cambia el nombre si quieres y pulsa Guardar en ESP32. Luego elige la animación en Rotación de pantallas o pulsa Añadir a rotación en su tarjeta. Finalmente aplica y guarda la secuencia.

El importador soporta modelVersion 2, hasta 16 capas con opacidad, PNG incrustado con layout de chunks, 120 fotogramas, ancho máximo 32 y alto máximo 8. Compone transparencias sobre negro y convierte RGB a RGB565 al guardar. El formato AWX usa un intervalo constante; FPS admitidos: 0,2–50. Los proyectos incompatibles muestran un error, sin modificar el dibujo actual.

Para convertir en el ordenador, con Pillow instalado:

```sh
python3 tools/import_awtrixer.py mi-logo.awtrixer mi-logo.awx
```

Los efectos originalmente generados al vuelo se reproducen aquí como secuencias finitas en bucle. No ejecutan apps B4J, juegos interactivos ni necesitan servidor.

## Logo Pixel-MB

`pixel-mb` es la animación propia de arranque, generada desde `include/BootLogo.h`: 32×8, 80 fotogramas, 40 ms por fotograma (3,2 s). Se incluye en `data/animations/pixel-mb.awx` y `examples/pixel-mb.json`. `python3 tools/make_boot_animation.py` reproduce ambos archivos usando el dibujado real del firmware. Se instala una sola vez y puede elegirse en la lista de rotación.
