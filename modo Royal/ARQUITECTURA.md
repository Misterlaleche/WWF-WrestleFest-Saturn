# Cómo está hecho Royal HW1

## 1. Origen y alcance

WP1980 parte de una implementación nativa R18 cuya referencia registrada es el commit `d95b8c37f62402bce4dead13cf7c0567b8fc2439`. HW1 es una copia aislada de esa entrega Royal, con nueve inputs modificados para residencia del resultado, límites de memoria, diagnóstico y el anillo de efectos. El árbol que se publica aquí conserva los bytes del candidato; los manifests permiten contrastarlo con el paquete privado.

La autoridad es `wwfwfest.zip` y, después, los paquetes FINAL auditados y la documentación Saturn/Yaul. No se deduce que todo el juego sea ROM-EXACT porque una demostración llegue a su final. Lo no demostrado permanece abierto.

El runtime no ejecuta un emulador 68000 general como motor del combate. Ejecuta en SH-2 las rutinas C nativas de propietarios, estados y recursos. El intérprete Musashi se utiliza **solo en la prueba host del tramo original de resultado**, restaurable desde el paquete; no forma parte de esta compilación Saturn.

## 2. Archivos principales

Todas estas rutas son relativas a `wp1980/work/hwfix01/`:

| Ruta | Función |
| --- | --- |
| `native/Makefile.wp1980` | Entrada activa, fuentes, opciones base/cart, audio y límites de memoria. |
| `native/src/wp1980_combat_entry.c` | Arranque directo, plataforma del banco de pruebas, callbacks, loop vivo, resultado y reinicio. |
| `native/src/wp1980_royal_init.c` | Valida `ROYINIT1`, tamaño, CRC y punto original; importa el estado inicial. |
| `native/src/wp1980_input.c` | Entradas automáticas de demostración y entrega del control físico. |
| `native/src/wf_gameplay_bindings.c`, `wf_scheduler.c` | Asociación de propietarios y avance del runtime. |
| `native/src/wf_r17_participant_native.c`, `wf_r17_royal_9132.c`, `wf_royal.c` | Rutas nativas de participantes y Royal; no confundir una sola ruta probada con cobertura de todas las mecánicas. |
| `native/src/wf_r18_present.c` | Lecturas de ROM, cachés, atlas y composición de sprites. |
| `native/src/wp1980_vram_cache.c` | Caché persistente de tiles VDP1 y protección de tiles usados en el fotograma actual. |
| `native/src/wf_saturn_yaul_backend.c` | Comandos, staging y transporte a VDP1. |
| `native/src/wf_r18_ring.c`, `wf_r18_video.c`, `wf_r18_hud.c` | Ring, planos VDP2, paletas y HUD. |
| `native/src/wf_r18_cdfs.c`, `wp1979_cd_read.c` | Archivos de CD, lecturas y diagnóstico del transporte. |
| `native/src/wf_r18_result_screen.c` | Presentación original de Royal GAME OVER. |
| `native/src/wf_r18_scsp.c` | Adaptador de audio, arranque y servicio de cola. |
| `audio/native/wf_scsp_sfx.c` | Motor PCM; contiene la corrección de límite físico del anillo. |
| `native/include/wf_saturn_memory.h` | Contratos y asignaciones de memoria; revisar junto a las opciones del Makefile. |

## 3. Entrada al combate y simulación viva

`user_init()` configura la consola, el muestreo SMPC, VBlank y el modo de pantalla. La variante cart exige detectar una expansión real de 4 MiB; si no está disponible falla de forma explícita. No cambia automáticamente al ejecutable base.

`main()` inicializa CDFS, vídeo y audio. Construye los bindings, llama a `wf_runtime_init()` y carga el punto inicial con `combat_load_initial()`. `ROYINIT.BIN` tiene 106.664 bytes y conserva estado de RAM, registros, mapas, sprites, paletas y scroll de una captura original en `0xF94`. Su importador comprueba magic, versión, tamaño, CRC y condiciones Royal; `wf_r18_runtime_handoff_f8c()` realiza la continuación original pendiente. La semilla de esta demostración procede de ese estado capturado, `6755F679`; no es una elección de RNG de la campaña integrada.

Después se cargan el ring, las páginas de ROM, el atlas apropiado y `SPRWARM.BIN`. Se presentan los planos mientras la pantalla está en blanco y se revela la escena cuando ha terminado la primera lista VDP1 calculada en vivo. Esta secuencia evita mostrar paletas o memoria parcialmente reemplazadas.

Cada vuelta del loop presta servicio al audio, toma una muestra física de control y alimenta las entradas de los participantes. `wf_runtime_frame()` avanza los propietarios. Cuando devuelve `WF_BLOCKED` con `frame_ready`, el entry ejecuta los trabajos pendientes de display/world/HUD, prepara VDP1 y VDP2, espera sus sincronizaciones, reconoce VBlank mediante `wf_runtime_vblank_ack()` y reanuda el runtime. Ese estado bloqueado de presentación es una frontera esperada, no un error que deba eliminarse para ganar velocidad. Los bloqueos sin continuación demostrada conservan su diagnóstico.

Los actores mantienen registros originales de 268 bytes; la comparación terminal cubre los nueve completos, no solo posición, vida o el actor del jugador. Los callbacks de juego llevan propietarios/direcciones ROM para seguir su procedencia. El código de presentación y el código de reglas son capas distintas, pero comparten el mismo avance real del estado.

## 4. Qué pertenece a la demostración

`wp1980_input()` calcula distancia a rivales activos y genera entradas activas a nivel bajo: acercamiento por cruceta y pulsos A/B. No decide por sí mismo si un golpe, agarre, caída o eliminación ocurre: esas decisiones corresponden al runtime. No representa la IA original del jugador humano y no debe describirse como un attract mode ya integrado.

En el entry directo, `wp1980_frontend_input_disable()` desactiva el arranque automático de menús. El mismo archivo conserva helpers de frontend históricos porque pertenecen a los inputs entregados, pero ese no es el recorrido activo del disco HW1.

Pulsar una entrada física de P1 reconocida por el adaptador (`cruceta`, `A`, `B`, `C` o `Start`) pone `wp1980_demo_active=0`. Desde ese momento se usan las entradas físicas. Los cuatro puertos lógicos se muestrean con la ruta de mandos del proyecto; la existencia de ese código no acredita todas las configuraciones multijugador en consola.

| Control P1 | Comportamiento |
| --- | --- |
| Cruceta | Bits de dirección original: derecha 1, izquierda 2, abajo 4, arriba 8. |
| A / B / C | Acciones 1 / 2 / 3, bits `0x10 / 0x20 / 0x40`; la acción concreta depende del estado original. |
| Start | Entrada `0x80` y cancelación del input automático; no se introduce una pausa nueva. |
| A+B+C+Start | Reinicio local Royal por el polling de la combinación; conserva el control manual. |
| Izquierda/derecha en una pantalla de fallo | Navega cinco páginas del snapshot congelado. |

## 5. Gráficos, CD y expansión de memoria

Los gráficos proceden de los recursos originales y conservan índices/paletas. VDP1 dibuja los sprites; VDP2 conserva ring, fondos y HUD. La caché VRAM dispone de 3.584 slots en la configuración entregada. Protege todos los tiles necesarios en un fotograma antes de reemplazar otros, comparte IDs repetidos y conserva la identidad del tile/crop. Este ahorro de transporte no cambia los pasos de animación originales.

La variante base carga el atlas comprimido sin pérdida `ROYSPR.BIN` (formato R18Z) y mantiene índices y cachés acotadas. La variante 4 MB utiliza `ROYRAW.BIN` (R18U) en el cartucho para evitar ese trabajo de descompresión. `combat_file_read()` traduce el nombre lógico `HOTSPR.BIN` al atlas de cada variante. El pack `SPRITES.PAK` sigue disponible como fuente fría; no se infiere que cualquier entrada manual esté libre de CD porque la demostración medida no lo necesitase.

Las lecturas de datos de programa consultan primero el mapa rápido inmutable, después la caché de páginas normal y sus pequeñas líneas. En HW1 se fijan las páginas de 1 KiB **10, 11, 14, 17, 18, 148 y 151** del resultado. Las páginas 11, 148 y 151 se promueven desde el conjunto reemplazable; las únicas incorporaciones al conjunto residente son 10, 14, 17 y 18. No se pierde ninguna página anteriormente residente.

| Configuración HW1 | Base | 4 MB |
| --- | ---: | ---: |
| Páginas rápidas inmutables | 39 | 135 |
| Páginas precargadas del conjunto principal | 198 | 133 |
| Staging de cabecera de combate | 4.096 B | 4.096 B |
| Área de índices HWRAM | `0x2B000` B | `0xB000` B |
| Margen desde final enlazado a fin HWRAM, observado | 22.764 B | 56.076 B |
| Distancia entre fin de BSS y pila secundaria, observada | 18.880 B | 52.192 B |

Los límites mínimos siguen siendo 16.384 bytes de reserva tras el final enlazado y 8.192 bytes hasta la pila secundaria. La pila principal queda bajo `0x06004000`; la secundaria se fija en `0x060FF000`. Estos son resultados de los binarios HW1 archivados; cada recompilación debe volver a pasar los verificadores.

`wf_saturn_memory.h` asigna LWRAM a escena (`0x00201000`), ROM (`0x00221000`), staging de escrituras (`0x00255400`) y tiles (`0x00261400`). Las transferencias SCU-DMA utilizan su staging HWRAM; no trasladar ciegamente esos buffers a LWRAM para liberar espacio. La política de VRAM/CRAM y el arrendamiento temporal de regiones de frontend necesitan un único propietario en la integración final.

La revisión independiente inicial señaló el riesgo de tamaños intermedios de cabecera. El fuente final impone potencia de dos y al menos 4.096 bytes; las pruebas archivadas rechazan 6.144 y 10.240. El comentario de esa revisión histórica no es una excepción pendiente que autorice esos tamaños. Véanse `header-size-guard/`, el `_Static_assert` final y el informe HW1.

## 6. Resultado y salida

`wf_r18_royal_game_over()` reconoce la frontera original del Royal con crédito. El entry guarda frame, resultado, contadores y tiempos, y llama a `wf_r18_result_royal_game_over()`. Se mantienen los commits, la paleta y la espera original explícita de 97 períodos. La presentación completa medida ocupa 98 campos, incluido el trabajo de presentación.

Después, el banco de pruebas permanece en GAME OVER prestando servicio al audio y al mando, hasta la combinación de reinicio. Esa espera no define el siguiente estado de la campaña. Al integrar, el propietario global de scene flow debe recibir el resultado y decidir la continuación demostrada por ROM.

Las cinco fotografías del usuario sitúan el fallo anterior en el propietario `0x1AFC`, frame 8.474. La investigación encontró la dependencia fría inicial de paleta en ROM `0x2A98`, offset de archivo `0x2800`. Eso no identifica por sí solo un timeout, un defecto de MODE o el estado concreto del lector. HW1 evita la dependencia durante el resultado, conserva el retorno de error y mejora su procedencia; no oculta un fallo.

El probe host deniega lecturas posteriores al preload y reemplaza las cachés normales. En base/cart mantiene cero lecturas de archivo para el resultado y el mismo estado final. El oráculo ejecuta el tramo 68000 original `0x1AFC..0x1B1E`: su proyección portable de 55.702 bytes coincide con los cuatro GREEN. El fixture combina ROYINIT con actores/RNG/frame terminales; no es un savestate completo de toda la partida.

## 7. Efectos y música

La configuración entregada es `WP1980_AUDIO=1`, `WP1980_DISABLE_MUSIC=1`. El SH-2 carga `SCSP.ELF`, carga el material `SFX.DAT`, crea la cola y presta servicio a comandos. El arranque comparte la propiedad de SMPC con el resto de la plataforma; VBlank llama al tick de audio. No sustituirlo por un segundo propietario independiente durante la integración.

HW1 corrige en `wf_scsp_sfx.c` el caso FAST_68000 que intentaba escribir 64 muestras contiguas comenzando en la posición 4.064 de un anillo de 4.096. La nueva cantidad se limita al espacio físico restante y la siguiente iteración continúa en cero. La prueba usa fill/work/advance reales; solo reemplaza el decodificador ensamblador por un escritor contiguo determinista con la misma ABI. El RED registra 20 aserciones fallidas; GREEN, 13 casos sin fallos.

El firmware corregido tiene SHA-256 `c06a9244dd36eb8b735f3328a59dfff32515c9e3405fb3ec64616b778c7b07ae`. Se conserva externamente y se restaura por hash. La reconstrucción anterior del firmware coincidió byte por byte, justificando la cadena de herramientas usada para la corrección.

En ambas partidas HW1 se envían y completan 131 comandos; la cola termina vacía y sin errores registrados. Esto acredita transporte de comandos. La señal no silenciosa del vídeo no demuestra fidelidad, ausencia de pérdidas ni sincronización acción por acción. La política previa que detiene voces tras más de 512 frames SCSP sin servicio no se cambió; su activación física no está demostrada.

Los comandos musicales `0x01..0x12` y `0x1F` se filtran cuando la música está desactivada. No presentar ese comportamiento diagnóstico como implementación musical final. El backend histórico incluido en este snapshot no desplaza al propietario WP24 ni a la autoridad R04/R05 del repositorio. La integración final debe preservar su reproducción, lifecycle y mapeos probados, con presupuesto simultáneo de gameplay, VDP1, VDP2 y audio.

## 8. Contrato de integración

Integrar módulos fuente; nunca concatenar BIN/CUE. Mantener un único propietario de boot, CDFS, SMPC, VBlank, VDP1/VDP2, SCSP, CRAM/VRAM y memoria. El código de `wp1980_combat_entry.c` es un banco de pruebas que ilustra el enlace; no reemplaza el bucle maestro CLEAN-R01.

La entrada real deberá enlazar modo Royal, elección de luchador mediante el mapeo probado, estado/RNG/progresión y setup equivalentes de ROM. La salida deberá entregar el resultado al propietario de flujo existente. Preservar los invariantes hardware de MODE + ROSTER TEST03: blank/sync antes de cambiar CRAM/VRAM, NBG0 transparente sobre fondo fijo, primer plano residente y parche de confirmación 96×96 en VBLANK-IN.

La incorporación de esta carpeta no cierra por sí sola ninguno de esos contratos. Una nueva ruta, semilla, cambio de disposición o activación de música requiere evidencia y repetición de los controles de memoria, estado y hardware correspondientes.
