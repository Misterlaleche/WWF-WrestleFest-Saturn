# RUNTIME-18 — capas del ring y fluidez desde a680f7a

Checkpoint local de 23 de octubre de 2026, rama `runtime18/direct-combat`.
El commit consolidado consta en CHECKPOINT.txt. Arranque directo con mando,
sin menús ni audio. No se ha vuelto al frontend ni cambiado decisiones,
animaciones, entradas, daño, reloj, RNG o finales del combate.

## Composición demostrada

El entry anterior imponía prioridad4 a las ocho clases de sprites. Eso
colocaba luchadores y cuerdas/postes construidos con sprites encima de ambas
capas del ring, contradiciendo el modo7C seleccionado por la ROM.

| Modo original | Orden de fondo a frente |
|---|---|
| 7C | FG_WORLD, sprites, BG_WORLD |
| 7B | FG_WORLD, BG_WORLD, sprites |
| 78 | BG_WORLD, FG_WORLD, sprites |

En7C las cuerdas frontales de BG_WORLD pueden tapar partes del luchador:
ese solapamiento es correcto. La corrección evita que los sprites de las
cuerdas laterales pasen por encima de las cuerdas frontales. No se ha
inventado una máscara por altura ni cambiado el orden de records.

Evidencia: tabla original `$26E9E` de la ROM68000 SHA1
`a726c46f350723ea73158bc78055d17d418b14e2`, contrato WP26
`tables/layer-priority-modes.csv`, y
[MAME, composición de WrestleFest](https://github.com/mamedev/mame/blob/master/src/mame/technos/ddragon3_v.cpp#L252-L289).
El test ejecuta la rama Saturn mediante el doble Yaul y comprueba píxeles
solapados de7C/7B/78, las ocho clases y la transparencia del pen0.
Falló con la configuración anterior y pasa con la corrección.

## Cuello de botella y medida SH-2

Antes se expandían en CPU los tiles del ring a dos bitmaps. El profiler
separado mide fases reales con FRT/128 y los VBlanks del emulador, manteniendo
la partida controlada y sus ACK. Se comparan exactamente90 frames, de30 a120.
El disco normal y el replay completo no incluyen el profiler ni su parada.

| Misma secuencia en Mednafen1.32.1 | a680f7a | Final |
|---|---:|---:|
| VBlanks /90 frames | 371 | 159 |
| VBlanks por frame | 4,122 | 1.767 |
| Mapa/world, ms por frame aprox. | 8,24 | 0,61 |
| Actualización del ring, ms por frame aprox. | 25,74 | 1,10 |
| Lecturas CD en esos90 frames | 0 | 0 |

La secuencia tarda un57,1% menos en VBlanks: **2,33 veces más rápida**.
No es una medición de fps del ordenador ni acredita todavía velocidad arcade,
60fps o timing de Saturn físico. Las fases cronometradas se solapan: runtime
incluye present y present incluye las lecturas; no deben sumarse sus tiempos.

El ring usa ahora caracteres VDP2 nativos de16x16, cuatro cuadrantes8x8,
nombres de patrón de una palabra y mapas512x512 con scroll original. Se
suben una vez los803 caracteres originales; después cambian sólo las celdas
necesarias. Se conservan flips, paletas, pen0, wrap y overlays animados.
Los bancos0/2 contienen caracteres y1/3 los nombres; CRAM mantiene sus ventanas.

Los tests comparan todos los nibbles originales y los píxeles visibles frente
al lector escalar en scrolls grandes/negativos, dirty/restore y todas las
orientaciones. La captura completa del frame120 final es **idéntica byte a
byte en RGB** a la referencia bitmap con las prioridades ya corregidas.
La prueba no establece igualdad universal con el framebuffer arcade.

## Precarga y lecturas durante el combate

Las trazas completas usan3993 tiles distintos y171 páginas ROM de1KiB.
HOTSPR.BIN copia esos3993 tiles sin cambios desde SPRITES.PAK. La partición
LWRAM es: ring00201000–0021D0E6, ROM00220000–00260000 y
sprites00260000–00300000. La caché ROM dispone de256 páginas y la de sprites
de5120 tiles, con1127 huecos disponibles tras la precarga.

La precarga ROM dispersa agrupa páginas consecutivas y conserva sus offsets
originales. Los IDs son pistas de caché, no sustituciones de contenido.
Cualquier dirección o sprite desconocido mantiene su lectura real bajo demanda;
no se promete cero lecturas para entradas o modos distintos.

Ambas partidas completas HOST y ASan/UBSan terminan con **ROM=0 y SPRITES=0
lecturas de archivo tras el arranque**. Antes, la secuencia sin entrada con
el mismo contador de archivo provocaba193 lecturas ROM y12322 de sprites.
Se ha reforzado el fixture con páginas diferentes, incluidos los bits altos
del número de página: una mutación del offset original hace fallar el test.

## Pruebas y ejecución

- 48 regresiones CORE HOST y48 ASan/UBSan, sin fallos.
- `make test`, prioridad Saturn,803 caracteres/celdas, IDs, scanlines,
  scroll cache, ROM full/sparse, sprites residentes, DMA Yaul y CDFS.
- Sanitizadores específicos de celdas nativas, IDs y prioridad hardware.
- Partidas completas HOST y ASan/UBSan:4391 sin entrada y14409 controlada;
  cámara coherente en cada presentación y lecturas CD cero en ambas.
- Pipeline ROM original R17; cámara2304 casos; world400 casos, D0–D7,
  animación/colas y transporte de mapas contra instrucciones originales.
- El antiguo oráculo world con Unicorn abortaba en su primer caso26B68
  en este entorno. Se cambió únicamente el motor del test a Musashi68000,
  ya usado por cámara; los400 fixtures y las comparaciones se mantienen.
- Revisión fresca sin hallazgos críticos/importantes. La debilidad menor
  del fixture ROM se corrigió y se comprobó con mutación y ejecución correcta.
- La prueba de exportación descubrió que faltaba la regla para crear `build/`
  en los tests R18. La compilación paralela desde una carpeta vacía falló antes
  de añadirla; después completa4391 y pasa los tests de celdas y caché ROM.
- Enlace SH-2 limpio de símbolos pendientes; validación de sectores, EDC/ECC,
  IP, ISO9660 y payloads para los discos normal y sin entrada.

El replay final SH-2 llega en Mednafen a **4391**, resultado8001,
cuenta0300 y reloj0021, sin owner/dependencia fatal. El observador sólo lee
memoria y verifica4096 bytes del ejecutable exacto. El disco normal se observa
en movimiento y también alcanza4391 sin entrada ni bloqueos.

## Discos y reproducción

Normal con mando: `validation/results/runtime18/wrestlefest-runtime18.cue`
y `.bin`. Replay separado: `validation/results/runtime18/replay-noinput/`.
Abrir o grabar el **CUE**, conservando su BIN al lado. Una pista MODE1/2352.

- Normal A.BIN: `0cc7ce19a2d2de10d4ebccdc3682567829fdc353de85a5e649dddd799c4ee095`.
- Normal BIN: `be21487ea52dcad085105223e85a31a3536bdc2684d78d4a1117eb41203e1c92`.
- Replay A.BIN: `5ae88e1bbf64bb0a9f9ede5f107508be6c9e4d49ba2d316d4e23415e18d64f53`.
- Replay BIN: `960f45e8d3b33d3b7fc182b327f237f05235e4c634efd61c08f8bcd509a0231b`.
- CUE de ambos: `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`.
- HOTSPR.BIN: `7d6d9bb76fa71ce1844b21d611326ea76f30e6f0732e1819d4bc6d233bd36d35`.

Desde la raíz, con los assets locales y los requisitos de ROM instalados:

```sh
make -C src/runtime test test-runtime18-ring-priority test-runtime18-ring-cells test-runtime18-ring-ids test-runtime18-ring-scroll-cache test-runtime18-rom-cache test-runtime18-hot-sprites test-runtime18-yaul-dma
make -C src/runtime test-runtime18-ring-native-asan
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match
make -C src/runtime test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
python3 src/runtime/tools/verify_r18_world_rom.py --rom /ruta/wwfwfest.zip
make -C src/runtime ROM_PYTHON=/ruta/venv/bin/python test-runtime18-camera-rom
DOCKER=/ruta/docker sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_REPLAY_TEST=noinput DOCKER=/ruta/docker sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 DOCKER=/ruta/docker sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

La última orden genera un disco exclusivamente diagnóstico en
`profile-controlled/`. Tras ejecutarlo, usar `observe-mednafen-runtime18.py`
con su PID, A.BIN y mapa exactos y `--profile`, y aplicar
`check-runtime18-profile.py` al JSON: exige90 ACK y como máximo2 VBlanks/frame.
El profiler de la línea base queda conservado junto a su ejecutable y contadores.

El ZIP incluye fuente consolidada, cuatro assets canónicos, ROM de referencia,
discos, resultados y hashes. Se reconstruye A.BIN desde la fuente exportada y
se compara con el ejecutable normal probado. SDK/Yaul, Docker y BIOS son
externos; los timestamps del ISO pueden variar al reconstruir el disco.

**Prueba de este disco en Saturn físico con CD-R: pendiente del usuario.**
No se declara ROM-EXACT universal, pixel perfect completo ni aptitud física
por haber superado HOST y Mednafen. Permanecen fuera de este disco los menús,
introducciones, audio y la escena posterior al desenlace; se conserva el ring.
