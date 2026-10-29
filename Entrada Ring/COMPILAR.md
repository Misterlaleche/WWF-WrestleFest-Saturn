# Compilar WP01 R02 — audio nativo SCSP

La reconstrucción normal usa las fuentes de `source/`, los gráficos ya extraídos de la ROM y los dos recursos de audio incluidos en `source/cd/`. No requiere volver a ejecutar el arcade, extraer la ROM ni generar una grabación de música. Todos los comandos de esta guía se ejecutan desde la carpeta que contiene `source/`, `tools/` y `research_scsp/`.

Trabaja en una copia nueva de este paquete. `build.sh` escribe sus productos dentro de `source/`; los generadores opcionales sustituyen únicamente sus productos dentro de esta copia R02.

## 1. Dependencias

| Operación | Dependencias |
|---|---|
| Compilar Saturn y crear ISO | Linux, Bash, GNU Make y el SDK Yaul `sh2eb-elf` aportado al proyecto, con sus bibliotecas, cabeceras, scripts `share/` y herramientas `make-ip`, `make-iso` y `make-cue`. La compilación entregada utiliza GCC SH-2 13.2.0. |
| Crear ISO | `xorrisofs`, instalado con sus bibliotecas dinámicas. El script del SDK comprueba que sea el ejecutable `xorrisofs`. |
| Convertir ISO a BIN/CUE de sectores de 2352 bytes | Python 3; sólo biblioteca estándar. |
| Regenerar los recursos nativos de audio | Python 3 y `gcc` para el ordenador anfitrión, con soporte C11. |
| Regenerar y contrastar la decodificación OKI original | Además, `g++` con soporte C++17 y los fuentes MAME 0.264 indicados abajo. |
| Pruebas de cola de audio, parejas, FM y OKI | Compilador C del ordenador anfitrión. |
| Reconstrucciones gráficas y análisis de señales de investigación | Dependencias adicionales indicadas en la sección 6; no participan en la compilación normal. |

El programa Saturn compila cinco unidades C: `main.c`, `wp01_scene.c`, `wp01_audio.c`, `wp01_audio_queue.c` y `cd_transfer32.c`. El planificador de eventos utiliza el SH-2 y programa el SCSP. Esta versión no compila un programa para el 68000 de sonido; la variable de prefijo M68K de `build.sh` forma parte de la configuración general del SDK.

## 2. Construir la ISO y el BIN/CUE

Configura rutas absolutas a tu instalación del SDK y a `xorrisofs`:

```sh
export YAUL_INSTALL_ROOT=/ruta/al/sh2eb-elf
export MAKE_ISO_XORRISOFS=/ruta/a/xorrisofs

bash source/build.sh clean
bash source/build.sh

python3 source/tools/iso_to_bin.py source/WF-WP01-R02.iso disco-recompilado
```

Si `xorrisofs` procede de una instalación local, sus bibliotecas deben estar disponibles para el cargador dinámico, por ejemplo mediante `LD_LIBRARY_PATH`. `build.sh` añade el directorio `bin/` de Yaul a `PATH` y configura los prefijos del compilador y el asignador `tlsf`.

La última orden crea en `disco-recompilado/`:

- `WF-WP01-R02.bin`: imagen de disco con sectores completos de 2352 bytes.
- `WF-WP01-R02.cue`: descriptor de una pista de datos `MODE1/2352`.
- `disc-validation.json`: identidades SHA-256 y comprobación de todos los sectores, sus datos, EDC y ECC.

El conversor exige que los nombres de salida no existan. Usa otra carpeta para cada reconstrucción. El archivo `source/build/WF-WP01-R02.bin` es el ejecutable SH-2 intermedio; la imagen de disco para cargar o grabar es la que crea `iso_to_bin.py`. Abre el CUE junto a su BIN.

El paso `clean` afecta a los productos de compilación de esta copia. Es especialmente necesario tras regenerar `SOUNDRAM.BIN` o `AUDIOREG.BIN`: la regla ISO del SDK depende del ejecutable y de `IP.BIN`, pero no declara los restantes archivos de `cd/` como dependencias. Recompilar desde limpio garantiza que la ISO incorpore los recursos actuales.

Los avisos de segmentos ELF con permisos RWX proceden del script de enlace del SDK suministrado. Los mapas y el desensamblado de la compilación quedan en `source/build/`.

## 3. Qué contiene el audio incluido

`source/cd/SOUNDRAM.BIN` ocupa 346.208 bytes. Contiene las tablas periódicas de oscilador utilizadas por los operadores FM y las dos muestras individuales originales del OKI. `source/cd/AUDIOREG.BIN` ocupa 19.776 bytes y contiene 488 escrituras de inicialización y 2.224 escrituras temporizadas del SCSP, derivadas de los 748 eventos originales conservados en `research_events/export/entrance_audio_events.h`.

Las voces SCSP 0–23 realizan la síntesis FM; las voces 24–27 reproducen las muestras OKI. El reloj de los eventos es el FRT del SH-2 dividido por 32, con el racional `120093750/143` ticks por segundo para el modo NTSC de 320 puntos. La síntesis FM constituye una traducción a los operadores, envolventes y cuantización del SCSP; sus límites se describen en la documentación de FM.

Las identidades de los recursos entregados, también registradas en `evidence/native-audio-assets.json`, son:

```text
6938a51117b38ce536b675caf69c29e2061ef74d851fa2e738f9b9a2496bb9ee  SOUNDRAM.BIN
9b3e7fef5d8511eeab5bfdf7359c2a01d1306a42a76c10748137a27b566c2ba1  AUDIOREG.BIN
```

## 4. Regeneración opcional del audio

### Recompilar los eventos y reunir los bancos incluidos

```sh
python3 tools/build_native_audio.py
```

El script compila `tools/export_native_audio.c` junto a `research_scsp/wp01_fm.c` y `research_inventory/wp01_oki.c` con el `gcc` del anfitrión. Consume las tablas FM, el banco OKI y los eventos originales incluidos; vuelve a escribir ambos BIN de `source/cd/` y los informes `evidence/native-registers.csv` y `evidence/native-audio-assets.json`. Después aplica la sección 2 para reconstruir el disco.

### Regenerar también las tablas FM y el banco OKI

```sh
python3 research_scsp/generate_fm_tables.py
python3 research_inventory/build_oki_bank.py \
  --source-dir research_events/export \
  --out research_inventory/generated
python3 tools/build_native_audio.py
```

El generador FM lee las tablas de `ymfm_fm.ipp`. Para registrar la procedencia también necesita `ymfm_opm.cpp`, `scsp_mdfn.inc` y `SCSP-MANUAL.pdf` en `research_scsp/`. Produce `FM-SINE.PCM`, `wp01_fm_tables.h`, `fm-mapping-manifest.json` y la licencia de YMFM. Las tablas contienen formas de onda periódicas de oscilador.

El PDF del manual Sega no se redistribuye en este paquete. Para ese paso opcional, obtén el [SCSP User's Manual ST-077-R2-052594](https://antime.kapsi.fi/sega/files/ST-077-R2-052594.pdf) y guárdalo como `research_scsp/SCSP-MANUAL.pdf`; su SHA-256 de referencia está en `fm-mapping-manifest.json`. El script sólo lo abre para documentar su hash. La recompilación del disco y `tools/build_native_audio.py` funcionan con los recursos incluidos sin necesitar ese PDF.

El generador OKI lee `oki_bank0_sample001.pcm16be` y `oki_bank0_sample018.pcm16be`. Aplica sus volúmenes originales, la ganancia de mezcla documentada y el redondeo a PCM16; produce el banco, su cabecera de direcciones y el manifiesto de `research_inventory/generated/`.

Para repetir además la decodificación ADPCM y su contraste byte a byte con MAME 0.264:

```sh
python3 research_events/decode_oki.py
```

Este paso necesita los dos archivos `.adpcm`, `export/oki_samples.json` y `research_inventory/mame0264/okiadpcm.{cpp,h}`. Genera también los archivos temporales y el programa de contraste bajo `research_events/verify_oki/`. Una vez decodificado, repite la generación del banco OKI y la compilación de recursos anterior.

## 5. Pruebas reproducibles

Las siguientes pruebas del anfitrión no requieren emulador:

```sh
gcc -std=c11 -O2 -Wall -Wextra -Werror -I source \
  source/tests/test_audio_queue.c source/wp01_audio_queue.c -o /tmp/wp01-r02-queue
/tmp/wp01-r02-queue

gcc -std=c11 -O2 -Wall -Wextra -Werror -I source \
  source/tests/test_pairs.c -o /tmp/wp01-r02-pairs
/tmp/wp01-r02-pairs

gcc -std=c11 -O2 -Wall -Wextra -Werror -I research_scsp \
  research_scsp/test_fm_mapping.c research_scsp/wp01_fm.c -o /tmp/wp01-r02-fm
/tmp/wp01-r02-fm

gcc -std=c11 -O2 -Wall -Wextra -Werror -I research_inventory \
  research_inventory/test_wp01_oki.c research_inventory/wp01_oki.c -o /tmp/wp01-r02-oki
/tmp/wp01-r02-oki

gcc -std=c11 -O2 -Wall -Wextra -Werror -I research_inventory \
  research_inventory/test_wp01_oki_trace.c research_inventory/wp01_oki.c -o /tmp/wp01-r02-oki-trace
/tmp/wp01-r02-oki-trace
```

La comparación del movimiento necesita la captura RAM original de MAME usada en WP01, con su numeración de fotogramas original. Los analizadores de telemetría necesitan una captura real con `telemetry.csv` y `audio-telemetry.csv`:

```sh
python3 source/tests/test_native_scene.py --ram /ruta/a/mame_ram.bin
python3 source/tests/check_telemetry.py /ruta/a/captura/telemetry.csv
python3 source/tests/check_native_audio_telemetry.py /ruta/a/captura
```

La prueba de escena reproduce las decisiones aleatorias de la captura para contrastar posiciones, fases y contadores; no usa los valores generados por Saturn como referencia. Las pruebas de telemetría verifican el avance de eventos, reinicios y presentación gráfica sobre datos capturados. Estas órdenes documentan la reproducción de las comprobaciones; no capturan por sí mismas un emulador.

## 6. Portabilidad de las herramientas de investigación

La compilación normal y `tools/build_native_audio.py` resuelven sus archivos dentro de esta carpeta. Los generadores FM y OKI tampoco requieren la ruta del ordenador donde se hizo la entrega.

La recaptura del arcade es una operación separada. `research_events/capture/launch.json` documenta rutas absolutas del entorno de investigación: ejecutable MAME, ROM, complementos, bibliotecas y directorios de salida. Deben sustituirse por las de tu instalación para repetir aquella captura. `capture_audio.lua` recibe su directorio mediante `WF_CAPTURE_OUT` y su límite mediante `WF_CAPTURE_LIMIT`.

`research_events/export_events.py` también asume que `project_sources/01-wwfwfest.zip` está situado junto a la carpeta R02, en su directorio padre. Lee `research_events/capture/audio_writes.csv`. Ese requisito se aplica al reprocesado desde la ROM y la captura, no a los eventos exportados ni a la reconstrucción normal del disco.

Las herramientas de comparación de audio bajo `research_events/reference_render/`, `native_compare/` y `final_emulator/` usan recursos de investigación adicionales, compiladores C++, NumPy y/o SciPy. Sus ejecutables anfitrión deben reconstruirse para otra plataforma. `final_emulator/package_validation.py` conserva en su informe la ruta histórica `/tmp/wp01_r02_mednafen.mov`; el análisis lee los WAV locales, por lo que esa cadena identifica la captura de origen y no configura una nueva captura.

`source/tools/build_environment.py` recibe como argumentos la ROM, la captura de estados de vídeo y la carpeta de salida; requiere NumPy. Los gráficos listos para compilar ya están en `source/assets/`. Las capturas completas y las herramientas de investigación no son requisitos del programa Saturn.

## 7. Archivos que debe conservar un paquete de fuentes

Para reconstruir el disco basta con `source/`, conservando sus C, cabeceras, `Makefile`, `build.sh`, todos los recursos usados de `assets/`, `cd/SOUNDRAM.BIN`, `cd/AUDIOREG.BIN` y `tools/iso_to_bin.py`, más el SDK externo y `xorrisofs`. `cd/A.BIN` se vuelve a generar; el SDK crea los archivos TXT auxiliares si faltan.

Para conservar también la regeneración del audio, añade estos archivos respetando su jerarquía:

| Grupo | Archivos necesarios |
|---|---|
| Exportador nativo | `tools/build_native_audio.py`, `tools/export_native_audio.c` |
| Traducción FM | `research_scsp/wp01_fm.c`, `wp01_fm.h`, `wp01_fm_tables.h`, `FM-SINE.PCM`, `YMFM-LICENSE.txt` |
| Reproducción OKI | `research_inventory/wp01_oki.c`, `wp01_oki.h`, `generated/wp01_oki_assets.h`, `generated/oki_volume_bank.bin` |
| Eventos originales | `research_events/export/entrance_audio_events.h` |
| Regeneración de tablas FM | `research_scsp/generate_fm_tables.py`, `ymfm_fm.ipp`, `ymfm_opm.cpp`, `scsp_mdfn.inc`, `SCSP-MANUAL.pdf` |
| Regeneración del banco OKI | `research_inventory/build_oki_bank.py` y los dos `.pcm16be` de `research_events/export/` |
| Decodificación original opcional | `research_events/decode_oki.py`, los dos `.adpcm`, `export/oki_samples.json`, `research_inventory/mame0264/okiadpcm.cpp` y `okiadpcm.h` |

Incluye asimismo las pruebas citadas, los manifiestos JSON y la documentación de procedencia para poder revisar el resultado. Los productos de `source/build/`, `source/test-output/`, cachés de Python, ejecutables y bibliotecas compartidas del anfitrión se regeneran y pueden omitirse del paquete de fuentes.
