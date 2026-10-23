# RUNTIME-18 — sprites nativos y escrituras del ring

Continuación de `64d93b1`. Combate directo TAG, sin frontend ni audio. El
checkpoint conserva las decisiones originales y reduce trabajo de render.
No declara 60 FPS sostenidos, ROM-EXACT universal ni validación física.

## Cambios integrados

- Caché de bodies: 8 → 32 entradas, 6532 bytes. La medición del combate
  detectó expulsiones repetidas: 82 de los 90 frames iniciales necesitan más
  de ocho claves. Misses iniciales 695 → 39; tardíos 276 → 41. En 4391 frames,
  lecturas del parser 342552 → 199390, con hashes de registros y records
  idénticos. No se cachean decisiones de cámara, paleta, clipping ni gameplay.
- Cada comando VDP1 se escribe completo una vez, con el mismo CB16/replace,
  transparencia de pen0 y end-code desactivado para conservar penF. Las
  máscaras de residencia/dirty de ocho bits usan una tabla, evitando las
  llamadas de desplazamiento variable observadas en el ejecutable SH-2.
- Los sprites lógicos siguen siendo tiles ROM 16×16. VDP1 recibe sprites
  normales 16×1..16 cuando hay filas exteriores completamente transparentes.
  SRCA, altura y origen Y respetan ambos espejados. Los límites se calculan
  sobre staging HWRAM cuando cambian sus bytes, y se reutilizan mientras
  estén residentes o pendientes. Aborto/reintento invalida la residencia;
  los 512 bytes nuevos no contienen imágenes reconstruidas.
- `$298EC` escribía el parche de cuerdas en RAM BG, pero no alcanzaba VDP2.
  Ahora sus escrituras llegan al overlay residente, después de bootstrap y
  antes del transporte IRQ. Se prevalidan los dos vectores completos del
  batch; byte/word/long y solapamientos preservan la celda actual del
  streaming y las animaciones. La restauración posterior conserva el orden
  original. No se modifica `$247C/$27B8/$2836` ni el decoder de poses.
- La cola temporal de 4096 escrituras de vídeo conserva su capacidad y pasa
  de HWRAM a LWRAM `$00254000..$00260000`. ROM cache ocupa 208 KB en
  `$00220000..$00254000`, con 172 páginas precargadas y 36 huecos. Tile cache
  conserva 640 KB en `$00260000..$00300000`. Las fuentes de DMA VDP1 siguen
  en HWRAM. El observador comprueba ring, páginas ROM y sprites precargados
  intactos dentro de la misma LWRAM después de ejecución real.

El modo de sincronización VDP1 permanece AUTO. La prueba VARIABLE no mejoró
el tramo inicial con la base y se descartó. Un intento de congelar el
framebuffer borraba sprites en ambos modos: no se usó como evidencia de
retardo de imagen. Los experimentos descartados no forman el disco entregado.

## Rendimiento y resultado visual

Las cifras son VBlanks emulados de Mednafen 1.32.1, no FPS del ordenador
usando `nothrottle`. Mismas entradas y semilla en cada ventana. Los tiempos
runtime incluyen presentación; no se deben sumar etapas anidadas.

Los valores finales y hashes exactos constan en
`evidence/checkpoint-sync/performance-verification.json` del ZIP. El tramo
30–120 pasa de 99 a 98 VBlanks/90 frames. CPU runtime pasa de 180459 a
168014 ticks FRT/128 (−6,9 %); presentación 109489 → 97148 (−11,3 %).
El tramo 900–1080 conserva 198 VBlanks/180 frames. La mejora de cadencia
es pequeña y no equivale a alcanzar 60 FPS.

El recorte reduce filas enviadas a VDP1, pero no se le atribuye por separado
una mejora de cadencia: la candidata con caché de 32 cuerpos sin recorte
también obtuvo 98/90 y consumió menos CPU en esa ventana (162671 ticks
runtime frente a 168014 finales). La mejora global se compara siempre con
`64d93b1`; reducir trabajo de VDP1 no implica por sí solo reducir tiempo SH-2.

En el frame 120 se envían 1474 filas de sprites donde antes eran 1840
(−19,9 %). El corpus completo HOTSPR conserva sus bytes y pasa de 63888 a
51988 filas por orientación (−18,6 %). No se omiten frames ni fases.

La apertura/restauración original de cuerdas está integrada y contrastada
contra instrucciones ROM. **Su comparación visual es una reconstrucción
software**, no una captura Mednafen de esa acción. Ninguna de las dos replays
canónicas activó `$8218A`; tampoco se obtuvo una secuencia manual reproducible
en 64 exploraciones. Varias exploraciones alcanzaron otras dependencias de
acciones aún bloqueadas. No se forzaron actores para simular una demostración.
La ruta no se presenta como cobertura completa del juego.

La partida normal completa 4391 frames en **4887 VBlanks**, frente a 4919
del checkpoint anterior (−0,65 %). Conserva las 131 esperas del resultado
y los píxeles de las capturas del frame 120 y GAME OVER. HWRAM termina
en `$060F4974`: 46732 bytes libres estáticos frente a los 4492 anteriores.

## Pruebas y alcance

- 365 casos `$D1FC` contra ROM, con caché fría/caliente y sin caché; mismos
  records y D6/D7. Regresión de 16 cuerpos cíclicos: 48 relecturas con ocho
  entradas → cero; conservación de 32 cuerpos y expulsión del 33.
- 256 encodings verifican los 32 bytes de comandos, todos los slots del
  staging, paletas, flips, coordenadas y ausencia de tail obsoleto.
- 20595 comparaciones con un raster independiente completo 16×16: todas
  las alturas/top, cuatro espejados, paletas, penF, clipping, orden,
  mutación de texturas, residencia, aborto/reintento y 3993 tiles ROM.
  Normal y ASan/UBSan. Las capturas Saturn comparan muestras con el
  checkpoint anterior; no acreditan equivalencia universal al framebuffer arcade.
- 96 ejecuciones originales `$298EC` en Musashi, sin reemplazar el owner:
  BG completo, D0–D7 y las 18 celdas VDP2. Tests de escrituras parciales,
  batch inválido atómico, streaming/cámara, píxeles y restauración IRQ.
- Suite general, CORE 48/48 y 48/48 con ASan/UBSan; transporte/DMA,
  HUD/paletas, 1540 casos del planificador y pipeline ROM independiente.
  Partidas HOST normales y ASan: 4391 sin entrada y 14409 con entrada
  controlada, con cero lecturas CD durante juego y 131 VBlanks de resultado.
- Enlace SH-2 sin símbolos sin resolver. Sectorización MODE1/2352, IP,
  ISO9660 y EDC/ECC verificados. Prueba Mednafen del disco normal y replay,
  resultado original y memoria: ver `FINAL-VERIFICATION.json` y observaciones.
- Revisión independiente sin Critical/Important confirmados. El test de
  raster HOST no sustituye la ejecución SH-2 ni una prueba de Saturn físico.

**Saturn real con CD-R: pendiente del usuario.** Quedan rutas de acciones sin
implementar que bloquean explícitamente; estas dos partidas no prueban todas
las combinaciones de entrada, roster, modos, introducciones o sonido.

## Reproducir

```sh
make -j4 -C src/runtime test
make -C src/runtime test-runtime18-sprite-crop-asan test-runtime18-metasprite-cache-asan
make -C src/runtime test-runtime18-background-ring-asan test-runtime18-command-work
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match
make -C src/runtime test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
python3 validation/run-host.py src/runtime --out validation/results/runtime18/core-host
python3 validation/run-host.py src/runtime --sanitize --out validation/results/runtime18/core-sanitizers
python3 src/runtime/tests/r18/verify_background_rom.py
python3 src/runtime/tests/r18/verify_plan_equivalence.py
DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 R18_PROFILE_START=900 R18_PROFILE_STOP=1080 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_REPLAY_TEST=noinput DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

Los tests ROM necesitan los módulos fijados en `validation/requirements-rom.txt`.
El ZIP contiene fuente del commit, cinco assets canónicos, ROM de referencia,
Git bundle, discos y evidencia. Docker, SDK fijado por digest y BIOS son
externos. La exportación fría sin Git reconstruye un A.BIN idéntico al probado;
los timestamps ISO pueden variar. Se verifica cada fichero del manifiesto
SHA256 y que el BIN/CUE incluido coincide con el probado. Abrir/grabar el CUE
con su BIN al lado: una pista de datos MODE1/2352.

Disco normal probado (SHA256):

- A.BIN: `79eca3573273abfd5bc560aa5aa2a906e9ed1ecb54936d26a85ded42f11f5547`
- wrestlefest-runtime18.bin: `553ce1bb4eb484b08e7d3a42ef83c8f5bb4084cd80e447c1d8b8cc47ba6775eb`
- wrestlefest-runtime18.cue: `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`
