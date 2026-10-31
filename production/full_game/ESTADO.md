# Estado de integración — 2026-10-31

Estado: CHECKPOINT04 PARCIAL, no beta completa.
Hay un disco universal de integración MODE/ROSTER probado en Mednafen1.32.1
None/1MiB/4MiB. La confirmación termina esta ruta parcial; aún no abre combate.
La ampliación se detecta y valida, pero las cachés de gameplay no están integradas.

## Nuevo en checkpoint04

- Un propietario CD/FRT y transporte CDFS/CD32 original HW1 conservado.
- Siete recursos aprobados cargados antes de MODE: tamaños y CRC32 comprobados;
  sólo se publican los siete punteros cuando termina toda la carga.
- Ocupación de130848B en una reserva de128KiB de LWRAM. El final de estáticos
  HWRAM baja105504B respecto al03; no es todavía el presupuesto de combate.
- Conversión de los diez slots a identidad original comprobada contra ROM.
  Esto no sustituye la selección de compañero, CPU, RNG ni formación de equipos.
- Mismo disco en Mednafen1.32.1: None/TAG,1MiB/ROYAL,4MiB/TAG y A mantenido
  durante scroll PASS. Los96 sectores del contador de carga no aumentan durante
  estas selecciones. Reloj coherente en247/261/246/203 publicaciones activas.
- Quinta prueba, disco deliberadamente alterado: rechaza el último recurso
  por CRC antes de MODE y deja los siete punteros vacíos. No se entrega ese
  disco negativo como ejecutable funcional.
- Suite C11/ASan/UBSan y9testsPython con ROM PASS; originales2023/2023 intactos.
  Revisión independiente sin defectos accionables en el alcance04.
- Cinco vídeos de ejecución conservan todos los PTS de los MOV nativos.
  El retrato final probado coincide píxel a píxel con03; no se afirma equivalencia
  de toda la animación ni del selector arcade completo.

Reproducción, hashes, evidencias y límites: docs/RECOVERY-04.md.
Auditoría de selección, compañero y límites de TEST03: docs/SELECTION-04.md.
No hay prueba física del nuevo disco ni prueba del recorrido completo.

## Conservado desde checkpoint03

- Único main, VBlank/SMPC y reloj37609/39168; deuda conservada.
- Detección SDK más validación de16 puntos en bancos;1MiB no activa4MiB.
- Renderer TEST03 derivado de fuente fijada por hash; reutiliza122880B.
- Mando real emulado: MODE, páginas del roster, regreso y confirmación.
- Cuatro pruebas Mednafen PASS sobre el mismo ejecutable; incluye mantenerA
  durante scroll. Esta regresión fue reproducida y corregida antes de publicar.
- Reloj observado con publicación coherente; ninguna violación en
  247/264/246/200 publicaciones activas distintas. Deuda máxima observada2.
- Tests C11/ASan/UBSan y9testsPython con ROM pasan; originales2023/2023 intactos.
- BIN/CUE, ELF/mapa, vídeos reales y configuración conservados. Sin audio de juego.

Evidencia histórica del disco03: docs/RECOVERY-03.md.

## Referencias verificadas en checkpoints01/02

- Recuperados 2023 archivos del commit aedca682; sus 989 blobs y árbol Git
  coinciden. Las carpetas originales permanecen intactas por SHA256.
- ROM del usuario: programa68000 SHA1 a726c46f350723ea73158bc78055d17d418b14e2.
- Nuevo reloj racional37609/39168 con deuda conservada;100 períodos exactos,
  errores y overflow comprobados; C11 y ASan/UBSan.
- Adaptador Entrada→Presentadora enlaza directamente los núcleos aprobados;
  6+402 salidas Entrada, latencia de sprites, IDs lógicos y terminales conservados.
  53 recorridos contra núcleos originales. Continuidad RNG entre propietarios OPEN.
- Presentadora original:20 casos/24922 ticks contra ejecución ROM mediante
  Unicorn2.1.4; estados,RNG y comandos comparados.
- Core SH2 GCC13.2.0 compilado y enlazado relocatable sin backend audio.
  No es un programa arrancable; necesita todavía startup/libc/plataforma/renderer.
- Ocho texturas411712B regeneradas desde ROM y hashes aprobados;6 tests pasan.
- Cinco ZIP suministrados por el usuario coinciden con sus SHA256. Recursos
  visuales19/19 restaurados y verificados. TAG es R3; R6 NO recuperada.
- Recibido y verificado además MODE+ROSTER TEST03:30 hashes de manifiesto y
  BIN de referencia coincidente con el hardware-pass. Fuentes preservadas en vendor.
- R3 recuperada íntegramente:370 fuentes/config/herramientas y SOURCE.json en Git.
- Reconstruidos Intro,Entrada,Presentadora,TEST03,TAG y Royal base/4MiB.
  Los cuatro primeros ejecutables coinciden con sus referencias aprobadas.
  Siete discos de referencia pasan EDC/ECC completo y correspondencia A.BIN.
- Royal sin audio:smoke Mednafen1.32.1/full,180 campos por perfil,None/4MiB,
  176 dibujos,cero contadores fatales y MOV nativo. No es recorrido completo.
- Pasadas largas Royal completadas:8474 actualizaciones hasta GAME OVER por
  perfil;RNG y9 actores iguales a HW1,CDdelta0,cero fatales. Media50,5904/s base
  y58,8948/s4MiB;limitaciones y ventanas en docs/PERFORMANCE-02.md.
  Los vídeos MOV/MP4 conservan todos los PTS. El objetivo integrado sigue OPEN.
- Royal: inflate fijo378 comprobaciones,LZ4 1536,dinámico ASan/UBSan13005;
  estas pruebas iniciales no incluían atlas original.

## OPEN y trabajo siguiente

1. Completar integración de plataforma y pruebas con mando del juego completo; las pasadas largas Royal
   de referencia y sus binarios,mapas,presupuestos,hashes y vídeos están registrados.
2. Completar los traspasos VDP1/VDP2 entre escenas sobre CD/SMPC/reloj compartidos.
3. Conectar la detección4MiB ya probada al presupuesto dinámico; Royal usa dos builds por macros y no
   cabe sumar sus máximos estáticos en la Saturn base.
4. Frontend completo y escenas gráficas integradas: estado original persistente
   de créditos/start/modo/roster, selección de compañero TAG, flash original,
   imágenes/posición separadas de Demolition en ROYAL y continuidad RNG.
   El terminal de Intro R04 no define créditos ni transición automática a MODE.
   Eventos Presentadora conservados; contrato prepare/start/stop y registros
   de Entrada OPEN. Las semillas de fixtures no justifican un arranque real.
5. TAG/Royal con todos los participantes,acciones,IA y transiciones originales;
   resultados,progresión,continues,records,rankings y finales.
6. Disco universal del juego completo;Mednafen1.32.1 None/4MiB,mando,vídeos y todas las métricas
   pedidas. Las tasas históricas no son medidas de este nuevo core.

Sin sonido en producción. Reconstrucciones de referencia pueden conservar el
código original para auditoría; no se promueven como beta de esta integración.

## Reproducción del checkpoint

Desde este directorio: `WF_ROM=/ruta/wwfwfest.zip make test sanitize`.
`make test` sin WF_ROM omite explícitamente cinco tests que requieren la ROM.

En Linux con Python3,make y SDK suministrado:
`YAUL_INSTALL_ROOT=/ruta/sh2eb-elf make sh2`.
El resultado es `build/sh2/libwf_full_game_core.a`, no un disco.

Para repetir los diferenciales originales, usar un venv con Unicorn2.1.4 y
compilador host/liblz4 disponibles:
`python tools/run_reference_checks.py --rom /ruta/wwfwfest.zip --out /ruta/nueva`.

Para restaurar los seis ZIP a una copia nueva:
`python tools/restore_packages.py --packages /ruta/paquetes --out /ruta/nueva`.
Los binarios protegidos y la ROM permanecen externos a Git.

Revisión menor pendiente: pruebas adicionales de confinamiento del recuperador.
Su publicación por renames no es atómica ante un error de disco; toda salida de
una ejecución fallida debe considerarse incompleta y revisarse antes de usarla.
