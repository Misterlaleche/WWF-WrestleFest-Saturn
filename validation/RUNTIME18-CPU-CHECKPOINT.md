# RUNTIME-18: CPU, acciones y presentación — 2026-10-20

Checkpoint parcial sobre `e1113d3`, rama `runtime18/direct-combat`. Se superan
los límites observados en `$1F7F0` y `$1C3FC`, con implementaciones nativas
contrastadas contra ejecución de la ROM original. **No se ha completado un
combate. No se declara ROM-EXACT ni aptitud para Saturn real.**

## Autoridad y alcance implementado

ROM 68000 canónica: SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`;
SHA256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.
El emulador Musashi se usa solamente en HOST como oráculo; no se enlaza con
el ejecutable Saturn. No se parchea la ROM del oráculo.

- `$1F7F0`: arbitraje, bancos de participantes, índice de secuencia y carry.
- `$1C3FC`: decisiones del estado cero, persecución y continuación `$1C5B6`.
  También se amplían las reacciones del estado uno y las decisiones de agarre.
- Controladores `$15/$16`, velocidad y marcha enlazada `$14DA2`, reacción
  `$13034`, relevo `$196AE` y entrada al ring `$18948`, incluidos efectos
  compactos reales y escrituras de paleta.
- Cuerdas `$11D56/$11DE0`, entrada `$12078`, espera `$121E6`, recoil `$1B478`,
  recuperación `$1B1D0` y salto ofensivo `$153BC`, con sus tres rutas de contacto.
- Las tablas de decisión se extraen como datos de ventanas exactas de la ROM;
  el generador verifica su hash y permite comprobar reproducción byte a byte.

Los límites de otras ramas continúan siendo explícitos. Superar estas dos
secuencias no demuestra que estén implementadas todas las acciones o CPU.

## Pruebas reproducibles

Desde la raíz del repositorio, con los archivos descritos en RUNTIME18-DISC.md:

```sh
make -C src/runtime test test-runtime18-direct test-runtime18-hot-sprites \
  test-runtime18-rom-cache test-runtime18-ring-scanline \
  test-runtime18-ring-scroll-cache test-runtime18-yaul-dma test-runtime18-cdfs
python3 -m venv /tmp/wf-rom-oracle
/tmp/wf-rom-oracle/bin/pip install -r validation/requirements-rom.txt
make -C src/runtime test-runtime18-cpu-rom ROM_PYTHON=/tmp/wf-rom-oracle/bin/python
/tmp/wf-rom-oracle/bin/python validation/run-rom.py /path/main68k.bin \
  /path/wwfwfest.zip --out validation/results/runtime18/rom
DOCKER=/path/to/docker sh validation/build-runtime18.sh /path/to/sh2eb-elf
```

El oráculo CPU/acciones ejecuta **888 casos**; compara los nueve actores,
D0–D7, RNG, carry de arbitraje y, según la ruta, globals, paletas, efectos y
colas de render. Grupos (índices de caso inclusivos):

| Casos | Propietario/ruta |
|---|---|
| 0–71 | Estado CPU 12 |
| 72–95 | Persecución desde estado 0 |
| 96–287 | Arbitraje `$1F7F0` |
| 288–335 | Controladores `$15/$16` |
| 336–359 | Estado 2/cuerdas |
| 360–383 | Acción `$16` |
| 384–407 | Estado 6 |
| 408–431 | Acción `$06` |
| 432–455 | Acción `$66` |
| 456–479 | Estado 0 con flag34.7 |
| 480–503 | Estado 1 frente a acción del oponente |
| 504–527 | Acción `$4D`, efectos y cola |
| 528–551 | Estado 8 |
| 552–575 | Estado 1 frente a carrera |
| 576–599 | Estado 9 |
| 600–695 | Estado 4, subtipo `$0E` |
| 696–791 | Recuperación `$1B1D0` |
| 792–887 | Salto ofensivo `$153BC` |

Se mantienen las comparaciones anteriores de TAG, Royal, frame activo,
registros del render y auditorías de fuente generada. El test del transporte
compila el backend Yaul real contra una frontera de prueba: exige una DMA
para 256 tiles, bytes exactos, rechazo de streams incompletos y sincronización
diferida. Ese doble no constituye prueba de hardware.

## Integración y límite actual

- Sin entrada: prueba satisfactoria de 540 frames; bloqueo real en frame 545,
  fase `$F4C2`, callback de víctima `$1BCAA`.
- RIGHT en frames 60–179 y A en 180–183: prueba satisfactoria de 490 frames;
  bloqueo real en frame 493, fase `$1C150`, decisión CPU `$1CBF8`.
- Los índices de bloqueo son los índices de iteración desde cero del probe.
- Ambas secuencias se verifican también con AddressSanitizer/UBSan.

No se alteran entradas ni stamina para evitar estos límites. No se sustituye
ningún propietario pendiente por éxito. El siguiente trabajo es implementar
estas rutas con ROM, continuar la integración y observar un final de combate
real; los números de frames anteriores no equivalen a ese final.

## Presentación y medición

El backend prepara un stream contiguo de tiles en 32 KiB de HWRAM y realiza
una sola DMA al terminarlo. El pen 15 se conserva como color con ECD=1:
[manual Sega VDP1](https://www.infochunk.com/saturn/segahtml_en/hard/vdp1/hon/p06_34.htm).
VDP1 y VDP2 se presentan juntos antes del ACK de gameplay. Se usa `-O2` en SH-2.

Medición en Mednafen, sin entrada, exactamente frames 31–120 (90 frames):

| Configuración | VBlank en 30 | VBlank en 120 | VBlanks/frame |
|---|---:|---:|---:|
| Base `e1113d3` | 558 | 937 | 4,2111 |
| DMA agrupada y ECD | 543 | 900 | 3,9667 |
| Sincronización conjunta | 540 | 882 | 3,8000 |
| SH-2 `-O2` | 517 | 808 | 3,2333 |

La reducción de tiempo en ese tramo es **23,2 %**. Todavía es lento. No es
una medición de partida completa ni de Saturn físico. Las paradas de
instrumentación utilizadas para medir fueron retiradas del BIN/CUE entregado.
Las capturas diagnósticas y performance.json conservan los resultados.

El BIN/CUE final arranca en Mednafen y muestra el ring y un agarre entre
luchadores. La captura `mednafen-cpu-final.png` conserva esa observación.
El árbitro presenta colores sospechosos: la fidelidad visual sigue pendiente
de contraste de framebuffer/paletas con el arcade.

El paquete incluye fuente, assets locales necesarios, BIN/CUE, evidencia y
SHA256SUMS. El SDK Yaul, BIOS de Saturn, Docker y dependencias de oráculo deben
estar disponibles para repetir las pruebas correspondientes. El build fija
la imagen Docker por digest; la fuente incluye los generadores y verificadores.

La reproducción reconstruye desde la fuente y assets identificados; los
timestamps de ISO9660 pueden variar entre builds. Los hashes entregados
identifican el disco probado, no garantizan identidad de metadatos entre builds.
