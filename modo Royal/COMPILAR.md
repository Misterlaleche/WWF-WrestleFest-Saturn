# Restaurar, compilar y comprobar HW1

## Dependencias y ruta correcta

Usar Linux (o WSL), Python 3, `make`, `xorrisofs` y el SDK Yaul suministrado con el proyecto. El restaurador crea archivos con descriptores POSIX y apertura sin seguir enlaces, también ante cambios concurrentes de directorios; no dispone de una alternativa menos estricta basada solo en nombres de ruta. La cadena documentada para HW1 es SH-2 `sh2eb-elf` GCC 13.2.0. BIOS, SDK y emuladores no están incluidos en Git ni en el paquete HW1 de fuentes. No sustituir un recurso privado por otro de procedencia desconocida.

El Makefile activo es `wp1980/work/hwfix01/native/Makefile.wp1980`. El `native/Makefile` general se conserva por fidelidad y pruebas heredadas; no es el comando de compilación HW1.

## 1. Restaurar recursos con sus hashes

Desde `modo Royal/`:

```bash
python3 tools/test_restore_resources.py
python3 tools/restore_resources.py /ruta/wp1980_hw1_fuentes_y_pruebas.zip --scope build
python3 tools/restore_resources.py --scope build --verify
```

La comprobación `build` cubre **356 archivos / 17.841.618 bytes**: 337 fuentes y textos conservados en Git, y 19 binarios externos. El SHA-256 de todo el ZIP está fijado en `RECURSOS.json`; el mismo manifest contiene tamaño y hash de cada miembro. El comando rechaza otro ZIP, miembros inconsistentes, rutas inseguras, enlaces simbólicos y archivos de destino diferentes. Comprueba todos los destinos antes de crear archivos y conserva la fecha/bytes de los ya idénticos.

El comando no modifica archivos para «arreglar» un hash. Si existen cambios locales, copiar esta carpeta a un destino limpio para reconstruir la referencia. Para trabajar con una copia ya verificada y modificada, registrar nuevos hashes y mantener separados los de HW1.

Para las pruebas RED/GREEN, el oráculo y las capturas históricas, restaurar todos los miembros:

```bash
python3 tools/restore_resources.py /ruta/wp1980_hw1_fuentes_y_pruebas.zip --scope all
python3 tools/restore_resources.py --scope all --verify
```

`all` materializa **1.444 archivos / 90.637.756 bytes**. Incluye la raíz anterior `wp1980/native/`, `audio/`, Musashi, fixtures y evidencia amplia. Son recursos de trabajo externos: no añadirlos indiscriminadamente a Git. Los fuentes seleccionados que ya están en Git conservan exactamente sus bytes, incluidos finales CRLF. `.gitattributes` desactiva su normalización.

## 2. Copiar el trabajo a una ruta sin espacios

El SDK usado contiene reglas que no aceptan espacios en la ruta de build. El nombre de esta carpeta en Git sigue siendo **`modo Royal`**. Compilar en una copia nueva de `wp1980/`, por ejemplo:

```bash
python3 - <<'PY'
from pathlib import Path
import shutil
destination = Path('/tmp/wp1980-hw1-build')
if destination.exists():
    raise SystemExit('Elige un destino nuevo: no se sobrescribe una copia previa')
shutil.copytree(Path('wp1980'), destination)
print(destination)
PY
cd /tmp/wp1980-hw1-build
export WP1980_TOOLING_ROOT=/ruta/sin_espacios/al/tooling
```

`WP1980_TOOLING_ROOT` debe contener `sh2eb-elf/` y, si corresponde, `host/`. También se admite `YAUL_INSTALL_ROOT` apuntando a un SDK compatible. Establecer una de estas rutas explícitamente; los fallbacks históricos del script no identifican una dependencia descargable ni válida en otra máquina. No cambiar el SDK para sortear un fallo de memoria o flags.

## 3. Compilar base y cart secuencialmente

Desde la copia sin espacios:

```bash
bash work/hwfix01/tooling/build.sh WP1980_CART=0 \
  SH_BUILD_DIR=audit/hw1-base-rebuild \
  SH_OUTPUT_DIR=audit/hw1-base-rebuild-disc

python3 validation/verify-hwram-budget.py \
  work/hwfix01/native/audit/hw1-base-rebuild/wrestlefest-wp1980.map
python3 validation/verify_saturn_stack_layout.py \
  work/hwfix01/native/audit/hw1-base-rebuild/wrestlefest-wp1980.elf

bash work/hwfix01/tooling/build.sh WP1980_CART=1 \
  SH_BUILD_DIR=audit/hw1-cart-rebuild \
  SH_OUTPUT_DIR=audit/hw1-cart-rebuild-disc

python3 validation/verify-hwram-budget.py \
  work/hwfix01/native/audit/hw1-cart-rebuild/wrestlefest-wp1980.map
python3 validation/verify_saturn_stack_layout.py \
  work/hwfix01/native/audit/hw1-cart-rebuild/wrestlefest-wp1980.elf
```

**No lanzar ambos builds a la vez:** comparten `native/audit/wp1980-release-cd/` y su `A.BIN` temporal. El script puede compilar objetos de una variante con `-j4`; la exclusión se refiere a las dos variantes completas. Cada una tiene su carpeta de programa y disco propia. No relajar las reservas de 16 KiB / 8 KiB ni mover las pilas para hacer pasar un enlace.

Los parámetros entregados mantienen efectos (`WP1980_AUDIO=1`), música desactivada (`WP1980_DISABLE_MUSIC=1`), 3.584 slots VRAM y cabecera de atlas de 4.096 bytes. `WP1980_CART=1` exige una expansión de 4 MiB; `0` es la variante base.

Para convertir el ISO a BIN/CUE MODE1/2352 con EDC/ECC:

```bash
python3 validation/iso_to_bin_exact.py /ruta/imagen.iso /ruta/salida_nueva
python3 validation/verify_disc_exact.py --help
```

Comprobar sectores, ISO9660 y que `A.BIN` corresponde al binario enlazado de **esa variante**. No prometer igualdad de hash de la imagen reconstruida si cambian los timestamps internos del ISO. Los hashes de los discos originalmente entregados están en `ESTADO.json` y en el informe HW1.

## 4. Firmware de efectos

Los 19 recursos restaurados incluyen el `SCSP.ELF` corregido. Para reconstruirlo por separado, sin reemplazar ese archivo congelado:

```bash
python3 research/hardware-result-01/audio-ring/build_firmware.py \
  --source work/hwfix01/audio/native \
  --tooling /ruta/al/tooling \
  --out /ruta/nueva/firmware
```

Requiere el compilador 68000 suministrado. Comparar el resultado y la procedencia antes de introducirlo en otro disco. La corrección es la del anillo PCM descrita en [ARQUITECTURA.md](ARQUITECTURA.md), no una activación de música.

## 5. Pruebas host y comparación con ROM

Después de restaurar `all`, desde la copia sin espacios:

```bash
python3 research/hardware-result-01/run_hw1_probe.py check \
  --source-root work/hwfix01 --original-root . \
  --out research/hardware-result-01/rerun-result-probes
```

Requiere Python 3.8+ y un compilador C11 host con `--gc-sections`. `--out` debe ser nuevo. El wrapper verifica los hashes de ROM, ROYINIT, informe y actores; compila referencias originales en el mismo host y compara snapshots nuevos, evitando depender del layout de estructuras del compilador de la sesión histórica.

El contrato completo comprende 13 ejecuciones: 5 RED deben devolver fallo de la prueba real y 8 GREEN deben pasar. Un fallo de compilación no cuenta como RED. El oráculo ejecuta el tramo 68000 original; las variantes corregidas deben producir la misma proyección de placa y completar el resultado sin lecturas posteriores a la precarga, incluso después de sustituir las cachés reemplazables. [PORTABLE-PROBES.txt](wp1980/research/hardware-result-01/PORTABLE-PROBES.txt) detalla comandos individuales y límites.

El test del anillo PCM puede repetirse en la copia de trabajo:

```bash
python3 research/hardware-result-01/audio-ring/run_regression.py \
  --source work/hwfix01/audio/native/wf_scsp_sfx.c --label rerun-hw1
```

Usar una etiqueta nueva; este runner histórico guarda sus resultados junto al script. Las fuentes se conservan, pero se generan un ejecutable host y un informe nuevo.

La suite general heredada tiene un bloqueo conocido:

```text
make: *** No rule to make target 'tests/test_core.c', needed by 'build/test_core'. Stop.
```

No inventar ese archivo ni declarar el conjunto completo PASS. Registrar el bloqueo junto con las pruebas específicas realmente ejecutadas.

## 6. Mednafen y medición reproducible

La aceptación HW1 conservada corresponde a **Mednafen standalone 1.32.1**, NTSC, caché CPU `full`, reloj normal, fast-forward 1 y frameskip 0. Para repetir, usar el CUE y el ELF de la misma compilación:

```bash
python3 tooling/mednafen/run_mednafen.py /ruta/imagen.cue /tmp/royal-captura-nueva \
  --elf /ruta/wrestlefest-wp1980.elf \
  --bios /ruta/BIOS_privada/mpr-17933.bin \
  --mednafen /ruta/mednafen \
  --sdk /ruta/tooling/sh2eb-elf \
  --cartridge 4MiB --cache full --offscreen
```

Para base, usar `--cartridge None`. Requiere `ffmpeg`, `ffprobe` y Pillow. `tooling/beetle/run_beetle.py` se importa para comprobar la identidad del disco/A.BIN; esa importación no ejecuta Beetle. Si hay dependencias dinámicas suministradas, establecer `WP1980_MEDNAFEN_RUNTIME_ROOT`. Para mando, Xvfb/xdotool pueden seleccionarse con `WP1980_HOST_TOOLS_ROOT`; omitir `--offscreen` y consultar escenarios en `tooling/mednafen/README.txt`.

El observador comprueba la identidad de su proceso hijo y lee su memoria; no inyecta savestates ni escribe RAM/registros. Su muestreo es asíncrono. La comprobación `boundary_stable` filtra pares de contadores incoherentes y no convierte cada observación en un snapshot atómico de todo el juego.

Para vídeo y análisis de la ejecución nueva:

```bash
python3 tooling/mednafen/verify_media.py /tmp/royal-captura-nueva
python3 tooling/mednafen/analyze_execution.py /tmp/royal-captura-nueva \
  --reference research/hardware-result-01/fixtures/delivered-release-4mb/report.json
python3 research/hardware-result-01/analyze_hw1_terminal.py /tmp/royal-captura-nueva \
  --reference research/hardware-result-01/fixtures/delivered-release-4mb/report.json \
  --output /tmp/royal-analisis-nuevo --expect-zero-result-cd
```

`verify_media.py` verifica identidad de PTS MOV/MP4; no acelerar, interpolar ni retocar tiempos para alcanzar el objetivo. Para una pasada con `--no-movie`, `analyze_execution.py --pts-check /ruta/video-pts-check.json` permite reutilizar **explícitamente** la calibración del mismo modo de vídeo. La medida se calcula con actualizaciones/campos nativos, no con segundos de CPU del host.

Comparar el frame final, RNG y los nueve registros completos de actores, ausencia de fatal, duración del resultado y cambios de CD. Los contadores de CD cuentan transferencias completadas; cero incremento no demuestra por sí solo ausencia de intentos fallidos. La prueba de denegación de lecturas del host cubre por separado la dependencia eliminada.

El material restaurado contiene los registros de las pasadas históricas, pero el MOV grande no forma parte del ZIP. Conservar las comprobaciones PTS archivadas como evidencia histórica; para una captura nueva, generar su comprobación nueva. El funcionamiento emulado no reemplaza la prueba física descrita en `PROBAR_HW1_EN_SATURN.txt`.
