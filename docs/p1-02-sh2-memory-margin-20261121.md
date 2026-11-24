# P1-02 — margen estático SH-2

## Problema

FIX2R enlazaba con `_end = 0x060DBFC4`, inmediatamente antes de `JO_WORK` en
`0x060DC000`: solo 60 bytes libres. Cualquier instrumentación o reparación
posterior podía invadir la reserva contigua de música.

## Cambio

Solo los módulos de carga, frontend, configuración de combate, postcombate,
ranking, guardado y zlib se compilan con `-Os`. El bucle activo de gameplay,
render, vídeo y audio conserva `-O2`. No se reducen cachés, actores, órdenes
VDP1, tiles, paletas, muestras ni la reserva musical.

Se añade `tests/test_sh2_memory_margin.py` y el objetivo
`make sh2-memory-margin`. La prueba exige al menos 8192 bytes.

## TDD

- RED sobre FIX2R: `_end=0x060DBFC4`, margen 60, faltan 8132 bytes.
- GREEN: `_end=0x060D9CB0`, margen 9040 bytes.

El ejecutable binario baja de 537696 a 528704 bytes. La reducción efectiva de
`_end` es 8980 bytes.

## Binarios de verificación

- `game.raw` SHA-256: `5c484e6badfd77928d71aa56581f5e260797c4a04eefd3c83723df5b27e177e5`
- `game.elf` SHA-256: `aa52817d9440e9424a7f7f687500a0fa20e7434c9c5d4de757e3a216b0f15434`
- `game.map` SHA-256: `dc4e1621d077d4f54ab7cdd397688ca6de25fb6ab736ff861852d1993641e152`

## Regresiones ejecutadas

- P0-01: 12/12 grupos.
- P0-02: 777/777 con GCC y Clang; 17/17 pruebas del intérprete.
- P0-03: 5589/5589 con GCC y Clang; 14/14 pruebas del intérprete.
- P0-04: 3411/3411 con GCC, Clang y sin traza; 5/5 pruebas auxiliares.
- P0-05: 4447/4447 con GCC, Clang y sin traza; 3/3 pruebas auxiliares.
- P0-06: 3435/3435 con GCC, Clang y sin traza.
- GFX1: PASS.
- Jo frontend TAG y Royal: PASS.
- ringout/frontend, coin-run y protocolo CD: PASS.
- campaña TAG combate 1 -> combate 2: PASS, `failed=0`.
- audio service, decoder, combat audio, stream y deadline: PASS.

La suite agregada original puede detenerse de forma intermitente dentro de su
ejecutor de mutantes; sus 19 órdenes se ejecutaron también de forma acotada e
independiente. No se presenta este bloque como validación en Saturn física.
