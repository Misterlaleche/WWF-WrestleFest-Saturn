# Estado de integración — 2026-10-31

Estado: CHECKPOINT05 PARCIAL, no beta completa.
El nuevo núcleo conserva el estado original desde RESET hasta la entrada F8C
de combate. Está probado en host y compilado para SH-2; todavía no está unido
al renderer y a los eventos reales de Saturn. No hay un nuevo disco05.
El último disco visual sigue siendo04: MODE/ROSTER en Mednafen1.32.1
None/1MiB/4MiB, con final al confirmar un luchador y sin combate.

## Nuevo en checkpoint05

- Una sola RAM de16KiB, registros y contexto de control desde RESET654,
  moneda, START, modo, roster y compañero hasta AC0 y preparación original F8C.
- Propietarios HW1 originales enlazados sin cambios. El adaptador exige RAM
  inicial y eventos de plataforma explícitos; no inventa semilla ni equipos.
- Diez recorridos con ROM externa pasan en host normal y ASan/UBSan: incluyen
  TAG/P2, Demolition/P2, ROYAL/Crush, cancelación de attract por START, otra
  semilla de fixture, eventos agrupados y fallo terminal de recurso.
- El orden Demolition desdeP2 es11/10, como produce la normalización original.
  Los actores sólo se importan después de F8C; el fallo no repite efectos.
- 408 comparaciones directas de crédito/START contra ROM68000: RAM persistente,
  D/A y carry. No es un diferencial de todo el recorrido.
- Núcleo SH-2 GCC13.2.0 enlazado:183464B, sólo memcpy/memset pendientes de libc.
  Reconstrucción comprobada con un miembro obsoleto insertado en el archivo:
  queda únicamente el objeto del núcleo y el mismo SHA256 final.
- Suite completa C11/ASan/UBSan y9testsPython con ROM PASS; originales2023/2023
  intactos. Dos hallazgos de revisión resueltos con sus regresiones.

Reproducción, hashes, evidencias y límites: docs/RECOVERY-05.md.
Pendiente conectar la sesión a bus/IRQ/renderer Saturn y fijar el origen de la
RAM inicial. No hay nuevas pruebas visuales, de rendimiento ni de hardware05.

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
4. Conectar la sesión original05 al frontend y escenas gráficas de Saturn:
   bus/IRQ/reloj, fuente de RAM inicial, selección de compañero TAG, flash
   original e imágenes/posición separadas de Demolition en ROYAL.
   El núcleo05 ya conserva créditos/start/modo/roster/RNG y prepara F8C con
   eventos de test declarados; la integración visual sigue pendiente.
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
`make test` sin WF_ROM omite explícitamente los tests y recorridos que requieren ROM.

En Linux con Python3,make y SDK suministrado:
`YAUL_INSTALL_ROOT=/ruta/sh2eb-elf make sh2`.
El resultado es `build/sh2/libwf_full_game_core.a`, no un disco.
El nuevo núcleo de sesión se construye por separado con
`YAUL_INSTALL_ROOT=/ruta/sh2eb-elf python3 tools/build_session.py --sh2 --out /ruta/nueva`;
ver docs/RECOVERY-05.md. Tampoco es un disco.

Para repetir los diferenciales originales, usar un venv con Unicorn2.1.4 y
compilador host/liblz4 disponibles:
`python tools/run_reference_checks.py --rom /ruta/wwfwfest.zip --out /ruta/nueva`.

Para restaurar los seis ZIP a una copia nueva:
`python tools/restore_packages.py --packages /ruta/paquetes --out /ruta/nueva`.
Los binarios protegidos y la ROM permanecen externos a Git.

Revisión menor pendiente: pruebas adicionales de confinamiento del recuperador.
Su publicación por renames no es atómica ante un error de disco; toda salida de
una ejecución fallida debe considerarse incompleta y revisarse antes de usarla.
