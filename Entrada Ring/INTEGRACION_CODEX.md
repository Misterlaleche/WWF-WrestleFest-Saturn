# Entrada Ring — guía de integración posterior para Codex

## Alcance y estado

Este directorio entrega **WF-WP01 Entrada TAG R02** como bloque autónomo: pasillo de entrada de una pareja TAG, sin presentación dentro del ring ni gameplay. La subida conserva las fuentes de producción; no conecta el bloque al juego. `source/main.c` es el ejecutable de prueba, no el bucle principal canónico.

**El nombre WP01 de esta entrega no reasigna propietarios del proyecto.** El WP01 de finishers/callbacks citado en `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md` conserva su responsabilidad. Identificar este módulo como «Entrada Ring / entrada TAG R02» para evitar confundir ambos ámbitos.

`VERIFICACION.json` documenta el candidato funcional en emulador: comparación de 756 estados de actores con referencia MAME, ensayos de reinicio y captura de audio nativo. Declara prueba física pendiente y audio no idéntico bit a bit al YM2151. El «Todo ok» del usuario no identifica plataforma, artefacto ni alcance de ensayo; no convertirlo en un HARDWARE-PASS específico. Las mediciones registradas son evidencia anterior, no pruebas ejecutadas de nuevo al subir las fuentes.

Los gráficos y el controlador de escena se conservaron respecto de R01 según esa verificación. R02 usa seis voces FM nativas SCSP, con 24 operadores, más muestras OKI individuales originales. **El timbre es una adaptación aproximada SCSP del YM2151**; no es un reemplazo global del backend R04/WP24 del repositorio.

## Lectura obligatoria y frontera del repositorio

Antes de integrar, leer desde la raíz del repositorio:

- `README.md`, `HANDOFF.md`, `PROJECT-STATE.md` y `docs/SATURN-PIPELINE.md`.
- `docs/MODE-ROSTER-HARDWARE-VALIDATION.md` y `docs/RUNTIME-AUDIO-RECONCILIATION.md`.
- `docs/AUDIO-INTEGRATION-READINESS-R05.md`, `docs/AUDIO-ROM-EXACT-R04-CLOSURE.md` y `docs/AUDIO-HARDWARE-VALIDATION.md`.
- `src/audio/wp24/wf24_scsp_layout.h`, `wf24_audio_contract.h`, `wf24_music_stream.h` y `wf24_command_bus.h`.

La revisión previa de `main`, commit `5246319f335c8d7bd3c5476f2d9b7a19b322b38f`, no encontró `AGENTS.md` ni runtime canónico de gameplay/escenas. Los documentos confirman esa ausencia. Volver a inspeccionar el árbol al integrar: no inventar nombres de hooks, un controlador de escenas o un asignador de recursos que aún no existen. No importar WIP160/175/176 como sustituto.

Se mantiene la jerarquía ROM original > paquetes FINAL auditados > documentación Saturn/Yaul > código. Integrar módulos fuente mediante una plataforma común; nunca concatenar BIN/CUE. Mantener abiertos los comportamientos cuya evidencia o propietario falten.

## Recursos externos y reproducción

Los binarios gráficos y sonoros quedan externos con sus hashes. Restaurarlos mediante `restore_assets.py` siguiendo el README de esta entrega y usando únicamente el ZIP aprobado, cuyo SHA256 es:

`dcacfe5d4de8067067194a9c0117fad95ad912982ebbb0030bf1e09cc4a8024d`

No fabricar sustitutos si falta el ZIP. No añadir ROMs, packs protegidos o capturas PCM al repositorio como efecto de la integración. El `source/Makefile` requiere `YAUL_INSTALL_ROOT`, enlaza los gráficos declarados en `BUILTIN_ASSETS` y genera el ejecutable autónomo `WF-WP01-R02`. El runtime carga `SOUNDRAM.BIN` y `AUDIOREG.BIN` desde la raíz del disco.

| Recurso R02 | Bytes | SHA256 |
|---|---:|---|
| `SOUNDRAM.BIN` | 346208 | `6938a51117b38ce536b675caf69c29e2061ef74d851fa2e738f9b9a2496bb9ee` |
| `AUDIOREG.BIN` | 19776 | `9b3e7fef5d8511eeab5bfdf7359c2a01d1306a42a76c10748137a27b566c2ba1` |

## API real de escena y selección

`source/wp01_scene.h` expone `wp01_scene_init(s, id0, id1, rngseed)`, `wp01_scene_step(s)`, `wp01_scene_set_rng_hook(s, hook, context)` y `wp01_rng_21b4(state, d1_to_d7_sum, global_timer)`. El estado `wp01_scene_t` contiene `frame`, `movement_updates`, `done`, `wipe_tile`, dos actores y la luz.

Los argumentos son **IDs internos ROM seleccionados**: 0 Hogan, 1 Warrior, 2 Jake, 3 Bossman, 4 Hawk, 5 Animal, 6 Slaughter, 7 Perfect, 8 Earthquake, 9 DiBiase, 10 Smash y 11 Crush. Hawk/Animal no forman parte de las selecciones de este harness. `init` aplica el orden ROM: Hogan a la derecha y DiBiase a la izquierda; `actor[0]`/`actor[1]` son posiciones físicas, no necesariamente el orden de entrada.

`source/wp01_pairs.h` enumera 29 parejas legales: las 28 combinaciones de ocho individuales y Demolition (Smash/Crush). El sorteo `entropy()%29`, la semilla RTC/FRT y los reinicios pertenecen al harness. En integración, recibir la pareja elegida por el propietario TAG y validar su mapeo ROM; no volver a sortearla ni inferir IDs desde el orden visual del roster.

El hook RNG recibe `(context, actor, movement_update)` donde la ROM llama a `$21B4`; el bit 0 decide el ciclo de poses. La semilla nativa no reproduce los registros vivos D1..D7 de la CPU arcade. Para comparación exacta usar la secuencia de entropía demostrada o enlazar el estado canónico probado; no presentar una semilla fija como equivalencia ROM.

## Cadencia, buffering y sincronía

- El núcleo produce **402 actualizaciones**, frames 0..401. `init` ya produce frame 0. Mostrar el estado y avanzar una vez por actualización arcade.
- Fases: 0..23 apertura; 24..343 caminar; 344..367 cierre; 368..400 negro; 401 limpieza terminal. Un `step` **después de mostrar 401** activa `done` y conserva `frame == 401`.
- Hay **6 actualizaciones negras de preparación externas al núcleo**: 408 estados de salida contando preparación y núcleo. No son 408 VBlanks Saturn.
- `main.c` usa acumulador: sumar `11*1820*263`, umbral `448*272*45`, fase inicial `umbral-1`. Conserva la relación arcade/NTSC 320; no cambiar a «un step por VBlank» ni compensar carga alterando tiempos ROM.
- Los sprites arcade tienen **una actualización de latencia**. El renderer dibuja `previous` mientras fondos/texto usan el estado actual; en cada avance copia `previous=scene` antes de `wp01_scene_step`. No retrasar también el núcleo ni añadir otro buffer de un frame.
- Usar `anchor_x/anchor_y` con los offsets de las piezas; no añadir un desplazamiento extra de 16 px. `world_y` es signed 16.16.
- `restart()` prepara audio y marca `audio_start_pending`; el callback VBLANK-IN ejecuta `wp01_audio_start()`. Preservar la alineación del inicio sonoro con el primer commit visual de preparación, no arrancar audio al comenzar la caminata.
- La repetición automática se dispara por `TOTAL_FRAMES` en el harness; no consume `scene.done`. START elige otra pareja y A repite la actual mediante el estado de botones de ese ejecutable. Ninguno de esos reinicios define el flujo del juego final.

La captura anterior midió 424 frames de vídeo entre repeticiones. Es una observación del harness, no una duración nueva del núcleo. La propia verificación indica que el reinicio recorta la cola del último golpe PCM; no prolongar o recortar la escena al integrar sin demostrar la transición y el ciclo de vida sonoro correspondientes.

## API real de audio y propiedad global

`source/wp01_audio.h` expone `wp01_audio_init()`, `wp01_audio_bank_load(data, bytes)`, `wp01_audio_program_load(data, bytes)`, `wp01_audio_prepare()`, `wp01_audio_start()`, `wp01_audio_stop()`, `wp01_audio_service()` y `wp01_audio_written()`. `service()` está vacío: la reproducción se atiende por interrupciones, no por llamadas periódicas a esa función. `written()` informa eventos temporizados enviados; no es un callback de final de escena.

Secuencia autónoma existente: inicializar plataforma/audio; copiar banco; vincular programa; preparar escena/audio; arrancar audio en VBLANK-IN. `wp01_audio_program_load()` conserva punteros al buffer: **no copia `AUDIOREG.BIN`**. `main.c` reutiliza `LWRAM(0)` tras copiar el banco a SCSP y deja allí el programa residente. Reservar ese buffer durante toda la reproducción; no reutilizarlo para cargas de otra escena.

**Estas funciones asumen propiedad exclusiva del hardware en el harness:**

- `init()` ejecuta `SNDOFF` sobre el procesador de sonido, escribe registros comunes, borra los 32 slots y el área DSP `0x800..0xBFF`. No llamarlo dentro del runtime como si fuera inicialización local inocua.
- `stop()` silencia y desactiva **los 32 slots, incluido el 31**, y ejecuta KYONEX global. `prepare()` y `program_load()` llaman a `stop()`.
- Los datos musicales usan slots 0..23 para FM y 24..27 para OKI, pero esa asignación no limita el alcance de init/stop. KYONEX evalúa los bits KYONB de todos los slots.
- El escritor preserva al menos 39 ticks FRT entre una ejecución de key y la siguiente modificación de control; equivale al margen de dos muestras SCSP requerido por este programa. Mantener el orden y las guardas de KEYOFF/KEYON.

### FRT e interrupciones

R02 usa FRT SH-2 con divisor 32 y reloj contractual `120093750/143` ticks/s para NTSC 320. Instala prioridad 9 y sustituye el vector compartido `CPU_INTC_INTERRUPT_FRT_OCI`; su handler atiende compare-A. Programa plazos adaptativos mediante `cpu_frt_oca_set`, con heartbeat `0x4000`, y `stop()` desactiva compare-A. El `user_init()` del harness inicializa FRT.

Antes de integrarlo, resolver con el propietario de plataforma el vector OCI, compare-A, compare-B, prioridad y lectura del contador; no asumir que son libres. No reinicializar el reloj compartido ni sustituir handlers ajenos. La liberación/restauración del vector no está implementada como API de escena y deberá diseñarse en la integración. No añadir una fase WP28 por conveniencia.

### Mapa SCSP y conflicto con WP24

`bank_load()` copia desde offset SCSP **0**: el banco ocupa `[0x00000, 0x54860)` en `0x25A00000`. **Solapa** el área residente WP24 `[0x14000, 0x3C5E4)` y parte de su caché dinámica `[0x3C600, 0x5A600)`. También sobrescribe cualquier contenido del sistema alojado en la zona baja de sound RAM.

WP24 mantiene slot 31 y ring `[0x60000, 0x70000)`; aunque el banco R02 no alcanza ese ring, init/stop sí afectan al slot y al propietario M68K/poneSound. No ejecutar ambos backends simultáneamente sin una solución explícita de propiedad, memoria y restauración. No desplazar únicamente el banco: los registros del programa contienen direcciones de muestras y requieren una adaptación coherente si se reubica.

El backend canónico R04 mantiene Z80 + Nuked-OPM, transporte PCM16BE y tratamiento original de `0x1F`. Esta entrada SCSP no autoriza sustituirlo, remapear productores ni declarar cierre hardware del audio global.

## Recursos gráficos, memoria y carga

`source/main.c` configura globalmente vídeo 320×240 sin entrelazado, CRAM modo 2, NBG0/NBG1/NBG2, prioridades, scroll y ciclos VRAM. El renderer de la entrada debe recibir propiedad explícita de esos recursos y de sus callbacks VBlank.

| Recurso del harness | Asignación real |
|---|---|
| VDP2 NBG0 / gráficos | `0x25E00000`, 58112 bytes; mapa/crowd en `0x25E60000`, 4096 bytes |
| VDP2 NBG1 / gráficos | `0x25E20000`, 34432 bytes; mapa en `0x25E62000`, 4096 bytes |
| VDP2 NBG2 / fuente y texto | Fuente en `0x25E40000`, 131072 bytes; mapa en `0x25E64000`, 16384 bytes |
| Color de fondo | VDP2 offset `0x7FFFE` |
| CRAM entorno / sprites | `0x25F00000`, 3072 bytes / `0x25F00C00`, 704 bytes |
| VDP1 | Capacidad de 128 comandos y 90496 bytes de texturas; base obtenida de las particiones Yaul |
| Buffers de texto CPU | Dos mapas de 16384 bytes y texto bruto de 8192 bytes, además del resto del estado |

Los sprites usan piezas 16×16, prioridad 3; los fondos tienen prioridades NBG0=2, NBG1=1, NBG2=5. Se actualizan crowd y texto con DMA encolado. La carga inicial usa SCU DMA canal 0 y espera su terminación. Conservar composición, scroll, prioridades y commits VBlank al trasladarlos al renderer compartido; no reconstruirlos por semejanza visual.

El bloque MODE/ROSTER anterior usa también NBG0/NBG2 y ya exige blank/sync antes de reconfigurar CRAM/VRAM. Aplicar una entrega ordenada de recursos entre escenas para evitar flashes de paleta. Conservar su confirmación localizada 96×96; la entrada no modifica ese comportamiento.

`source/cd_transfer32.c` define símbolos globales `cd_block_init`, `cd_block_transfer_data` y `cd_block_sectors_read` para la ruta A-bus de 32 bits. Revisar duplicación/interposición al enlazar con la plataforma común; no añadir otro lector CD concurrente. Mantener su atribución/licencia. Las direcciones de carga/pila del Makefile pertenecen al ejecutable autónomo, no constituyen un mapa de memoria del master.

## Trabajo pendiente de integración y validación

1. Recuperar el runtime canónico y enlazar la selección TAG con IDs ROM demostrados. Definir enter/update/render/exit bajo su controlador real; estos nombres describen responsabilidades, no APIs existentes.
2. Asignar y liberar vídeo, DMA, buffers, FRT/OCI y sonido con sus propietarios. Separar la lógica reutilizable del boot, SMPC, CDFS, bucle y controles del harness sin alterar las fuentes de referencia depositadas.
3. Preservar las 6+402 actualizaciones, latencia de sprites y origen audiovisual. Tras mostrar el estado terminal, devolver finalización al propietario de scene flow; **no inventar presentación del ring, siguiente música, pausas ni entrada a gameplay**.
4. Ejecutar los tests aplicables de `source/tests/`, comprobar hashes restaurados y compilar el integrado. Medir retrasos IRQ, VBlanks perdidos y presupuesto de CPU/RAM bajo carga real; los máximos de `VERIFICACION.json` no garantizan esos presupuestos.
5. Validar el artefacto integrado identificado por hash en Saturn NTSC-U + MODE, con prueba visual/sonora, transiciones y recuperación de recursos. Mantener separadas la evidencia previa de emulador, la aceptación del usuario y cualquier resultado físico nuevo. PAL permanece sin validar.
