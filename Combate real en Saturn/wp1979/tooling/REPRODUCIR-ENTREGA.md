# Reproducir la entrega WP1979

Estas instrucciones corresponden al candidato final 4 y a los scripts presentes el 29 de octubre de 2026. Se han comprobado sus rutas, argumentos y dependencias mediante lectura de los archivos. Esta fase de documentación no ha recompilado ni vuelto a capturar el juego.

La compilación y la captura anteriores se realizaron en **Linux x86-64**. El SDK suministrado contiene ejecutables Linux x86-64; el core incluido es una biblioteca ELF x86-64 con dependencias `libm.so.6`, `libmvec.so.1` y `libc.so.6`. No se ha verificado este procedimiento en Windows, macOS, ARM ni una instalación Linux distinta. El paquete de fuentes no incluye todas las herramientas de compilación: las dependencias externas se enumeran abajo.

## 1. Identidad de la entrega

| Archivo | SHA-256 de referencia |
| --- | --- |
| BIN de disco final, sectores de 2352 bytes | `f1232edb31d6e3748b60001d8d82bca5f9b6171d16aaf812f618f3dbf858da5e` |
| CUE final | `e3a329a9b0e57b1b22440c4165e302fcdf64b5409a98149579f7fbb299ba0fdb` |
| ISO usado para convertir ese BIN | `a2078c9dab8a984b14be647dd8efa1b116d5121caebc157098ebaf6890ddb659` |
| Programa SH-2, equivalente a `A.BIN` | `49b03fc81c6801eb376979079f96746f1bbc887346b29c90d55f1003bad2720e` |
| ELF final con símbolos | `cf58d68ea37af04f6cbfdd382175329f4a5f40daa20b9c949f45f75faebb3967` |
| Core corregido de la captura | `176c4611333b458c6aa04cb877d69795070f05480bd5c082884c4259cacb6297` |
| Archivo fuente original de Yabause | `7f1bff1d902b8c0574c650463ba872d86029eb7c513b3e39ee03bf3b991795a4` |

El disco tiene una pista de datos MODE1/2352 y 6592 sectores. El nombre `wrestlefest-wp1979.bin` se utiliza tanto para el **programa SH-2** en el directorio de build como para el **disco completo** en el directorio de entrega; son archivos distintos.

El empaquetador ISO conserva fechas del sistema de archivos y no fija todas las marcas de tiempo. Por ello, recompilar y empaquetar en otro momento no garantiza el mismo SHA-256 del ISO/BIN. La conversión ISO→BIN sí es determinista para un ISO dado. `verify_disc_exact.py` comprueba boot area, sectores, EDC/ECC, `A.BIN` y contenido de todos los archivos contra el build que acaba de realizarse. Una versión diferente del compilador o de FFmpeg tampoco promete hashes idénticos del ELF o del MP4.

## 2. Directorio y requisitos

Los comandos siguientes mantienen el diseño actual: `native/`, `tooling/`, `measure/` y los assets bajo un mismo directorio `wp1979`. Situarse en ese directorio y definir una ruta absoluta:

```bash
cd /ruta/absoluta/a/wp1979
export WP1979_DIR="$PWD"
```

Usar una ruta **sin espacios**: el SDK rechaza espacios en `YAUL_INSTALL_ROOT` y sus scripts contienen otras limitaciones de escape.

Para recompilar el programa y el disco hacen falta Bash, GNU Make, Python 3, las utilidades POSIX usadas por el SDK, el SDK SH-2/Yaul suministrado y **xorrisofs**. Para capturar hacen falta además Python con Pillow, FFmpeg con `libx264` y AAC, y `nm` del sistema para la inspección de audio. El wrapper `measure/run_capture.py` ejecuta literalmente `python`, por lo que ese nombre debe resolver al mismo Python 3 con Pillow. Un entorno virtual puede proporcionar ambos nombres:

```bash
python3 -m venv "$WP1979_DIR/venv-repro"
source "$WP1979_DIR/venv-repro/bin/activate"
python -m pip install Pillow
```

Esta instalación de Pillow requiere una fuente de paquetes disponible; no está embebida en el paquete de fuentes. FFmpeg y `nm` también son dependencias del sistema.

### SDK original 14

El archivo del usuario `14-sh2eb-elf-yaul-1-.zip` ya contiene GCC 13.2.0, binutils, cabeceras, bibliotecas precompiladas Yaul y scripts de enlace/boot. Su SHA-256 es:

`8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`

Si aún no está colocado, extraerlo una sola vez, conservando su directorio superior `sh2eb-elf`:

```bash
unzip /ruta/al/14-sh2eb-elf-yaul-1-.zip -d "$WP1979_DIR/tooling"
source "$WP1979_DIR/tooling/env.sh"
test -x "$YAUL_INSTALL_ROOT/bin/sh2eb-elf-gcc"
test -f "$YAUL_INSTALL_ROOT/sh2eb-elf/lib/libyaul.a"
```

El ZIP 12 de libyaul es la fuente de referencia y su licencia; no es necesario reconstruir el SDK para esta receta. El build usa el SDK precompilado del ZIP 14. No mezclar una biblioteca Yaul de otra versión con esas cabeceras sin una validación nueva.

### xorrisofs externo

`tooling/env.sh` apunta por defecto a la copia local `tooling/host/usr/bin/xorrisofs` usada en el entorno de trabajo. El directorio `host` no se incluye en el paquete compacto. Después de cargar `env.sh`, indicar el ejecutable instalado en el equipo:

```bash
export MAKE_ISO_XORRISOFS="$(command -v xorrisofs)"
test -x "$MAKE_ISO_XORRISOFS"
```

La herramienta usada fue xorrisofs/xorriso 1.5.6. `make-iso` exige el frontend **xorrisofs** y comprueba que su ayuda contenga `xorriso -as mkisofs`; pasar únicamente la ruta de `xorriso` no satisface esa comprobación. Esta variable admite una ruta a un ejecutable, no una cadena con argumentos. Si falta xorrisofs, debe instalarse o proporcionarse externamente antes del build.

## 3. Recompilar el programa nativo con audio

La opción real se llama **`WP1979_AUDIO=1`**. Una variable llamada sólo `AUDIO=1` no controla este Makefile.

La ruta principal reutiliza `SCSP.ELF`, `MUSIC.ELF` y `SFX.DAT` ya preparados en el staging del disco. No necesita un compilador m68k para cada recompilación SH-2.

```bash
source "$WP1979_DIR/tooling/env.sh"
export MAKE_ISO_XORRISOFS="$(command -v xorrisofs)"
make -C "$WP1979_DIR/native/src/runtime" -f Makefile.wp1979 -j4 \
  WP1979_AUDIO=1 \
  R18_ENTRY=src/wp1979_entry.c \
  R18_PROFILE_START= R18_PROFILE_STOP= R18_DIAG_RATE=
```

El Makefile compila el código nativo con el SDK, enlaza el programa, crea `IP.BIN`, coloca el programa enlazado como `A.BIN` y empaqueta el contenido actual de `audit/runtime18-cd`. Su sello de configuración controla cambios de flags y entry; las variables de perfil se dejan explícitamente vacías para producir la entrada normal.

Salidas principales, relativas a `native/src/runtime/`:

| Ruta | Contenido |
| --- | --- |
| `audit/runtime18-build/wrestlefest-wp1979.elf` | Programa enlazado con símbolos |
| `audit/runtime18-build/wrestlefest-wp1979.map` | Mapa de memoria |
| `audit/runtime18-build/wrestlefest-wp1979.bin` | Programa plano SH-2 |
| `audit/runtime18-build/IP.BIN` | Boot area producida por Yaul |
| `audit/runtime18-disc/wrestlefest-wp1979.iso` | Imagen ISO con sectores de 2048 bytes |
| `audit/runtime18-disc/wrestlefest-wp1979.cue` | CUE de esa imagen ISO |

El staging debe conservar los assets finales: `FONT8.BIN`, `FRONTSPR.BIN`, `HOTSPR.BIN`, `MUSIC.ELF`, `POSTSPR.BIN`, `R18MAP1.BIN`, `R18MAP2.BIN`, `R18MAP4.BIN`, `R18MAP5.BIN`, `R18MENU.BIN`, `R18RING.BIN`, `ROM68K.BIN`, `SCSP.ELF`, `SFX.DAT` y `SPRITES.PAK`. `A.BIN`, `ABS.TXT`, `BIB.TXT` y `CPY.TXT` se generan o actualizan durante el build. No sustituir el atlas final por una versión anterior: el atlas final tiene 6474 tiles e incluye la ruta de demostración y la prueba de control manual.

Este procedimiento reconstruye el disco a partir de los assets preparados. No constituye una receta para regenerar todos los gráficos desde cualquier ROM o workpack.

## 4. Convertir ISO a BIN/CUE y verificar

Utilizar un directorio de salida nuevo. El conversor se niega a reemplazar un BIN o CUE existente:

```bash
export WP1979_REBUILT="$WP1979_DIR/rebuild-release"
mkdir -p "$WP1979_REBUILT"
python3 "$WP1979_DIR/native/validation/iso_to_bin_exact.py" \
  "$WP1979_DIR/native/src/runtime/audit/runtime18-disc/wrestlefest-wp1979.iso" \
  "$WP1979_REBUILT" > "$WP1979_REBUILT/conversion.json"
python3 "$WP1979_DIR/native/validation/verify_disc_exact.py" \
  --root "$WP1979_DIR/native/src/runtime" \
  --makefile Makefile.wp1979 \
  --iso "$WP1979_DIR/native/src/runtime/audit/runtime18-disc/wrestlefest-wp1979.iso" \
  --bin "$WP1979_REBUILT/wrestlefest-wp1979.bin" \
  --cue "$WP1979_REBUILT/wrestlefest-wp1979.cue" \
  --json "$WP1979_REBUILT/disc-verification.json"
```

El conversor agrega cabecera de sector MODE1, EDC y ECC P/Q. El verificador debe terminar con `status: PASS`, `all_sector_edc_ecc_exact: true`, `all_cdfs_payloads_exact: true` y `a_bin_equals_linked_binary: true`. Esta verificación estructural no sustituye la ejecución en un emulador o consola.

## 5. Repetir la captura real final

El core de referencia está en:

`tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so`

Conservarlo **sin strip**: `--probe-audio` necesita sus símbolos locales. Su JSON de configuración adyacente registra las tres correcciones revisadas: activar el motor SCSP con DSP, corregir SHIFT3 y corregir la firma de `FetchVram`. El core no contiene cambios de frecuencia ni frameskip.

### Captura con identidad del programa y observaciones de RAM

Después del build y conversión anteriores:

```bash
python3 "$WP1979_DIR/measure/run_capture.py" repro-final-full \
  --output-root "$WP1979_DIR/repro-captures" \
  --cue "$WP1979_REBUILT/wrestlefest-wp1979.cue" \
  --core "$WP1979_DIR/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so" \
  --frames 8400 --video --snapshot-every 120 --wall-timeout 600 \
  --probe-audio --actors
```

Son los parámetros de la captura final completa: 8400 llamadas reales a `retro_run`, equivalentes a 140 segundos a los 60 campos/s reportados por el core. La captura de referencia terminó con 7372 pasos y draws nativos, 7376 VBlanks de combate, música en estado 2 y error 0, y ningún fatal. No hay un schedule externo en esa captura: la cinta de entrada ordinaria incluida en la demostración alimenta el juego nativo, que sigue calculando los estados y dibujos.

El wrapper copia disco, ELF/map/programa, obtiene las direcciones de símbolos del ELF recién enlazado y comprueba que `A.BIN` del disco sea exactamente ese programa. Rechaza una combinación de disco y ELF que no coincide. Requiere `measure/actors-layout.bin` si se usa `--actors`, y `native/validation/verify_disc_exact.py` junto a su módulo de conversión. El destino debe ser nuevo; para repetir, usar otro nombre de captura.

Las salidas incluyen `native-capture.mp4`, `native-video.mp4`, `native-audio.wav`, PNG nativos, dumps de RAM, `report.json`, `identity.json`, `symbols.json` y el vector exacto de argumentos `command.json`. El audio procede de los callbacks del core. El MP4 conserva una superficie de 320×240; el arranque de 320×224 recibe padding negro, sin escalar ni recortar píxeles. El tiempo de pared puede variar; aumentar sólo `--wall-timeout` en una máquina lenta no cambia los campos emulados solicitados.

### Capturar el BIN/CUE entregado sin recompilar

Para obtener vídeo/audio del disco entregado no es necesario cargar el SDK ni utilizar el wrapper. Indicar la ruta real del CUE recibido; en el árbol de trabajo la copia final está en `release-candidate-4/`:

```bash
python3 "$WP1979_DIR/tooling/capture_yabause.py" \
  --cue "$WP1979_DIR/release-candidate-4/wrestlefest-wp1979.cue" \
  --core "$WP1979_DIR/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so" \
  --out "$WP1979_DIR/repro-disc-entregado" \
  --frames 8400 --snapshot-every 120 --wall-timeout 600 \
  --dump-memory --probe-audio
```

Esta variante registra vídeo, audio y memoria, pero no genera automáticamente las observaciones de símbolos del programa que añade `run_capture.py`. Mantener BIN y CUE en el mismo directorio, con el nombre de BIN que figura dentro del CUE.

### Prueba de mando manual

La segunda captura de referencia usa el mismo disco durante 1040 campos y entrega pulsaciones por la interfaz normal del mando. Para repetirla con el wrapper:

```bash
python3 "$WP1979_DIR/measure/run_capture.py" repro-final-controller \
  --output-root "$WP1979_DIR/repro-captures" \
  --cue "$WP1979_REBUILT/wrestlefest-wp1979.cue" \
  --core "$WP1979_DIR/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so" \
  --frames 1040 --video --snapshot-every 30 --wall-timeout 240 \
  --probe-audio --actors \
  --schedule "$WP1979_DIR/render_opt/takeover/logical_schedule/controller-logical-schedule.json"
```

El script de captura actual admite ese schedule activado por el contador lógico observado. Es necesario conservar la versión actual de `capture_yabause.py`; el schedule no debe pasarse a una versión antigua que sólo admita índices de campo emulado.

## 6. Reconstruir el core desde el archivo fuente incluido

Para acompañar el `.so`, el paquete debe conservar el archivo fuente original completo `tooling/yabause-libretro.tar.gz`, el parche `tooling/yabause-scsp-dsp/WP1979-ALL-FIXES.patch`, la configuración/procedencia y esta receta. No hacen falta los objetos intermedios del core.

Con GCC/G++, GNU Make, `tar` y `patch` disponibles, reconstruir en un directorio nuevo para conservar el binario de referencia:

```bash
mkdir "$WP1979_DIR/tooling/yabause-rebuilt"
tar -xzf "$WP1979_DIR/tooling/yabause-libretro.tar.gz" \
  --strip-components=1 -C "$WP1979_DIR/tooling/yabause-rebuilt"
patch -d "$WP1979_DIR/tooling/yabause-rebuilt" -p1 \
  < "$WP1979_DIR/tooling/yabause-scsp-dsp/WP1979-ALL-FIXES.patch"
make -j4 -C "$WP1979_DIR/tooling/yabause-rebuilt/yabause/src/libretro" \
  ENABLE_CHD=0 HAVE_THREADS=0
```

El resultado está en `tooling/yabause-rebuilt/yabause/src/libretro/yabause_libretro.so` y se selecciona con `--core`. Un compilador anfitrión diferente puede producir otro hash; el hash de la tabla identifica el binario usado realmente para la entrega.

El archivo fuente conserva `yabause/COPYING` (GPL versión 2), `mini18n/COPYING`, `yabauseut/COPYING`, `yabause/src/libchdr/LICENSE.txt`, sus licencias de dependencias, `yabause/src/tsunami/LICENSE` y los avisos en las cabeceras de libretro-common y zlib. Distribuir el archivo completo evita perder estos avisos o unidades necesarias para reconstruir. Conservar también las fuentes modificadas mediante el parche y la identificación explícita de que el core fue corregido.

## 7. Regenerar opcionalmente los tres assets de audio

Esta sección es independiente de la recompilación habitual. Los assets preparados ya están en `native/src/runtime/audit/runtime18-cd/` y conservan estos hashes:

| Asset | Bytes | SHA-256 |
| --- | ---: | --- |
| `SCSP.ELF` | 53404 | `bdf84aa656a0a1b03ac6a9a6f11a7d125acbf619f6f72ebf6c53e74e3ece83b4` |
| `MUSIC.ELF` | 128316 | `eb3ac0a870a74d996eafec92eccaaa9450dbf78bc3c25dd9c824f11b270621fe` |
| `SFX.DAT` | 440710 | `dbe749cf4ce13f333ff7318a74a64e30fa2b9af05d953688a0813311cbabd216` |

### Dependencias adicionales reales

- `native/src/audio/native/` completo, incluidos `generated/`, `reference_driver/` y sus licencias.
- `tooling/audio/build_audio_linux.py`, `gcc-lb1sf68.S`, `ROM_OKI_MANIFEST.csv` y `reference/tools/audio/native_probe/`.
- `native/tools/audio/native_probe/build_sfx_pack.py`: existe en esa ruta y **no** está en `tooling/audio/reference`.
- `source_wip160/wf_audio_sfx.c`, que el generador de SFX lee como modelo de comandos; sólo este archivo es necesario de WIP160 para esta operación.
- Las ROM propias `31a11-2.ic42` y `31j10.ic73`, usadas aquí bajo `tooling/audio/rom/`. Proceden del ZIP 01 aportado por el usuario; no se descargó una ROM ni una BIOS.
- Toolchain Linux m68k. El build utilizado fue `m68k-linux-gnu-gcc` 13.3.0 de Ubuntu, con binutils m68k 2.42. `build_audio_linux.py` busca estos ejecutables explícitamente bajo `tooling/host/usr/bin/` y ajusta sus bibliotecas locales. No basta con instalar otro compilador en PATH para satisfacer ese script sin adaptar la disposición.

El árbol `host` se obtuvo extrayendo paquetes Ubuntu Noble con `dpkg-deb -x` a `tooling/host`, sin instalación global. Para reconstruirlo hacen falta paquetes externos de GCC/binutils m68k, sus ejecutables de soporte y bibliotecas. Los paquetes usados incluyen `gcc-m68k-linux-gnu`, `gcc-13-m68k-linux-gnu`, `cpp-m68k-linux-gnu`, `cpp-13-m68k-linux-gnu`, `gcc-13-m68k-linux-gnu-base`, `binutils-m68k-linux-gnu`, `libgcc-13-dev-m68k-cross` y sus dependencias. No se incluyen esos paquetes grandes en el ZIP compacto ni se promete que otra distribución reproduzca sus binarios sin ajustes.

El build recompila `mulsi3`, `udivsi3` y `divsi3` desde el archivo GCC `gcc-lb1sf68.S` con `-m68000`; no utiliza las implementaciones 68020 de la libgcc precompilada del sistema. Conservar los avisos de ese archivo y los textos `COPYING3` y `COPYING.RUNTIME` del SDK original junto a la documentación de terceros.

### Recrear sh-compat sin duplicar el SDK

`sh-compat` contiene enlaces, no otro SDK. No incluirlo en el ZIP desreferenciando enlaces: eso duplicaría aproximadamente 134 MB. El generador MUSIC espera nombres `sh-elf-*.exe`; en Linux se pueden recrear los mismos grupos de alias usados aquí:

```bash
python3 - <<'PY'
import os
from pathlib import Path
root = Path(os.environ['WP1979_DIR'])
sdk = root / 'tooling/sh2eb-elf'
compat = root / 'tooling/audio/sh-compat'
links = {'bin/sh-elf-gcc.exe': sdk / 'bin/sh2eb-elf-gcc'}
for target in (sdk / 'bin').glob('sh2eb-elf-*'):
    if target.is_file():
        links['bin/' + target.name.replace('sh2eb-elf-', 'sh-elf-', 1)] = target
for target in (sdk / 'sh2eb-elf/bin').iterdir():
    if target.is_file():
        links['sh-elf/bin/' + target.name] = target
        links['sh-elf/bin/' + target.name + '.exe'] = target
for name, target in links.items():
    if not target.is_file():
        raise FileNotFoundError(target)
    link = compat / name
    link.parent.mkdir(parents=True, exist_ok=True)
    if link.is_symlink():
        link.unlink()
    elif link.exists():
        raise FileExistsError(link)
    link.symlink_to(target.resolve())
PY
```

Después de disponer de esas dependencias:

```bash
source "$WP1979_DIR/tooling/env.sh"
python3 "$WP1979_DIR/tooling/audio/build_audio_linux.py"
python3 "$WP1979_DIR/tooling/audio/reference/tools/audio/native_probe/build_music_module.py" \
  --repo "$WP1979_DIR/native" \
  --toolchain "$WP1979_DIR/tooling/audio/sh-compat" \
  --sdk "$WP1979_DIR/tooling/sh2eb-elf" \
  --rom "$WP1979_DIR/tooling/audio/rom/31a11-2.ic42" \
  --out "$WP1979_DIR/tooling/audio/music-module"
python3 "$WP1979_DIR/native/tools/audio/native_probe/build_sfx_pack.py" \
  --rom "$WP1979_DIR/tooling/audio/rom/31j10.ic73" \
  --manifest "$WP1979_DIR/tooling/audio/ROM_OKI_MANIFEST.csv" \
  --model "$WP1979_DIR/source_wip160/wf_audio_sfx.c" \
  --out "$WP1979_DIR/tooling/audio/pack"
cp "$WP1979_DIR/tooling/audio/music-driver/SCSP.ELF" "$WP1979_DIR/native/src/runtime/audit/runtime18-cd/SCSP.ELF"
cp "$WP1979_DIR/tooling/audio/music-module/MUSIC.ELF" "$WP1979_DIR/native/src/runtime/audit/runtime18-cd/MUSIC.ELF"
cp "$WP1979_DIR/tooling/audio/pack/SFX.DAT" "$WP1979_DIR/native/src/runtime/audit/runtime18-cd/SFX.DAT"
```

Recompilar/empaquetar el disco después de cambiar estos assets. Los scripts generan los informes de compilación y hashes en sus directorios de salida. `build_runtime18.py` y las rutas antiguas de las herramientas Windows no son la receta Linux empleada para esta entrega.

## 8. Contenido imprescindible del paquete

| Grupo | Archivos/directorios que hay que conservar |
| --- | --- |
| Disco y evidencia final | BIN/CUE final, sus hashes y verificación, vídeo/audio final y los informes elegidos para la entrega |
| Código SH-2 | `native/src/runtime/src/`, `include/`, `generated/`, `Makefile.wp1979`, `tools/wp1979_config_stamp.py`; conservar también los demás tools/tests si se entrega el árbol fuente completo |
| Audio nativo enlazado | `native/src/audio/native/` completo, con subdirectorios y licencias |
| Inputs del disco | `native/src/runtime/audit/runtime18-cd/` con los assets finales enumerados; no depende de los directorios históricos de auditoría |
| Conversión/verificación | `native/validation/iso_to_bin_exact.py` y `verify_disc_exact.py` |
| Captura reproducible | `tooling/env.sh`, `tooling/capture_yabause.py`, `measure/run_capture.py`, `measure/actors-layout.bin`; schedule manual si se ofrece esa prueba |
| Core distribuido | `.so` exacto sin strip, su `.configuration.json`, `WP1979-ALL-FIXES.patch`, `WP1979-SCSP-CONFIGURATION.json` y el archivo fuente completo `yabause-libretro.tar.gz`, que conserva sus licencias |
| Regeneración de audio | Scripts/ROM propias/modelo/manifiesto de la sección 7; no omitir `native/tools/audio/native_probe/build_sfx_pack.py` |
| Avisos de terceros | Licencia de libyaul; licencias/procedencia ya incluidas en audio; textos GPL/Runtime Exception del helper GCC; licencias incluidas en el archivo fuente de Yabause |
| Reproducir observaciones sin enlazar otra vez | ELF/map/programa plano finales; el wrapper espera esos archivos en `native/src/runtime/audit/runtime18-build/`. Para verificar el disco sin rebuild también hace falta su `IP.BIN` correspondiente |

### Dependencias y material que quedan fuera del paquete compacto

- SDK14 ya aportado por el usuario: aproximadamente **199 MB** extraído, **92.2 MB** su ZIP original. Se restaura en `tooling/sh2eb-elf/`.
- `tooling/host/`: aproximadamente **234 MB**, contiene los paquetes locales de xorrisofs, m68k y otros emuladores/herramientas. No es una dependencia íntegra del juego; el build principal requiere xorrisofs externo y el audio opcional requiere la disposición m68k descrita.
- `tooling/apt/`: aproximadamente **176 MB** de paquetes descargados y metadatos; no es fuente del juego.
- `tooling/audio/sh-compat/`: recrear los enlaces; no copiar sus destinos.
- Objetos intermedios, caches, capturas de candidatos anteriores, `perf_probe`, `backend_opt`, smokes, MAME/Mednafen/Xvfb y los árboles duplicados `reference_r18`/auditorías no son necesarios para recompilar el candidato final con sus assets preparados.
- El archivo fuente original de Yabause ocupa aproximadamente **15 MB** comprimido y sí acompaña al core. Puede omitirse el árbol de trabajo del core de aproximadamente **48 MB**, pues se reconstruye desde ese archivo más el parche. No omitir sus fuentes/licencias al distribuir el `.so`.

## 9. Alcance de la evidencia

La captura ejecuta el programa nativo desde el disco mediante Yabause HLE, intérprete SH-2 y renderer software. No utiliza una BIOS propietaria, savestate ni sustitución de fotogramas. HLE no garantiza compatibilidad idéntica al firmware original. No se ha presentado esta ejecución como una prueba en Saturn física.

El core procesa VDP1 de forma síncrona y no modela un presupuesto temporal completo de raster. La cadencia observada sirve para CPU, lógica y sincronización dentro de ese backend; no demuestra el fill-rate físico de VDP1. Las correcciones del core están identificadas y no modifican relojes ni frameskip. La validación definitiva de vídeo, CD y sonido en una consola real requiere ejecutar el BIN/CUE entregado en ese hardware.
