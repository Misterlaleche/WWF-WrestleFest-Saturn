# WP02 R02 — presentación TAG para Saturn

Revisión independiente de la escena de presentación en el ring. El disco
elige cuatro luchadores del plantel TAG normal, conserva la pareja Demolition
y ejecuta las voces, animaciones y salida de la presentadora recuperadas de
la ROM. La presentación acaba cuando la presentadora desaparece del ring.

## Probar en la consola

Conserva juntos **disc/WP02-TAG-R02.cue** y **disc/WP02-TAG-R02.bin**, con
sus nombres. Carga el CUE con tu método habitual de ejecución de homebrew.
La imagen tiene una pista de datos MODE1/2352 y está preparada para Saturn
NTSC de 60 Hz. No requiere cartucho de expansión.

La pantalla permanece negra durante la carga de recursos y la escena arranca
automáticamente. **START** vuelve a cargar las voces necesarias y repite la
presentación con otro reparto. Al terminar, el ring queda visible, se detiene
la música de esta presentación y las últimas voces/efectos terminan de forma
natural. Esta prueba no continúa hacia el combate jugable.

El MP4 entregado junto al ZIP procede de la ejecución real de este mismo
BIN/CUE en Mednafen. La Saturn ejecuta código SH-2, dibuja con VDP1/VDP2 y
reproduce el audio mediante SCSP; el disco no reproduce un vídeo de la escena.
El MP4 contiene las dos presentaciones completas. Se han recortado la carga
inicial y la espera con el ring ya terminado entre ambas; se conserva el
reinicio nativo y la velocidad de todas las imágenes. Los intervalos exactos
del original están en `audit/release-media-r02.json`.

## Corrección de R02

La grabación física recibida muestra una parada con fondo rojo aproximadamente
0,77 segundos después de comenzar el audio. Ese fondo corresponde al camino
de error de R01. La grabación no contiene un código que permita identificar
por sí solo la causa exacta en la consola.

Se encontró un defecto reproducible en el transporte de música: con una
llamada de servicio cada dos refrescos NTSC, R01 prepara once de los dieciséis
bloques necesarios y falla en el primer retorno del búfer, a 768.896 µs en la
simulación. La prueba controlada en el emulador también activa el error de
reposición incompleta. R01 sin esa limitación artificial no reprodujo la
parada física, incluso con caché completa del SH-2.

R02 incorpora estos cambios:

- Permite reponer dos bloques de audio tras una llamada retrasada. En la
  última zona de cada mitad mantiene un solo bloque para limitar el trabajo
  cerca del cambio de mitad. Las llamadas normales conservan su presupuesto.
- Envía primero el dibujo a VDP1 y aprovecha ese tiempo para decodificar
  audio. El instante de la escena sigue tomándose en el límite de pantalla.
- Conserva un registro del primer error y muestra cuatro filas hexadecimales
  legibles si la ejecución se detiene.

No cambiaron el controlador de la escena, las tablas de ROM, los gráficos,
los nombres, las muestras PCM/BGM, el códec ni el controlador M68K de sonido.
En `scene/src` cambian `main.c`, `wp02_bgm.c` y `wp02_bgm.h`; se añade
`wp02_diag.h`. El constructor identifica la nueva revisión como R02.

## Verificación de esta revisión

Las capturas se realizaron con **Mednafen 1.32.1**, Saturn NTSC, caché completa
de instrucciones y datos del SH-2 y cartucho de expansión desactivado. La
integridad del BIN se comprueba antes y después de cada captura.

| Comprobación | Resultado |
| --- | --- |
| Referencia fija, 1.636 imágenes de pantalla hasta la salida | Coincidencia exacta RGB con los originales esperados |
| 169 cambios visuales | Orden completo; ninguno omitido ni añadido |
| Cadencia original | 57,444852941 actualizaciones/s |
| Cadencia medida en las transiciones | 57,444900989 actualizaciones/s |
| Duración visible de la salida | 3,309569 s; referencia 3,307520 s |
| Refrescos perdidos en las tres escenas medidas | 0 |
| Fallos de reposición BGM en las tres escenas | 0 |
| Mayor servicio BGM observado | 4.862 µs |
| Reconstrucción desde `source/scene` aislado | Ejecutable idéntico al del disco |

Todas las duraciones de las poses y de los tramos comprobados quedan dentro
de un refresco de salida más dos unidades de tiempo del contenedor. Ese
margen expresa la presentación del reloj arcade sobre el refresco de Saturn;
no se modifica la velocidad del vídeo para obtener el resultado.

El disco aleatorio se comprobó antes y después de START:

| Ejecución | Reparto | Actualizaciones | Duración hasta completar |
| --- | --- | ---: | ---: |
| Primera | Demolition Smash + Crush / Jake Roberts + Big Boss Man | 1.226 | 21,345196 s |
| Tras START | Earthquake + Ted DiBiase / Hulk Hogan + Mr. Perfect | 1.505 | 26,209277 s |
| Referencia fija | Hulk Hogan + Sgt. Slaughter / Ted DiBiase + Ultimate Warrior | 1.569 | 27,329191 s |

Las pruebas de audio comparan las **1.278.900 muestras** originales en
servicios simulados de 59,826 Hz, 29,913 Hz, ritmo variable, 45 Hz y 50 Hz.
No se realizan lecturas de CD durante la presentación. El PCM de la captura
de referencia sin pérdida es idéntico al fragmento correspondiente de la
grabación directa del emulador.

En el audio realmente capturado se detectan las **13 intervenciones originales**
de la referencia. Las 54 ventanas de comparación de música mantienen **cero
muestras de desfase** a través de 36 vueltas completas del búfer. No se detectan
recortes de amplitud ni huecos de 10 ms. La música termina 5,533 ms después
de la imagen en que desaparece la presentadora. La metodología, las medidas
y sus límites están en `audit/audio_r02/FINAL-CAPTURED-AUDIO-AUDIT.md`.

El controlador y los recursos conservan los resultados de comparación contra
la ROM/MAME de R01. Esta revisión vuelve a comprobar la salida visual real;
los resultados históricos están identificados como referencia de R01.

## Si aparece una pantalla de error

Fotografía las cuatro filas completas. El primer carácter de cada fila es
su número, del 1 al 4; los ocho caracteres restantes son datos hexadecimales.

| Fila | Ocho dígitos de datos |
| --- | --- |
| 1 | Código de error (2), motivo BGM (2), registro de posición SCSP (4) |
| 2 | Bytes preparados de la mitad incompleta (4), bloques repuestos (4) |
| 3 | Imágenes enviadas a VDP1 (4), refrescos perdidos (4) |
| 4 | Mayor tiempo de servicio BGM, en microsegundos (8) |

El código `0E` indica el error de servicio de audio. Sus motivos BGM son:
`01` carga, `02` decodificación, `03` intervalo excesivo, `04` espera del
canal, `05` canal perdido, `06` canal inválido, `07` canal cambiado,
`08` posición inválida, `09` reposición incompleta y `0A` fin de fuente.

`audit/diagnostic-injection-screen.png` es una prueba provocada a propósito
para comprobar la lectura del diagnóstico. **El disco entregado no contiene
la limitación artificial usada en esa prueba.**

## Contenido y reconstrucción

- `disc/`: BIN/CUE aleatorio entregado y comprobación de todos sus sectores.
- `source/scene/`: fuentes SH-2, recursos de CD y constructor del disco.
- `source/research_rom/`: controlador recuperado y pruebas contra ROM/MAME.
- `source/research_render/`: extracción, composición, referencias y verificador.
- `source/research_audio/integration/`: audio, códec, recursos y pruebas originales.
- `source/verification/r02_audio/`: pruebas R02 portátiles y copia de las dos
  fuentes R01 necesarias para demostrar la regresión.
- `audit/`: mediciones R02, revisión del código, captura de referencia sin
  pérdida y procedencia de los recursos.
- `SHA256SUMS.txt`: hashes del contenido final del paquete.

Para reconstruir hacen falta Python 3, el SDK Yaul/sh2eb-elf suministrado
(GCC 13.2) y genisoimage. Configura `YAUL_INSTALL_ROOT` con la ruta absoluta
del SDK. Si genisoimage no está en PATH, configura `WP02_MKISOFS`.

```sh
python source/scene/build.py
python source/tools/iso_to_bin.py source/scene/build/WP02-TAG-R02.iso rebuilt-disc
python source/verification/r02_audio/run_tests.py
```

`build.py --reference` genera la variante fija [0,6,9,1], semilla
`0xCEB7C58E`, para la comparación de píxeles. El disco de `disc/` usa el
selector aleatorio. Las pruebas de PC necesitan un compilador C; el SDK
SH-2 es opcional en el conjunto de regresión de audio.

`source/verification/capture_mednafen.py` admite `--mednafen`, `--xdotool`,
`--home` y `--display` para repetir la captura en Linux/X11. Requiere la
BIOS propia en el directorio elegido. No se incluyen BIOS ni la ROM completa.

Para repetir la comprobación de audio sobre la referencia sin pérdida incluida
(requiere ffmpeg, NumPy y SciPy), desde la raíz del paquete:

```sh
ffmpeg -i audit/reference-scene-lossless.mov -map 0:a:0 -c:a pcm_s16le reference.wav
python source/verification/audit_reference_audio.py reference.wav --label local_audio_audit --scene-origin 0.143379 --scene-end 27.472585 --calibration audit/audio_r02/r01_same_method_calibration.json
```

## Estado de la comprobación física

**R02 está comprobada en emulador; su resultado en tu Saturn está pendiente
de la nueva prueba con este BIN/CUE.** La coincidencia temporal del defecto
de reposición con la parada recibida respalda la corrección, pero no prueba
por sí sola la causa física. Las mediciones del emulador no son una medición
del rendimiento de la consola.

La prueba de duración también conserva explícitamente una ventana heredada
de R01 si una única copia empieza demasiado cerca del cambio de mitad; no
se afirma haber eliminado cualquier posible retraso del sistema. Los límites
y el caso concreto se documentan en `audit/audio_r02/SERVICE-TIMING-REVIEW.md`.
La temporización de esta escena corresponde a NTSC de 60 Hz; las pruebas de
servicio de audio a 50 Hz no convierten esta entrega en una versión PAL.

Los **39 archivos originales** y los **398 archivos verificados del paquete
R01** conservan sus hashes. No se ha modificado ningún repositorio ni trabajo
anterior. Toda esta revisión se ha preparado en un directorio independiente.
