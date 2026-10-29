# Compilar el fixture WP1979

Las fuentes corresponden a la imagen entregada y comprobada en emulador.
Esta receta conserva el build original; no fusiona el bloque en el juego maestro.

## 1. Restaurar recursos y preparar una ruta sin espacios

Desde `Combate real en Saturn/`, con el ZIP exacto indicado en el README:

```bash
python3 tools/restore_resources.py /ruta/wp1979-fuentes-pruebas.zip --scope build
python3 tools/restore_resources.py --scope build --verify
```

Elegir un destino **nuevo, sin espacios**, para no sobrescribir trabajo previo:

```bash
WP1979_DIR="$(mktemp -d /tmp/wp1979-combate.XXXXXX)" &&
cp -a wp1979/. "$WP1979_DIR/" &&
cd "$WP1979_DIR" &&
export WP1979_DIR
```

Para las pruebas host, el audio regenerado o la captura completa, usar
`--scope all` antes de copiar. Recupera todas las dependencias del paquete
auditado, conservando el mismo diseño de directorios.

## 2. SDK y dependencias

Se utilizó Linux x86-64, Bash, GNU Make, Python 3, SDK SH-2/Yaul suministrado
(GCC 13.2.0) y xorrisofs/xorriso 1.5.6. No se ha validado esta receta en otro
sistema operativo o arquitectura. El restaurador de este módulo requiere
**Python 3.11 o posterior**.

Archivo del proyecto: `14-sh2eb-elf-yaul-1-.zip`, SHA-256
`8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`.

```bash
unzip /ruta/14-sh2eb-elf-yaul-1-.zip -d "$WP1979_DIR/tooling"
source "$WP1979_DIR/tooling/env.sh"
export MAKE_ISO_XORRISOFS="$(command -v xorrisofs)"
test -x "$YAUL_INSTALL_ROOT/bin/sh2eb-elf-gcc"
test -x "$MAKE_ISO_XORRISOFS"
```

`tooling/env.sh` conserva la configuración usada en la entrega. Su ruta
predeterminada a `tooling/host/usr/bin/xorrisofs` no existe en este paquete
compacto; por eso se sustituye por el ejecutable instalado. El SDK y los paquetes
del sistema anfitrión no están incluidos en Git ni en el ZIP de fuentes.

## 3. Compilar con el audio final

```bash
make -C "$WP1979_DIR/native/src/runtime" -f Makefile.wp1979 -j4 \
  WP1979_AUDIO=1 \
  R18_ENTRY=src/wp1979_entry.c \
  R18_PROFILE_START= R18_PROFILE_STOP= R18_DIAG_RATE=
```

La opción es `WP1979_AUDIO=1`. Este build reutiliza `SCSP.ELF`, `MUSIC.ELF`
y `SFX.DAT` restaurados, por lo que no necesita recompilar el firmware m68k.
La regeneración opcional de audio está explicada en
[REPRODUCIR-ENTREGA.md](wp1979/tooling/REPRODUCIR-ENTREGA.md).

Salidas dentro de `native/src/runtime/`:

- `audit/runtime18-build/wrestlefest-wp1979.elf`, `.map` y `.bin`.
- `audit/runtime18-build/IP.BIN`.
- `audit/runtime18-disc/wrestlefest-wp1979.iso`.

El `.bin` del directorio de build es el programa SH-2; no es el BIN completo de CD.

## 4. Fabricar y verificar BIN/CUE

Elegir un directorio de salida nuevo:

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

El verificador exige igualdad de boot, sectores EDC/ECC, contenido CDFS y
`A.BIN` con el ejecutable enlazado. Fechas de empaquetado o compiladores distintos
pueden cambiar el hash del disco recompilado; conservar su nueva identidad.
Una validación estructural no sustituye la ejecución en consola física.

## 5. Repetir captura y preparar integración

La receta extensa contiene comandos exactos para el vídeo de 8.400 campos,
la toma de control de 1.040, la compilación del core con sus tres correcciones y
el análisis de cadencia/audio. Requiere `--scope all`, Pillow, FFmpeg y `nm`.

La guía [INTEGRACION-CODEX.md](wp1979/github_handoff/INTEGRACION-CODEX.md) explica
qué partes del fixture deben reemplazarse por el flujo real del juego receptor.
El estado de este bloque permanece **CANDIDATE / HARDWARE-PENDING**.
