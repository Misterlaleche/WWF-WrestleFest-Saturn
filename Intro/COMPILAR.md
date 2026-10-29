# Fabricar la intro WP0 R04

Todos los comandos de esta guía se ejecutan desde `Intro/`, después de clonar el repositorio. Utilizar una copia nueva y rutas de trabajo sin espacios.

## 1. Recuperar los recursos externos

Descargar [WF-WP0-R04-BIN-CUE-Y-FUENTES.zip](https://chatgpt.com/library?file_id=libfile_51aacfacc41c8191a67f2943b27d5678&account_id=personal), de 12.869.253 bytes. Este enlace requiere acceso al archivo en ChatGPT. Proporcionar el ZIP al ordenador o entorno de Codex donde se vaya a compilar.

SHA-256 esperado:

```text
a6ef24b69de89abc25d4882829b36d7497beb1f0464318facc527f413ecde5d7
```

```sh
python3 tools/restore_assets.py /ruta/WF-WP0-R04-BIN-CUE-Y-FUENTES.zip
python3 tools/restore_assets.py --check
```

El script usa solo la biblioteca estándar de Python. Verifica el ZIP completo y los ocho recursos; restaura las seis entradas necesarias de `source/cd/`, el fixture gráfico y el PCM de referencia de las pruebas. No extrae rutas arbitrarias ni sobrescribe archivos con contenido distinto. Si encuentra una diferencia, se detiene e identifica el archivo: trabaja sobre otra copia nueva o revisa el cambio antes de continuar.

La comprobación correcta termina con `result: PASS` y `verified: 8`. La segunda ejecución de la restauración reutiliza los recursos idénticos. `A.BIN` no se restaura: lo genera el compilador.

## 2. Instalar las dependencias

| Operación | Requisitos |
|---|---|
| Compilación Saturn | Linux x86_64, Bash, GNU Make y el SDK Yaul completo suministrado al proyecto, con GCC SH-2 13.2.0. |
| Creación de ISO | `xorrisofs`; la referencia utiliza 1.5.6. |
| Conversión a BIN/CUE | Python 3; biblioteca estándar. |
| Pruebas host y diferencial Rice | Python 3 y compilador C11 del anfitrión. |
| Revisión ASan/UBSan | Compilador C del anfitrión con esos sanitizadores. |
| Pruebas gráficas opcionales | Además, NumPy, Pillow y la ROM original. |

El [SDK suministrado](https://chatgpt.com/library?file_id=libfile_0f83f2401a3c8191bd0c58679d97ea44&account_id=personal) se llama `sh2eb-elf-yaul(1).zip`; su copia de trabajo en el proyecto es `14-sh2eb-elf-yaul-1-.zip`. Requiere acceso a ese archivo. No forma parte de este checkout ni del ZIP R04.

Identidad del ZIP del SDK: 92.217.495 bytes; SHA-256 `8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`.

Un compilador SH-2 aislado no basta: hacen falta las bibliotecas, headers, scripts `share/` y herramientas `make-ip`, `make-iso` y `make-cue` del mismo SDK. `YAUL_INSTALL_ROOT` debe contener `bin/sh2eb-elf-gcc` y `share/build.pre.mk`, y su ruta debe estar libre de espacios.

Si al extraer el SDK se pierden los permisos ejecutables, restaurarlos en los ejecutables de su copia de `bin/` y `libexec/`. Si `xorrisofs` se instala fuera del sistema, sus bibliotecas dinámicas deben estar disponibles para el cargador.

BIOS y emulador no intervienen en la compilación. Con los recursos restaurados no hace falta volver a capturar la ROM en MAME.

## 3. Crear el BIN/CUE

Sustituir las rutas de ejemplo por las reales:

```sh
export YAUL_INSTALL_ROOT=/ruta/absoluta/al/sh2eb-elf
export MAKE_ISO_XORRISOFS=/ruta/absoluta/a/xorrisofs
export PATH="$YAUL_INSTALL_ROOT/bin:$PATH"

bash source/build_disc.sh /ruta/absoluta/nueva/disco-r04
```

`build_disc.sh` configura los prefijos SH-2/M68K y el asignador TLSF. Produce:

- `source/build/WF-WP0-R04.elf`.
- `source/cd/A.BIN`, ejecutable Saturn.
- `source/WF-WP0-R04.iso`.
- `WF-WP0-R04.bin`, `WF-WP0-R04.cue` y `disc-validation.json` en la carpeta de salida indicada.

La ruta de salida debe ser nueva: el conversor rechaza sobrescribir un BIN/CUE existente. La imagen para MODE o grabación es el BIN/CUE final de sectores de 2.352 bytes; `source/build/WF-WP0-R04.bin` es un ejecutable intermedio.

El programa compila nueve unidades: `main.c`, `wp0_cd_transfer32.c`, `wp0_rice.c`, `wp0_audio_player.c`, `wp0_scsp.c`, `wp0_framebuffer.c`, `wp0_error_screen.c`, `wp0_sequence_cache.c` y `wp0_timer.c`.

El Makefile heredado no usa las fechas de todos los recursos de `cd/` como dependencias de la ISO. Para una reconstrucción tras cambios de recursos, usar otra copia limpia de las fuentes, restaurar los datos y compilar allí. Cambiar únicamente los datos y reutilizar una ISO anterior no verifica la nueva entrada.

## 4. Comprobar el resultado

El ejecutable de referencia `source/cd/A.BIN` ocupa 874.100 bytes y tiene este SHA-256:

```text
32aa415f594f3860798ac942e936f322c2d53d1d14cf96762e1fe7d8694bb670
```

```sh
sha256sum source/cd/A.BIN
```

La reconstrucción con el SDK de referencia debe coincidir con ese ejecutable. El ELF puede variar por las rutas de depuración y la ISO por sus fechas; no se promete un hash de disco universal para otros entornos. El conversor comprueba todos los sectores, el contenido ISO y EDC/ECC, y registra el resultado en `disc-validation.json`.

La entrega original identifica un BIN de 4.769.856 bytes, 2.028 sectores MODE1/2352:

```text
eb9faea2016b3f433d2e2363ce51bdacc6c25911ff56c94a9a71db730dd0d63c
```

Durante la entrega original se detectó una anomalía del entorno al persistir escrituras incrementales del BIN. El procedimiento usado aquí genera el disco en un directorio privado de `/tmp` y vuelve a leerlo desde otro proceso. Si tu entorno de trabajo presenta ese problema, conserva esa ruta de salida y copia después el archivo completo; no trunques sectores finales de relleno.

## 5. Pruebas incluidas

Después de restaurar los ocho recursos:

```sh
python3 audit/host/run_host_tests.py
python3 audit/review_final/run_guard_review.py
```

El primer runner comprueba codec, anillo de audio, observación SCSP, controlador de framebuffer y caché de secuencia. Imprime su informe JSON; para guardarlo hay que redirigir su salida a un archivo nuevo. El runner ASan/UBSan regenera su corpus en un temporal, comprueba su hash y compara los resultados con `final_review_evidence.json`. LeakSanitizer queda desactivado; AddressSanitizer y UndefinedBehaviorSanitizer siguen activos.

Contraste adicional del decoder, con salida nueva fuera del checkout:

```sh
mkdir -p /ruta/nueva
python3 audit/audio/verify_decoder_equivalence.py \
  --baseline audit/reference/r03_decoder/wp0_rice.c \
  --candidate source/wp0_rice.c \
  --packed source/cd/TITLE02.RIC \
  --pcm audit/audio/TITLE02.PCM \
  --output /ruta/nueva/decoder-r04.json
```

Las pruebas gráficas opcionales admiten `WP0_ROM` como ruta a la ROM original:

```sh
export WP0_ROM=/ruta/wwfwfest.zip
python3 -m unittest discover -s source/tests -p 'test_*graphics.py'
python3 -m unittest discover -s source/tests -p 'test_native_assets.py'
```

`docs/original/REPRODUCIR.txt` conserva los comandos de auditoría de la entrega completa. Sus apartados de emulación y perfiles necesitan los archivos adicionales del ZIP original, además de BIOS y core externos. Esos apartados no describen un entorno de emulación completo incluido en GitHub.

## 6. Usar en Saturn

Copiar el BIN y su CUE juntos a una carpeta nueva de MODE y seleccionar la imagen. Mantener el nombre del BIN que declara el CUE. Objetivo: Saturn NTSC sin ampliación de RAM; no hay adaptación específica de cadencia para PAL.

Start permite repetir tras el final. A+B+C+Start permite reiniciar durante la secuencia. La preparación inicial y la del reinicio se realizan con la imagen oculta.

Ale confirmó «Intro ok.» después de probar R04. Esa observación corresponde a la intro independiente; cualquier integración con otras escenas debe volver a comprobarse.
