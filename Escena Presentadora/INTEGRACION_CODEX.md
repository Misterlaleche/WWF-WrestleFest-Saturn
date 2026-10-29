# Integración de Escena Presentadora — contrato para Codex

Referencia: **WP02 R02**, aceptada por el usuario el **2026-10-29**. Las
fuentes publicadas conservan los bytes del paquete aprobado. Este documento
explica cómo preparar una futura integración; la publicación actual conserva
el programa independiente y su comportamiento.

## Entrada y control de la escena


El `main.c` publicado es el programa de demostración independiente: elige un
reparto aleatorio, configura vídeo, carga audio y gestiona START. La integración
en el juego debe entregar a `wp02_init()` los **cuatro IDs internos originales
y el estado RNG real de entrada** que ya controla el juego. Conserva su orden
de equipo y el tratamiento de Demolition `[10,11]`; los IDs no usan la
permutación de una interfaz antigua.

La escena avanza a `7000000 / (448 × 272)` actualizaciones por segundo,
aproximadamente **57,444852941 Hz**. R02 toma el tiempo en el límite de
pantalla, envía el dibujo a VDP1 y después realiza el servicio de audio.
Conserva ese orden y todos los eventos efectivos de sonido. Al terminar hay
una última llamada que borra la presentadora sin incrementar `ticks`; esa
transición forma parte de la escena.

Si se conserva este backend, precarga las muestras y la fuente BGM antes de
fijar el origen temporal. Envía los `effective_commands` de cada actualización
a `wp02_audio_emit()` en el mismo orden. Llama una vez a
`wp02_audio_vblank(NULL)` por VBlank y a `wp02_audio_service()` desde el bucle
principal después de enviar el dibujo; no decodifiques desde la interrupción.
La comprobación temprana de `wp02_audio_state()` conserva el rechazo de un
error ya registrado antes de avanzar de nuevo la escena.

### Audio, reloj y continuidad

Esta versión conserva **poneSound/M68K con transporte PCM**: voces/reacciones
PCM16 a 8 kHz y una fuente BGM a 44,1 kHz comprimida sin pérdida. La música
es una captura de la salida original YM2151 más percusión OKI. La adaptación
al backend de síntesis SCSP nativa de WP01 es una tarea de integración
separada; estos módulos no realizan esa conversión.

La fuente BGM contiene **29 segundos finitos**. En la traza arcade observada,
`310E` se había emitido **7,215979833 segundos** antes de la entrada `3132`;
no aparece un nuevo `310E` durante la presentación. El fragmento publicado
empieza en esa fase observada. Al enlazar las escenas del juego, conserva el
propietario y la progresión reales de la música; ese valor medido no debe
convertirse en un reinicio o desplazamiento universal para cualquier flujo.
La procedencia está en
[confirmed_background_report.json](evidence/r01_reference/original_audio/confirmed_background_report.json)
y [WP02BGM.json](source/research_audio/integration/cd/WP02BGM.json).

El módulo controla el **FRT del SH-2 maestro**, sus interrupciones y los
servicios de audio por VBlank. `wp02_audio_init()` recarga el controlador
M68K. El driver elige los canales SCSP físicos y el SH-2 controla el selector
de monitor `0x25B00408`. La integración debe asignar un único propietario de
esos recursos y coordinar la transición con el audio global antes de
inicializarlos. El reloj está calibrado para NTSC, modo de 320 píxeles.

`wp02_audio_scene_done()` detiene la BGM de esta demostración y permite
terminar las voces/efectos; `wp02_audio_stop()` silencia y reinicia el módulo.
La continuidad hacia el combate necesita una transición explícita del juego.

### Memoria y vídeo

| Recurso | Reserva de esta demostración |
| --- | --- |
| Fuente BGM comprimida en LWRAM | 1 MiB desde `0x00200000` |
| Fuente BGM adicional en HWRAM | 720.896 bytes estáticos |
| Fin enlazado de la versión aceptada | `0x060F0334`; después queda el heap de comandos |
| Texturas VDP1 | 381.056 bytes, más 256 comandos y alineación |
| Búfer de música en RAM de sonido | `0x6C800..0x7C7FF`, más palabra de guarda |

`main.c` configura VDP1, VDP2, CRAM, prioridades, callbacks de VBlank y
periféricos para la escena completa. Reserva y concilia estas áreas con el
mapa del juego antes de enlazar el módulo; el demo no deja un presupuesto
general de memoria para el resto del runtime.

## Evidencia y límites

La entrega registra 1.636 imágenes de referencia exactas, las 169 transiciones
visuales en orden y tres escenas completas con cero refrescos perdidos y cero
fallos BGM en el emulador. La revisión R02 corrige el presupuesto de reposición
tras un servicio retrasado y añade un registro visible de primer error.

[SERVICE-TIMING-REVIEW.md](evidence/audio_r02/SERVICE-TIMING-REVIEW.md) documenta
el caso corregido y la ventana heredada de una sola copia demasiado cercana
al cambio de mitad. Las pruebas de servicio a 50 Hz no convierten esta escena
NTSC en una versión PAL. La revisión de integración está en
[INTEGRATION_REVIEW.md](evidence/INTEGRATION_REVIEW.md).

## Archivos que deben mantenerse coherentes

`source/scene/build.py` refresca los módulos recuperados desde
`source/research_rom`, `source/research_render/generated` y
`source/research_audio/integration` cuando esas carpetas existen. Conserva
ese origen de cada módulo al hacer una futura revisión: una edición aislada
del duplicado en `scene/src` puede perderse en el build. `main.c` y
`wp02_diag.h` pertenecen directamente a `scene/src`.

El recurso musical tiene un final real; el anillo de 32.768 muestras es el
transporte y no autoriza a repetir el contenido de 29 segundos. El diagnóstico
congela el primer error antes del reset del audio. Al adaptar la gestión de
errores al juego, conserva ese orden para que el motivo siga disponible.

Para verificar una revisión, usa el recorrido documentado en el README:
restauración comprobada, 16 casos host y build SH-2 en una ruta sin espacios.
La variante `--reference` fija los cuatro IDs `[0,6,9,1]` y la semilla
`0xCEB7C58E`; la selección aleatoria/START es el programa de demostración.
Las capturas físicas y de emulador deben quedar identificadas por separado.
