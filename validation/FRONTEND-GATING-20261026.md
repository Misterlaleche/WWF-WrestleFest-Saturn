# Frontend y audio: comprobación interna del 26 de octubre de 2026

Esta nota registra las pruebas internas. Las revisiones R01 y R02 se han
preparado para Saturn/MODE; todavía no hay resultado de la consola física
para estas revisiones.

## Hechos observados

- En el vídeo físico `IMG_4321.mp4`, alrededor de 115 s, la pantalla negra
  anterior a la demo deja ver varias filas de luces del fondo del ring.
  Es una exposición parcial del nuevo mapa, no un fallo de las acciones de
  los luchadores.
- El código anterior activaba las capas de VDP2 dentro de
  `wf_r18_ring_update` antes de terminar de escribir los nombres de patrón.
  También liberaba la puerta de la escena al ejecutar `$1F9E`, que solo
  limpia el texto antiguo. El primer `palette_and_ring` del combate volvía
  a habilitar capas aun con `unblank_pending` activo.
- La nueva ruta prepara el mapa con `wf_r18_ring_update_deferred`, mantiene
  ocultas las capas hasta que la escena nueva esté lista y espera un barrido
  tras vaciar VDP1 antes de exponer otro mundo. La placa de copyright se
  libera tras escribir su texto en `$2503C`.
- El audio PCM dejaba `fault` tras un underrun moderado. Ahora silencia solo
  la voz afectada y continúa. El SFX se lee en bloques de 32 KiB.

## Evidencia

- Primera pantalla antidroga: marca interna `legal_first_visible=840` VBlanks
  en Mednafen. La secuencia previa con carga del atlas antes de mostrarla
  mantenía el arranque negro hasta aproximadamente 1107 VBlanks.
- `frontend_driver_verify_host.py`: prueba roja con el criterio nuevo en el
  código anterior; cuatro recorridos TAG/Royal pasan tras el cambio.
- `make test-runtime18-ring-priority`: pasa con la escritura diferida y las
  prioridades de cuerdas/luchadores.
- ISO interna `front-gated2`: SHA-256
  `71b6c1f02111b5d29d959dac92c532694980bb470f306d63919cf1b5a186bc2c`.
  `verify_disc_exact.py` confirma sectores EDC/ECC y archivos CDFS exactos.
- En la nueva emulación, la secuencia antidroga → copyright → Technos se ve
  limpia en el muestreo de un fotograma por segundo. Audio SCSP: `state=2`,
  `error=0` durante el título. El cambio título → demo ya no conserva los
  retratos sobre el ring; queda por distinguir el barrido oscuro del fundido
  original y revisar la misma escena en Saturn física.

## Incidente de validación y pila SH-2

- Dos máquinas de pruebas usaron simultáneamente `work.qcow2`; se dañaron
  metadatos de QCOW2 y la partición ext4. Se guardó
  `work-before-fs-repair-20261026.qcow2`, se repararon ambas estructuras y
  se verificaron con `qemu-img check` y `e2fsck -f -n` sin nuevos errores.
  Desde entonces solo se arranca una máquina de pruebas sobre esa imagen.
- La repetición posterior, ya con sistema de archivos sano, se detenía al
  entrar en el primer combate. Una instrumentación mínima situó la llamada
  activa en `$8CF8 → $2503C`; el lote de vídeo tenía 304 escrituras válidas,
  con el buffer y el contexto en sus direcciones previstas.
- La pila principal alcanzó `0x06001CE8`, mientras la cabecera Saturn y el
  enlazador colocaban la pila del SH-2 esclavo en `0x06001E00`: las regiones
  se solapaban. El SH-2 esclavo se ha situado en `0x060FF000`. La compilación
  sin sondas deja 21.824 bytes entre el final de `.bss` (`0x060F9AC0`) y esa
  pila. `verify_saturn_stack_layout.py` comprueba el margen en cada build
  oficial. Las sondas de diagnóstico de alta frecuencia se retiraron.
- La compilación sin sondas completó 849 fotogramas de la primera demo de
  combate y volvió al frontend en Mednafen, con `fatal_owner=0`. La imagen
  MODE1/2352 de 9587 sectores pasó `verify_disc_exact.py` con EDC/ECC y
  todos los archivos CDFS exactos. Se entregó para prueba física en
  `entregas/WrestleFest-Saturn-frontend-r01-20261026/`.

## Revisión R02: pantalla legal antes del audio

- El reset original escribe `$3100` en el puerto de sonido antes de la
  primera placa. Es una orden de parada: si el SCSP aún no ha arrancado,
  no hay voz que detener. La primera emulación bloqueó en `$6C2`; tras
  reconocer exclusivamente esa orden temprana, el arranque prosigue.
- El arranque normal deja la carga del SCSP y de SFX para después de mostrar
  la placa antidroga. La carga previa de `R18RING.BIN` era redundante:
  `frontend_world_bind` selecciona el pack de escena cuando corresponde.
  La ruta de prueba de combate directo conserva su orden anterior.
- Mednafen: `legal_first_visible=399`, comienzo de carga SCSP en VBlank 432,
  final en 764 y atlas de sprites listo en 1029. Frente a R01 (840), la
  primera placa aparece 441 VBlanks antes. La demo terminó en el fotograma
  849 y volvió al frontend con `fatal_owner=0`.
- Capturas de la revisión nueva muestran antidroga, copyright y Technos sin
  residuos en el muestreo visual de 8 capturas por segundo. En el título se
  ve brevemente un rectángulo azul durante la entrada de los retratos; su
  relación con la animación arcade queda por comprobar en el punto 3. La
  comprobación en Saturn física todavía es necesaria.
  `verify_disc_exact.py` pasó en la nueva imagen:
  9588 sectores, EDC/ECC y 22 archivos CDFS exactos. BIN SHA-256:
  `55baf7c5a9f61a7dd9ff6e230d744e03d371c050d61f45895be98f7bcdb5f126`.
  La pareja está en `entregas/WrestleFest-Saturn-frontend-r02-20261026/`.

## Demo prolongada y rutina de caída

- En la prueba prolongada previa, un ciclo posterior de demo bloqueó en
  `owner=$F4C2`, `dependency=$1BBC6`, fotograma 962, actor 2 (Ultimate
  Warrior). La ROM muestra que `$1BBC6` es el callback de state-4/subtipo
  `$1A`; estaba presente como descriptor pero no ejecutado en el port.
- Se implementaron su preset de vuelo `$258E[$17]`, el efecto compartido
  `$10D3A` (IDs 16/17), la transición por contacto a subtipo 9 y el tick
  con el descriptor original. La prueba `test-runtime17-f4c2` cubre inicio,
  temporizador y aterrizaje; pasa en el compilador nativo. La nueva imagen
  pasó la compilación SH-2 y la verificación exacta del disco.
- Después de superar esa rutina, otro ciclo de demo detuvo la revisión
  intermedia en `owner=$B7E`, `dependency=$8490`, fotograma 929. `$8490`
  prepara una escena visual de 12 retratos y reutiliza la secuencia de
  dibujo/texto del título durante 256 intervalos. Se implementó esta escena
  con el orden y la tabla de objetos de la ROM, compartiendo los propietarios
  de título ya existentes. La compilación nueva llegó al VBlank 56.982,
  atravesó esa escena y volvió a un combate posterior con `fatal_owner=0`.
  La prueba no equivale a estabilidad ilimitada ni a validación física.

## Fallo físico de R02 y aislamiento de audio

- El usuario probó la BIN/CUE R02 en Saturn NTSC-U con MODE. Tras Technos,
  pantalla negra y ruidos no identificables; no apareció el título. R02 queda
  descartada como versión de prueba jugable. La prueba anterior de Mednafen
  no reproducía este fallo físico.
- Se generó una imagen diagnóstica a partir de la BIN entregada, verificando
  todos sus sectores MODE1/2352. Solo `SCSP.ELF` cambia a la versión nativa
  de efectos, rellenada hasta el tamaño CDFS original. `A.BIN` y los otros
  20 recursos conservan sus bytes, LBA y longitud. El firmware de efectos
  hace que `wf_r18_scsp_boot` omita `MUSIC.ELF` y el módulo de música del
  SH-2 esclavo; mantiene el resto del frontend intacto.
- Beetle Saturn ejecutó 6.400 fotogramas de la variante diagnóstica y mostró
  Technos, título con todos los retratos y comienzo de la demo. R02 original
  también llega al título en ese emulador. La variante diagnóstica va por
  delante en fotogramas absolutos, pero ya adelanta al original durante las
  placas legales por tener una inicialización de audio más corta. No se puede
  atribuir esa diferencia a la velocidad de los retratos. La ruta de música
  sigue siendo una hipótesis física por aislar, no una causa demostrada.
- El programa de música ejecuta instrucciones y datos desde la cola de VRAM
  VDP1 (`0x05C5C000..0x05C7FFFF`). El manual oficial de Sega indica que
  acceder a VDP1 VRAM durante el dibujo puede costar alrededor de una docena
  de ciclos por acceso. En el emulador la penalización puede diferir.
  Fuente: https://www.infochunk.com/saturn/segahtml_en/hard/vdp1/hon/p02_10.htm

## Límite actual

La inspección del emulador no acredita ausencia de residuos ni progresión
tras Technos en Saturn física. La BIN/CUE R02 original falla en Saturn/MODE.
La prueba diagnóstica de aislamiento determinará si el módulo de música
causa ese fallo o si hay que instrumentar la transición gráfica. El punto 2,
el timing arcade de retratos y los objetivos siguientes siguen pendientes.
