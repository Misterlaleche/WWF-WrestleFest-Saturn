# Compilar P1-03B

[English](BUILDING.md) · [Presentación del proyecto](../README.es.md)

Estas instrucciones describen las fuentes actuales y sustituyen la recuperación exclusiva de R03 heredada por los ZIP posteriores. No afirman que durante esta renovación se haya realizado una nueva compilación SH-2 ni una validación física.

## 1. Entradas necesarias

Utiliza GNU/Linux x86-64, GNU Make 4.2 o posterior, Python 3, compilador C/C++ de host y `genisoimage` o un `mkisofs` compatible. El compilador de destino documentado es GCC SH-2 8.2 del archivo fijado de Jo Engine 2026.3. El ejecutable Linux no funciona directamente como aplicación nativa de macOS. El makefile tiene rutas para otras plataformas, pero esta actualización no las revalida.

| Entrada externa | Identidad / requisito |
| --- | --- |
| `joengine-2026.3.zip` | SHA-256 `341209b0216f2e2247f26946a192cfe295d69a38df99c6639c86ee524eed5383` |
| Recursos preparados | Los 34 archivos deben coincidir en nombre, tamaño y SHA-256 con [`asset-lock.json`](../asset-lock.json) |
| Origen para recuperar recursos | BIN o ISO de datos P1-03/P1-03B compatible; el extractor comprueba cada archivo requerido |

El SDK registrado ocupa 141.364.905 bytes. Otro SDK más reciente o empaquetado de forma distinta no es la misma entrada aunque conserve el nombre. Los ejecutables del compilador, el disco compatible y los recursos del juego deben aportarse por separado.

Las músicas de R03 y FIX2R **no** cumplen los hashes actuales de P1-03. No sustituyas el manifiesto para aceptar un disco antiguo. `source_assets/` no se incluye: la ROM original por sí sola no basta para regenerar todos los recursos preparados actuales desde este repositorio.

## 2. Obtener las fuentes e instalar el compilador

```sh
git clone https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private.git
cd WWF-WrestleFest-Saturn-Private
python3 tools/setup_compiler.py /ruta/a/joengine-2026.3.zip
```

Mientras el repositorio sea privado se necesita acceso. El instalador verifica el ZIP completo y extrae `sdk/Compiler/LINUX/` y `sdk/Compiler/WINDOWS/`. Rechaza sobrescribir directorios de compilador existentes.

Conserva los directorios versionados `sdk/jo_engine/` y `sdk/Compiler/COMMON/` del proyecto. COMMON contiene las reglas, el linker adaptado y la plantilla Jo `IP.BIN` de 6.144 bytes, que no es una BIOS Saturn. Su recuperación desde el paquete R03 verificado se documenta en [PROVENANCE.md](PROVENANCE.md).

## 3. Recuperar y verificar los recursos

Desde la raíz del repositorio, sustituye la ruta del ejemplo por tu disco compatible:

```sh
python3 tools/extract_disc_assets.py /ruta/a/WrestleFest_Saturn_P1_03B_BOOTFIX.bin --output-dir cd
python3 tools/check_raw_disc_assets.py --cd cd --manifest asset-lock.json
python3 tools/generate_music_headers.py --verify --cd cd
```

El extractor acepta ISO2048 y una pista de datos MODE1/2352 iniciada en LBA 0. Verifica las entradas elegidas contra el manifiesto antes de reemplazar archivos de destino. No importa `0.BIN`: el ejecutable se compila desde las fuentes. No se necesita BIOS para extraer ni compilar.

También puedes colocar los recursos preparados correctos en `cd/` y ejecutar las dos últimas comprobaciones. Los 34 archivos suman **632.644.966 bytes**. Reserva espacio adicional para ISO, BIN de sectores raw y compilador.

Las cabeceras WAV y los metadatos musicales corresponden a esos recursos exactos. Verifícalos; no regeneres metadatos con otra banda sonora antigua para ocultar una incompatibilidad.

## 4. Compilar el frontend normal

```sh
make -j2 WF_FRONTEND=1 WF_AUTORUN= WF_CAPTURE=0 WF_PROFILE=0 MKISOFS=genisoimage
make sh2-memory-margin WF_FRONTEND=1 WF_AUTORUN= WF_CAPTURE=0 WF_PROFILE=0 MKISOFS=genisoimage
```

Usa las mismas opciones en compilación y comprobación de memoria. `WF_AUTORUN=` desactiva las entradas automáticas de prueba. `WF_CAPTURE=0` y `WF_PROFILE=0` desactivan captura y perfilado. Omitir `WF_FRONTEND=1` selecciona una entrada de diagnóstico en lugar del recorrido original.

Los resultados previstos son `game.elf`, `game.raw`, `game.map`, `game.iso`, `game.bin` y `game.cue`. El empaquetado comprueba recursos y cabeceras musicales. La puerta separada de memoria exige al menos **8.192 bytes** antes de la reserva `0x060DC000`. La compilación recuperada de P1-03B registraba 8.208 bytes, por lo que incluso cambios pequeños pueden afectar al margen.

Conserva estas condiciones:

- `-fno-peephole2` en compilación y enlace, por el problema documentado de generación de código del GCC SH-2.
- El linker adaptado y las reservas de heap, pila y memoria de audio.
- La selección de objetos fríos con `-Os`; **Adler-32 debe permanecer fuera de esa lista** en esta candidata.
- La configuración LTO y los contratos de doble búfer y propiedad del vídeo.

La versión y fecha antiguas de la cabecera del disco son distintas de la identidad del checkpoint P1-03B. [PROVENANCE.md](PROVENANCE.md) identifica el ejecutable original. Las fechas del empaquetado también pueden cambiar el hash ISO/BIN; compilar correctamente no demuestra por sí solo una reproducción idéntica del disco.

## 5. Pruebas y hardware

Varias pruebas de host funcionan sin recursos del juego:

```sh
python3 tests/test_boot_asset_build_policy.py
python3 tests/test_jo_controls.py --sanitize
python3 tests/test_music_stream.py
python3 tests/test_music_deadline.py
python3 tests/test_save.py
python3 tests/test_save_adapter.py
```

La suite más amplia necesita `cd/` restaurado:

```sh
python3 tests/run_initial_load_tests.py --source . --assets cd --output build/initial-load-tests
```

Ese ejecutor nació en R03. Su resultado solo cubre las pruebas que realmente ejecuta; no sustituye las regresiones P0/P1 posteriores, las comparaciones completas de campaña, las pruebas de emulador ni la aceptación física. Los oráculos de ROM necesitan además `machine68k==0.5.1` y los originales correspondientes.

Para probar en Saturn, coloca BIN y CUE juntos, abre la CUE en MODE y utiliza la expansión de RAM de 4 MB. Registra primero el resultado del arranque. [STATUS.md](STATUS.md) y [CONTRIBUTING.md](../CONTRIBUTING.md) detallan la aceptación pendiente y cómo comunicar pruebas útiles.

## Generación opcional y herramientas históricas

Los generadores se conservan para desarrollo. Los gráficos tienen una dependencia fijada en [`requirements-assets.txt`](../requirements-assets.txt); el audio usa la referencia Z80/YM/OKI de host y FFmpeg con libsoxr. Ambos necesitan entradas originales adicionales que no están en el repositorio. Las herramientas Z80/YM se ejecutan en el ordenador; no son el motor musical actual de Saturn.

`tools/verify_checkpoint.py` comprueba manifiestos de paquetes congelados del formato R03 original. Su `source-manifest.json` histórico no describe este checkout renovado. Para identificar estas fuentes utiliza el commit de Git, la [procedencia](PROVENANCE.md) y el registro de importación incluido.
