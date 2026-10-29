# Escena Presentadora — WP02 R02

Fuente de la presentación TAG de WrestleFest para Sega Saturn. **R02 fue
aceptada por el usuario el 29 de octubre de 2026.** Conserva el controlador
recuperado de la escena, los nombres, las poses, las voces y la salida de la
presentadora. La demostración termina al desaparecer la presentadora y
permite repetirla con START.

Esta carpeta conserva las fuentes y sus rutas del paquete aceptado. La
validación registrada corresponde a Mednafen 1.32.1 y a pruebas de PC; la
aceptación no añade una medición física de Saturn que no esté documentada.
El resumen de resultados está en [FINAL-VALIDATION.json](evidence/FINAL-VALIDATION.json).
`evidence/` conserva las auditorías seleccionadas; `docs/` conserva el LEEME
y el manifiesto SHA-256 originales como documentos históricos del paquete.

## Recursos externos

Los recursos binarios se restauran localmente desde el
[ZIP R02 aceptado](https://chatgpt.com/library?file_id=libfile_9090d6e523f08191a8b046188c6f0aab&account_id=personal),
siguiendo [ASSETS-MANIFEST.json](ASSETS-MANIFEST.json). El repositorio mantiene
fuentes, pruebas y procedencia; el ZIP conserva las cargas gráficas y de audio.

SHA-256 del ZIP:

```text
69673d389d872af5d66e37670c228f1b6ab72e7d737066ecc2fde28ad90f4730
```

El restaurador verifica el ZIP y cada recurso. `source/scene/cd` necesita
`TEX.BIN`, `RING.BIN`, `CRAM.BIN`, `SDRV.BIN`, `WP02PCM.PAK` y
`WP02BGM.PAK`. Las pruebas de audio también necesitan
`WP02BGM.decoded.pcm`, conservado como referencia independiente. El manifiesto
incluye las copias de estos recursos usadas por las carpetas de investigación.
`A.BIN` se genera al compilar.

## Reconstrucción y pruebas

Requisitos: Linux/POSIX, Python 3.9 o posterior, compilador C compatible con C11, SDK
Yaul/sh2eb-elf de la entrega —GCC 13.2— y `genisoimage`. La compilación SH-2
usa `YAUL_INSTALL_ROOT`; `WP02_MKISOFS` permite indicar la ruta de
`genisoimage` cuando no está en PATH. Las dieciséis pruebas host de audio
pueden ejecutarse sin el SDK.

El `make-ip` del SDK suministrado no admite espacios en su ruta de salida.
Desde esta carpeta, prepara una copia de trabajo con una ruta sin espacios:

```sh
WP02_BUILD_DIR="$(mktemp -d /tmp/wp02_presentadora.XXXXXX)"
cp -a . "$WP02_BUILD_DIR/"
cd "$WP02_BUILD_DIR"

python3 tools/restore_assets.py /ruta/WP02-Saturn-TAG-R02.zip
python3 tools/restore_assets.py --verify
python3 source/verification/r02_audio/run_tests.py

export YAUL_INSTALL_ROOT=/ruta/sin_espacios/sh2eb-elf
python3 source/scene/build.py
python3 source/tools/iso_to_bin.py source/scene/build/WP02-TAG-R02.iso rebuilt-disc
```

El ejecutable de la entrega aceptada tiene **967.476 bytes** y este SHA-256:

```text
1a3566c451fb195c3e2973fa3adece54b604d5bb9913a78a83f88ed748f2c776
```

`python3 source/scene/build.py --reference` construye la referencia fija
`[0,6,9,1]`, semilla `0xCEB7C58E`, para las comparaciones documentadas.

**Selección de fuentes durante el build:** si existen las carpetas
`research_rom`, `research_render/generated` y `research_audio/integration`,
`build.py` copia sus módulos y recursos a `scene/src` y `scene/cd` antes de
compilar. Conserva completos esos conjuntos al trabajar con esta estructura.
Una modificación aislada de sus duplicados en `scene/src` sería sobrescrita.
`main.c` y `wp02_diag.h` se mantienen directamente en `scene/src`.

La extracción desde ROM y las comparaciones originales contra MAME son
herramientas de investigación con dependencias adicionales. La reconstrucción
del disco y la suite host R02 utilizan los recursos restaurados del paquete.
Para repetir los análisis históricos de vídeo/audio/MAME, usa el árbol
completo del ZIP y sus materiales de entrada: algunos scripts conservan
rutas históricas de auditorías y capturas, fuera del recorrido de build/tests.

## Integración en el juego

Consulta [INTEGRACION_CODEX.md](INTEGRACION_CODEX.md) antes de enlazar la escena
con el runtime: documenta los IDs y el RNG de entrada, la fase musical, el
propietario del audio/reloj, las reservas de memoria y el ciclo de cierre.

## Comprobación de esta publicación

La restauración de los 273 recursos recuperó las 375 entradas conservadas
del árbol fuente original con sus bytes exactos. Una reconstrucción SH-2
desde una copia sin espacios produjo el mismo `A.BIN` de 967.476 bytes, y
los 16 casos host pasaron. El registro específico de esta subida está en
[github-upload-checks.json](evidence/github-upload-checks.json).

Los resultados históricos de imágenes, temporización y audio están en
`evidence/`. Las medidas registradas son de emulador y de pruebas de PC;
no añaden una medición física de Saturn.
