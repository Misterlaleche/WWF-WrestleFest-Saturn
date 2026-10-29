# WP1979 — Integración de «Combate real en Saturn»

**Estado: `HARDWARE-PENDING`.** Implementación validada en las rutas de emulador documentadas; integración en el juego receptor y prueba física pendientes.

## Punto de partida

Esta entrega conserva una implementación nativa SH-2 de combate TAG, con vídeo VDP1/VDP2 y audio nativo. El fixture final enfrenta a **Hulk Hogan + Ultimate Warrior contra Ted DiBiase + Mr. Perfect**. La integración en el frontend y la campaña del juego receptor todavía debe realizarse.

La base de comparación es `Misterlaleche/WWF-WrestleFest-Saturn-Private`, rama de referencia `runtime18/avances-20261028`, commit **`d95b8c37f62402bce4dead13cf7c0567b8fc2439`**. Los cambios finales están en `native/`; la referencia original permanece intacta.

En GitHub, este documento está previsto en `Combate real en Saturn/wp1979/github_handoff/INTEGRACION-CODEX.md`. Salvo indicación contraria, las rutas siguientes son relativas al directorio `wp1979/` que lo contiene. Conservar esas subrutas al trasladar el paquete. El SDK no admite espacios: antes de compilar, copiar/restaurar el subárbol `wp1979/` en una ruta como **`/tmp/wp1979-combate/`**.

**Identidad del disco final:** SHA-256 del BIN completo, MODE1/2352: `f1232edb31d6e3748b60001d8d82bca5f9b6171d16aaf812f618f3dbf858da5e`.

El programa plano SH-2 también se llama `wrestlefest-wp1979.bin`, pero es otro archivo: su SHA-256, igual al de `A.BIN`, es `49b03fc81c6801eb376979079f96746f1bbc887346b29c90d55f1003bad2720e`. No confundir programa y disco.

## 1. Archivos que deben acompañar la integración

En la tabla, `src/`, `include/` y `generated/` sin otro prefijo son subdirectorios de `native/src/runtime/`.

| Grupo | Rutas y función |
| --- | --- |
| Runtime final | `native/src/runtime/src/`, `include/` y `generated/`. Llevar las tres partes juntas; las tablas generadas forman parte del comportamiento. |
| Construcción de referencia | `native/src/runtime/Makefile.wp1979` y `native/src/runtime/tools/wp1979_config_stamp.py`. El sello evita reutilizar objetos con flags o entrada distintos. |
| Entrada y prueba reproducible | `src/wp1979_entry.c`, `src/wp1979_input.c`, `src/wp1979_tape.inc` y `include/wp1979_input.h`, dentro del runtime. Documentan el arranque directo, la cinta de niveles de mando y su cancelación. |
| Correcciones de lógica | Revisar especialmente `src/wf_cpu_runtime.c` y `src/wf_r17_human_raw23.c`: selección/solicitud TAG de CPU, flujo del estado 6 y efectos de registros del RNG humano. Conservar la guarda original `1C094`. |
| Presentación y memoria | `src/wf_r18_ring.c`, `src/wf_r18_present.c`, `src/wf_saturn_yaul_backend.c`, `src/wf_vdp1_plan.c`, `include/wf_saturn_memory.h`, `include/wf_saturn_yaul_backend.h` e `include/wf_r18_rom_hot_pages.h`. |
| CD | `src/wf_r18_cdfs.c`, `src/wp1979_cd_read.c` e `include/wp1979_cd_read.h`: lector independiente con acceso CPU de 32 bits al FIFO. No requiere modificar el SDK. |
| Audio | `src/wf_r18_scsp.c` y `native/src/audio/native/` completo, incluidos subdirectorios, cabeceras, procedencia y licencias. |
| Assets finales | `native/src/runtime/audit/runtime18-cd/`, con los archivos enumerados debajo. |
| Revisión y reproducción | `release/wp1979-independent.patch`, [informe final](../release/wp1979-informe.txt), [receta de reproducción](../tooling/REPRODUCIR-ENTREGA.md) y `native/validation/{iso_to_bin_exact.py,verify_disc_exact.py}`. |

Assets preparados que hay que conservar: `FONT8.BIN`, `FRONTSPR.BIN`, **`HOTSPR.BIN` final**, `MUSIC.ELF`, `POSTSPR.BIN`, `R18MAP1.BIN`, `R18MAP2.BIN`, `R18MAP4.BIN`, `R18MAP5.BIN`, `R18MENU.BIN`, `R18RING.BIN`, `ROM68K.BIN`, `SCSP.ELF`, `SFX.DAT` y `SPRITES.PAK`. El build genera o actualiza `A.BIN`, `ABS.TXT`, `BIB.TXT` y `CPY.TXT`.

**Los recursos binarios no versionados se restauran; no se suben ROM, PCM ni payloads a Git.** El recurso aprobado es `wp1979-fuentes-pruebas.zip`, SHA-256 `83e231d0ba11826683aaa6d22c1746e2fb9fba9ebfc0540529a73f61ab181f33`, disponible como [archivo de fuentes y pruebas](https://chatgpt.com/library?file_id=libfile_a870ebfd541881919615a0b602b60f62&account_id=personal). Desde `Combate real en Saturn/`, ejecutar:

```bash
python3 tools/restore_resources.py /ruta/wp1979-fuentes-pruebas.zip --scope build
```

El restaurador usa `RECURSOS.json`: `build` recupera los 19 archivos del staging final; `--scope all` restaura el paquete auditado completo y `--verify` sólo comprueba. Las fuentes nativas se conservan directamente en Git. Después de restaurar, copiar `wp1979/` a una ruta sin espacios antes de cargar `tooling/env.sh` y compilar. El README de la implementación contiene el procedimiento completo.

El parche ayuda a revisar diferencias respecto a la referencia. El árbol `native/` y los assets finales son la fuente de la implementación entregada; aplicar sólo el parche no aporta los assets binarios ni todas las dependencias.

## 2. Cómo incorporarlo al juego existente

1. **Comparar la rama receptora con el commit fijado.** Integrar los cambios en una rama separada y conservar este paquete como referencia. Resolver las diferencias del juego actual por función; evitar reemplazar todo el runtime si la rama ya contiene avances posteriores.

2. **Separar el arranque de prueba de la entrada del juego.** `Makefile.wp1979` fija `R18_REPLAY_TEST := 1` y define `WF_R18_REPLAY_TEST`. Esa configuración ejecuta `wf_r18_direct_boot`, selecciona el fixture, fija la semilla `1085`, reproduce la cinta y mantiene el resultado visible. **No trasladar la semilla fija ni la entrada de demo al juego normal.** El fixture no recorre selección, campaña, guardado ni siguiente combate. Las ramas sin `WF_R18_REPLAY_TEST` de `wp1979_entry.c` y `wf_saturn_frontend.inc` sirven para estudiar el enlace con el frontend, pero no fueron las ramas ejecutadas por las capturas finales. Desactivar la macro requiere integrar y validar ese recorrido.

3. **Adaptar el traspaso del frontend al combate.** El juego receptor debe proporcionar su selección, modo, participantes, RNG y estado de progresión. Mantener la inicialización nativa y los contextos `wf_runtime_t`, `wf_gameplay_binding_context_t`, vídeo, mundo, display y transición. Usar la secuencia de callbacks de `wp1979_entry.c` como referencia: bindings, lecturas ROM/RAM, entrada, árbitro, audio, HUD, paletas, ring y transiciones. No inyectar posiciones, daño, relevos o victorias para reproducir el vídeo.

4. **Conservar el contrato de frame.** El bucle llama a `wf_runtime_frame`; cuando devuelve `WF_BLOCKED` con `frame_ready`, completa display, mundo, HUD y ring, envía VDP1/VDP2 y espera su sincronización. Después reconoce el VBlank mediante `wf_runtime_vblank_ack` y continúa la lógica. No añadir otro avance lógico desde la interrupción ni multiplicar las velocidades para ajustar la tasa del vídeo.

5. **Conectar el mando al mismo intérprete.** Muestrear SMPC una vez por quantum disponible y entregar niveles activos a cero mediante `gameplay.input_hw_low`. La traducción actual usa derecha `0x01`, izquierda `0x02`, abajo `0x04`, arriba `0x08`, A `0x10`, B `0x20`, C `0x40` y Start `0x80`. En Yaul, esta entrada usa `pressed` como nivel muestreado; no sustituirlo por el campo de flancos `held`. En juego normal, conectar directamente el mando. Reservar `wp1979_input` y su cinta para una demostración explícita.

6. **Resolver salida y siguiente combate.** El fixture termina con la presentación del resultado y un bucle de espera. El juego integrado debe devolver el control a su flujo TAG/campaña y conservar sus decisiones originales de continuación, reinicio y resultado. Reacondicionar las cachés, el HUD y la memoria compartida al cambiar de escena; el frontend y el combate comparten la tabla de nombres NBG2. El código existente oculta la pantalla, refresca el HUD y revela el ring después de completar la primera lista VDP1.

No copiar los símbolos `main`, `user_init`, el callback de VBlank ni los propietarios SMPC/SH-2 esclavo encima de los del juego sin resolver su responsabilidad común.

## 3. Memoria y hardware: reservas que deben respetarse

La autoridad para los límites es `native/src/runtime/include/wf_saturn_memory.h`. Los intervalos siguientes incluyen el inicio y excluyen el límite final.

| Recurso | Inicio → límite | Uso |
| --- | --- | --- |
| LWRAM escena | `0x00201000 → 0x00221000` | 128 KiB; staging de audio durante el arranque y después blob del ring. |
| LWRAM ROM | `0x00221000 → 0x00255400` | Caché de páginas del programa arcade. |
| LWRAM escrituras de vídeo | `0x00255400 → 0x00261400` | Datos propiedad de CPU. |
| LWRAM tiles | `0x00261400 → 0x00300000` | Atlas y caché de combate. |
| VDP1 comandos, offsets | `0x00020 → 0x08020` | Reserva de comandos; primeros `0x20` bytes separados. |
| VDP1 tiles dinámicos, offsets | `0x4B580 → 0x5B580` | 64 KiB, 512 slots de 128 bytes. |
| VDP1 módulo MUSIC, offsets | `0x5C000 → 0x80000` | Código/datos auxiliares de audio; esta cola de VRAM no está libre para gráficos. |

Antes del combate, el frontend toma prestado el área de tiles para escena, tiles y su imagen RAM de 16 KiB en `0x002FC000`. **Liberar esos tres usos y reinicializar toda la caché de combate antes de cargar `HOTSPR.BIN`.** El atlas final contiene **6.474 tiles originales**, ocupa **643.456 bytes residentes en LWRAM** y deja **53 slots de respaldo**. No requiere cartucho RAM; otras secuencias de movimiento pueden provocar lecturas de CD.

Mantener el staging de SCU-DMA en HWRAM. Allí también viven los índices de sprites y la tabla inmutable de 8.192 índices de 16 bits para el ring. En el ELF final, BSS ocupa `0x06067120..0x060E85C0`; el tope de pila esclava es `0x060FF000`, a 92.736 bytes del final de BSS. La pila maestra se reserva por debajo de `0x06004000`. Estos valores describen el enlace entregado: al fusionar código cambia el mapa y hay que comprobarlo de nuevo. La distancia estática no mide el peor consumo dinámico de pila.

VDP2 conserva caracteres del mundo en bancos 0/2, nombres en banco 1 y fuente en banco 3. CRAM usa modo 2 con ventanas FG/sprites/texto/BG de 256 entradas desde `0x000/0x100/0x200/0x300`. Adaptar cualquier otra pantalla que comparta esas reservas.

### Audio y orden de carga

`SCSP.ELF` arranca el firmware de sonido; `SFX.DAT` se precarga y `MUSIC.ELF` se instala en `0x05C5C000..0x05C80000`. **El SH-2 esclavo ejecuta MUSIC**; un trabajo previo del esclavo debe haberse detenido antes de reemplazarlo. La zona compartida MUSIC se accede por `0x25C7F000`; el buzón de sonido, por `0x25A0C040`. Conservar las estructuras ABI, accesos y purgas de caché de los cargadores.

El arranque de audio reutiliza el buffer LWRAM de escena: debe completarse **antes** de cargar el ring. Mantener `wf_r18_scsp_vblank()` en el hook de VBlank y `wf_r18_scsp_service()` en contexto normal, también durante las esperas previstas. En VBlank, emitir INTBACK sólo cuando `wf_r18_scsp_smpc_owned()` sea falso. No ejecutar un segundo driver de sonido ni otro propietario del esclavo sobre esas reservas.

Conservar la lectura estable alto-bajo-alto del FRT durante el arranque. La configuración final usa **`WP1979_AUDIO=1`**; `WP1979_AUDIO=0` selecciona un stub y ya no reproduce la entrega con sonido.

## 4. Arte, velocidades y comportamiento de ROM

La ROM sigue siendo la autoridad. Conservar los gráficos, paletas, tablas, fracciones 16.16, modificadores contextuales y tiempos de animación. La optimización recorta mitades transparentes de sprites; no redibuja el arte.

| Personaje | Caminar X | Caminar Y | Correr X | Correr Y |
| --- | ---: | ---: | ---: | ---: |
| Hogan / DiBiase | 2,109375 | 1,6875 | 3,671875 | 2,9375 |
| Warrior | 2,265625 | 1,8125 | 4,140625 | 3,3125 |
| Perfect | 2,34375 | 1,875 | 4,375 | 3,5 |

Son deltas cardinales de coordenadas del mundo por paso, no velocidades independientes de la cadencia. Caminar mantiene 11 pasos por pose y correr 7; las diagonales conservan sus tablas originales. No introducir un multiplicador de velocidad ni eliminar guardas para facilitar el combate.

- `ROM68K.BIN`: SHA-256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.
- `SPRITES.PAK`: SHA-256 `130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31`.

## 5. Fixture y evidencia que debe conservarse

El caso de referencia usa IDs `0,1,9,7`, semilla `1085` y 1.761 cambios de niveles de mando. El relevo Hogan→Warrior ocurre en el paso 529; DiBiase→Perfect, en el 5.768. El combate termina en el paso **7.372**, resultado **`0x4001`**. La cinta alimenta la simulación nativa; no prescribe esos resultados.

| Ruta final, desde el paso 0 | Pasos / dibujos | VBlanks | Media a 60 VBlanks/s | Mínimos móviles |
| --- | ---: | ---: | ---: | --- |
| Combate completo, hasta resultado | 7.372 / 7.372 | 7.376 | **59,967462 fps** | 1 s: **58,0**; 5 s: 59,6; 10 s: 59,8 |
| Prueba de mando | 560 / 560 | 562 | 59,786477 fps | 1 s: 58,0; 5 s: 59,6 |

Ambas rutas registradas tienen **0 fallos fatales, 0 ventanas de lectura de CD y 0 sectores leídos durante el combate observado**. Esto no garantiza cero lecturas para cualquier entrada manual. La métrica cuenta avances y dibujos completados frente a VBlanks; no procede del contenedor MP4 a 60 fps ni incluye la pantalla final estática.

La prueba de mando usa el mismo disco y ejecutable. Pulsaciones normales por el callback libretro cancelan la demo en el paso 66. Se comprobaron 19 pasos libres de Hogan: 13 a la izquierda y 6 hacia arriba. Las demás direcciones y botones llegan al intérprete, pero esta prueba no acredita desplazamiento libre en los cuatro sentidos. En la entrada del fixture, cualquier cruceta/A/B/C/Start del mando de jugador 1 cancela la cinta y deja el control al mismo intérprete nativo.

Fuentes de evidencia: `measure/final-full/{analysis-from-start.json,report.json}`, `measure/final-controller/{analysis-from-start.json,takeover-analysis.json}`, los `identity.json` de ambas rutas y `release-candidate-4/disc-verification.json`. Consultar el informe final para las pruebas de ROM y audio, y la receta de reproducción para el schedule exacto de mando.

**Límites obligatorios al describir el resultado:** validado en Yabause/libretro con intérprete SH-2, VDP software y BIOS HLE, sin cartucho RAM, overclock ni frameskip. Ese renderer completa VDP1 síncronamente y no acredita fillrate ni contención de buses físicos. **Saturn física todavía NO probada**; tampoco BIOS comercial ni PAL. Las comparaciones conservadas con ROM cubren casos concretos: queda prohibido presentar esta entrega como equivalencia ROM completa o rendimiento físico verificado. El audio observado acredita funcionamiento/cadencia, no identidad analógica perfecta con la placa arcade.

## 6. Dependencias y trabajo pendiente para el integrador

- Para recompilar el programa: Linux x86-64 del procedimiento documentado, Bash, GNU Make, Python 3, utilidades POSIX, **SDK14 `14-sh2eb-elf-yaul-1-.zip`** aportado por el usuario y **xorrisofs externo**. El SDK se restaura en `tooling/sh2eb-elf/`; contiene GCC SH-2 13.2.0 y Yaul precompilado. No mezclar sus cabeceras con otra biblioteca Yaul sin validación nueva.
- Conservar `SCSP.ELF`, `MUSIC.ELF` y `SFX.DAT` finales permite recompilar SH-2 sin regenerar audio. Para regenerarlo hacen falta además las ROM propias `31a11-2.ic42` y `31j10.ic73`, toolchain m68k, scripts/modelo/manifiesto y alias descritos en la sección 7 de la receta de reproducción.
- Para repetir capturas: core exacto con sus fuentes/parche/licencias, Python con Pillow, FFmpeg y `nm`; para el wrapper, ELF/map/programa finales y `measure/actors-layout.bin`. Estas herramientas no son dependencias de ejecución de la Saturn.
- La integración concreta debe resolver el entry y mapa de memoria de la rama receptora, el traspaso frontend→combate→campaña, la propiedad de VBlank/SMPC/SCSP/SH-2 esclavo y el restablecimiento de cachés. Este paquete no incluye esa fusión ya realizada.
- Tras modificar el juego, comprobar primero el mismo fixture y la misma prueba de mando, y luego los recorridos de frontend/campaña que realmente cambien. La prueba en Saturn física sigue pendiente. No atribuir a un binario integrado nuevo los hashes ni las mediciones del BIN final congelado.

Usar [REPRODUCIR-ENTREGA.md](../tooling/REPRODUCIR-ENTREGA.md) para los comandos exactos. El build normal utiliza `WP1979_AUDIO=1`, `R18_ENTRY=src/wp1979_entry.c` y variables de perfil vacías. Las marcas de tiempo del empaquetado pueden cambiar el hash de un disco recompilado; comprobar su contenido y su nuevo ejecutable, además de registrar su identidad.

Esta guía se ha preparado leyendo la entrega final. Su redacción no recompila, recaptura ni amplía la validación ya registrada.
