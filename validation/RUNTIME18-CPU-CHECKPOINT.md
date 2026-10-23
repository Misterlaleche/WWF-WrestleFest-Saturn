# Estado actual: capas y fluidez desde a680f7a

Ver [RUNTIME18-LAYERS-PERF-CHECKPOINT.md](RUNTIME18-LAYERS-PERF-CHECKPOINT.md)
para el disco actual, medidas, precarga3993tiles/171páginas y pruebas frescas.
Se conservan los finales4391/14409 y la cámara canónica. El replay final
Mednafen llega4391 sin bloqueos. Los hashes siguientes son históricos.

## Histórico426ed17 — cierre de combate sin entrada

# RUNTIME-18 — combate sin entrada desde3435a09

Checkpoint local de 23 de octubre de 2026, rama `runtime18/direct-combat`.
El commit consolidado consta en CHECKPOINT.txt del paquete. Sin cambios
publicados en GitHub ni regreso al frontend.

## Resultado

La secuencia sin entrada, semilla de reset cero y todos los puertos a FF,
completa el combate en **frame 4.391**, frontera original `$11B6`. El árbitro
cuenta uno, dos y tres en frames 3.989/4.077/4.165. Al final, `$169A=0300`,
reloj `$16A0=0021`, resultados de actores0/1/4/5 `8001,8000,4000,4000`.
El jugador pierde por inmovilización. No se fuerza el resultado, se acorta el
reloj, se altera la stamina ni se cambia una decisión de la CPU.

HOST, ASan/UBSan y el SH-2 final en Mednafen llegan a ese mismo final.
A.BIN del replay tiene SHA256
`e34888506fc3698efe531191497d58d804e8029e2ec59a56a28bc279013e527b`. El observador sólo lee la memoria del emulador: identifica
4.096 bytes de instrucciones del A.BIN exacto y consulta los diagnósticos.
El observador de final del juego exige flags reales después del ACK; cumplir
un presupuesto de frames no cuenta como éxito.

La partida controlada anterior —RIGHT60–179, A180–183, semilla 0— se conserva
como regresión completa a14.409 frames por tiempo. El disco normal lee el mando
y retiene el ring al finalizar. Dos modos de compilación separados permiten
repetir las entradas controladas o la secuencia sin entrada; sólo los discos
de validación muestran el informe verde y el frame codificado al terminar.

## Evidencia ROM

ROM 68000 SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`, SHA256
`265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.
Los oráculos ejecutan instrucciones originales de esta ROM. El runtime Saturn
sigue siendo C nativo, sin intérprete68000 enlazado. `$2052` conserva registros
por el contrato de sonido silencioso. Las escrituras a ROM se ignoran en los
casos que las alcanzan, manteniendo sus bytes originales para lectura.

| Cambio desde3435a09 | Comparaciones nuevas |
|---|---:|
| `$59/$19274`: enlaces liberados/null/tagged y limpieza213A6 | 1.536 |
| CPU1CB30→1DE66: selección con tier0/1/2 | 1.152 |
| Proyección33/$17282 y reversión84/$1A950 | 2.304 |
| Acción0E/$13D46: cuatro fases, impacto y fallo | 3.072 |
| Víctima51/$18C24 y transición al pin | 1.920 |
| Aproximación CPU1CD5C/1CDD0 | 864 |
| Aproximación alineada1CE38 y decisión1F310 | 2.304 |

Se comparan actores, globals, RNG y D0–D7. Los casos incluyen semántica de
BYTE/WORD, orientación, fases de animación y modos. La prueba ampliada de0E
exigió conservar el cambio a movimiento2 de `$25B2`. La víctima51 descubrió
que el primer peso de la decisión defensora Royal es40 (`$21838=$28,$3C`),
no32. Ambas correcciones se verifican contra la ROM. La revisión independiente
detectó además que84 debe guardar A2 completo en+$92, incluidos los bits
altos del link. El caso etiquetado falló antes de corregirlo y pasa después.
La revisión añadió 5.440 comparaciones diferenciales y no deja hallazgos abiertos.

El bloqueo59 era una validación anticipada de A2: el original sólo lo usa en
las ramas de inicialización o liberación. Una animación inactiva ya liberada
no necesita un actor ficticio. Null literal usa `$7F000`; un puntero con bits
altos marcados puede acceder físicamente a ROM0. Las ramas realmente no
implementadas o los punteros no cubiertos conservan el bloqueo explícito.

Las matrices ROM anteriores de CPU, daño, pin, cuenta, árbitro, paletas,
víctimas, TAG, Royal, frame activo y registros siguen en la validación agregada.
Los logs conservan sus recuentos completos. Las48 regresiones del núcleo y
las dos partidas completas pasan también con ASan/UBSan.

## Precarga, ejecución y límites

HOTSPR.BIN conserva2.944 tiles y128 huecos de caché adicionales en384 KiB.
SHA256 `d17ab8e077803b9d491a77c02876c9d64c9bf4609bc5838fc50dc6328fd0d603`.
La partida controlada conserva cero lecturas de SPRITES.PAK tras la precarga.
La partida sin entrada hace **11.984 lecturas de983 tiles adicionales**; su
traza CSV se entrega. Esto puede frenar el combate y requiere trabajo de
precarga/caché posterior. No se declara velocidad arcade ni una precarga de
todas las acciones o todo el roster.

Los discos SH-2 usan GCC 13.2.0 y Yaul fijado por digest, sin símbolos sin
resolver. El SDK emite sus avisos conocidos de segmentosRWX. La validación
estructural comprueba todos los sectores MODE1/2352, EDC/ECC, IP, ISO9660 y
payloads, incluido A.BIN. Es evidencia distinta de ejecutar el juego.

No se han ejecutado pruebas en Saturn físico con CD-R. Sin declaración
ROM-EXACT, igualdad de framebuffer o aptitud para hardware. El disco normal
conserva el último ring; no implementa la escena posterior. TIME UP aún no
se publica en VDP. No se han validado todas las entradas, semillas o parejas.

## Reproducción

Desde el árbol incluido, con los cuatro assets locales en
`src/runtime/audit/runtime18-cd/`:

```sh
make -C src/runtime -j3 test test-runtime18-direct \
  test-runtime18-complete-match test-runtime18-noinput-complete-match \
  test-runtime18-hot-sprites test-runtime18-rom-cache \
  test-runtime18-ring-scanline test-runtime18-ring-scroll-cache \
  test-runtime18-yaul-dma
make -C src/runtime test-runtime18-complete-match-asan \
  test-runtime18-noinput-complete-match-asan
python3 validation/run-host.py src/runtime --sanitize --out /tmp/wf-core-sanitizers
python3 -m venv /tmp/wf-rom-oracle
/tmp/wf-rom-oracle/bin/pip install -r validation/requirements-rom.txt
make -C src/runtime -j2 test-runtime18-rom-validation \
  ROM_PYTHON=/tmp/wf-rom-oracle/bin/python
/tmp/wf-rom-oracle/bin/python validation/run-rom.py \
  src/runtime/audit/runtime18-cd/ROM68K.BIN /path/wwfwfest.zip \
  --out /tmp/wf-core-rom
DOCKER=/path/to/docker sh validation/build-runtime18.sh /path/to/sh2eb-elf
make -C src/runtime test-runtime18-cdfs
R18_REPLAY_TEST=noinput DOCKER=/path/to/docker \
  sh validation/build-runtime18.sh /path/to/sh2eb-elf
```

El replay sin entrada sale en `validation/results/runtime18/replay-noinput/`;
el disco normal en su carpeta padre. Mantener juntos BIN/CUE y abrir o grabar
el CUE. `R18_REPLAY_TEST=1` reconstruye el replay controlado anterior en
`replay/`. SDK, BIOS, Docker y dependencias de oráculo son externos. Los
metadatos de fecha ISO pueden variar; los hashes identifican los bytes
entregados y probados. El paquete contiene fuente, assets, discos, pruebas y
SHA256SUMS; los binarios históricos no se mezclan con el replay nuevo.
