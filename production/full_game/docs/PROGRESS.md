# Ledger — plan: production/full_game/docs/PLAN.md

Base: aedca6827c7801a88dbb1eae280830cd4776b95f.
Ruling: el usuario autoriza investigación, implementación, rama y publicación;
no se solicita aprobación rutinaria de diseño/plan.
Ruling: los recursos aprobados no están en Git; una reconstrucción con bytes
distintos no puede sustituirlos silenciosamente. Coste: integración visual OPEN.
Pre-flight: reloj y adaptador se compilan en conjunto pero las escenas reciben
una sola llamada por tick consumido; el adaptador no crea temporizador propio.
La regeneración de recursos no altera código ni datos originales.
Auditoría: Intro lee SEQ.DAT externo; Entrada/Presentadora tienen C nativo.
Royal selecciona memoria por compilación; R6 no recuperada. La cadencia del
entry Royal no está desacoplada de VBlank. No promover sus métricas históricas.

Checkpoint 01: reloj y adaptador pasan pruebas C11 y ASan/UBSan; 53 recorridos.
20 casos/24922 ticks de Presentadora comparados con ROM mediante Unicorn2.1.4.
SH2 GCC13.2.0 compila y enlaza relocatable los cuatro módulos; no hay main/disco.
Revisión independiente: sin defectos bloqueantes en reloj/adaptador para este
alcance parcial. Eventos de Entrada OPEN documentados (P2 corregido).
Minor deferred: pruebas de confinamiento offline adicionales del recuperador.
Minor deferred: publicación de recursos no atómica ante errores del filesystem;
una ejecución fallida no es una recuperación utilizable, aunque existan archivos.

2026-10-31: el usuario entrega los cinco ZIP aprobados en Downloads. Todos sus
SHA256 coinciden con la autoridad. Restaurados en work/upstream_rebuilds,
sin modificar Git ni paquetes originales. 19/19 recursos visuales verificados.
Se levanta el bloqueo de recursos de escenas/TAG/Royal. R6 sigue no recuperada.
Siguiente: reconstruir cada fuente aprobada con el SDK fijado; después adaptar
plataforma/render/carga sobre este núcleo sin bucles main duplicados.

Checkpoint02: recuperación TEST03 adicional, manifest30/30; reconstrucción de
los seis bloques completada (7 discos contando Royal base/cart). Cuatro programas
de escenas/frontend idénticos a referencias. Probes Royal13/13 contratos observados.
Royal Mednafen1.32.1 None/4MiB:smoke180campos por perfil,176draws,cero fatales,
MOV original; conversión MP4 de la pasada base conserva todos los PTS.
Pasadas largas completadas:8474 ticks hasta GAME OVER,actor0..8 y RNG idénticos
a HW1,CDdelta0,fatal0. Media50,5904base/58,8948cart;objetivo base todavía incumplido.
Ventanas60/99 y1/5s cuantizadas medidas;PTS de cada fotograma MOV/MP4 idénticos.
El analizador nuevo de ventanas99 coincide con el original. No se usa wall-time.
Fuentes R3 completas370 y TEST03 guardadas bajo vendor; no recuperación R6.

Checkpoint03: plataforma única VBlank/SMPC y reloj racional,
detección SDK y validación de16 puntos sobre ocho bancos antes de usar4MiB.
El renderer TEST03 se extrae de una fuente fijada por hash; main/reloj/input
privados excluidos; MODE reutiliza el almacenamiento del fondo ROSTER (122880B).
Ruling: se rechazan códigos de dirección>10 antes de indexar las tablas; la
semántica ROM de entradas contradictorias sigue OPEN, coste: error explícito en
ese input en lugar de lectura fuera de límites. No inventar normalización.
Ruling: el alcance del disco03 es MODE/ROSTER con terminal de selección; la
identidad ROM, formación de equipo y siguiente escena requieren sus propietarios.
No se reutiliza el slot visual como ID ni se anuncia beta completa.
Revisión independiente encontró dos P2: SMPC consumía el flanco de A durante
scroll y el runner ignoraba muestras incoherentes. El primero se reprodujo
con mando en Mednafen (scroll-edge-red: timeout, sigue ROSTER). Corregido
manteniendo la frontera original de polling. El segundo tiene tres tests RED→GREEN;
se añadió secuencia de publicación par/impar y rechazo de violación del reloj,
contando sólo publicaciones distintas en escenas activas. Cuatro pasadas finales PASS: None,1MiB,4MiB y flanco durante scroll.
247/264/246/200 publicaciones distintas activas, sin violación del reloj;
deuda máxima observada2, sin descartar ticks. Suite host con ROM9testsPython,
C11 y ASan/UBSan de reloj/adaptador/cartucho/frontend PASS.
Final: fixed input boundary — Mednafen scroll-edge RED→GREEN y las tres
rutas normales PASS. Final: fixed evidence gate — tres regresionesPython
RED→GREEN y muestras nativas con secuencia coherente. No quedan P2 abiertos
de esta revisión del alcance03. El resto del juego sigue OPEN.

Checkpoint04: único CD/FRT; CDFS y transporte CD32 HW1 copiados sin cambios
desde fuentes bloqueadas por SHA. Siete recursos TEST03 pasan al CD y se
precargan en130848B de LWRAM antes de MODE. Tamaños y capacidad se comprueban
antes de leer; CRC32 antes de publicar ningún puntero. Build valida SHA256.
Fin de estáticos HWRAM:0x06064314; ahorro105504B respecto03.
RED→GREEN de cargador y conversión de identidades; C11/ASan/UBSan PASS.
Oracle ROM: diez conversiones nativas iguales a $5DA0/$64CC. Nueve fixtures de
confirmación,1500 escrituras de paleta y cuatro setups originales documentan
los siguientes propietarios, pero no equivalen a su integración nativa.
Ruling: no inventar transición Intro→MODE, semilla de arranque, compañero ni
equipo CPU. El handoff real conserva16KiB RAM, registros y control/IRQ;
el terminal MODE/ROSTER continúa parcial. TAG requiere compañero y ROYAL
requiere corregir el transporte de imagen/posición de slot9.
Cuatro rutas normales Mednafen1.32.1 PASS: None,1MiB,4MiB y A durante scroll.
247/261/246/203 publicaciones activas distintas, deuda máxima2, sin violación;
contador CD permanece96 desde MODE hasta confirmación. Quinto disco negativo:
un byte cambiado enSELPAL.BIN, CRC de último recurso falla, no entra MODE y
los siete punteros quedan cero. Disco bueno y programa idénticos en las cuatro
rutas normales. Cinco MP4 conservan todos los PTS de sus MOV nativos.
Retrato terminal comparado con03: RGB idéntico, sin extrapolar a la animación.
Suite host con ROM9testsPython y originales2023/2023 PASS. Revisión independiente
sin defectos accionables en este alcance. No hay validación física, combate ni
prueba de todos los fallos posibles del CD; fases completas siguen OPEN.

Checkpoint05: wf_session compone los propietarios HW1 exactos con una única
RAM16KiB, D/A y contexto de control desde RESET654 hasta AC0/F8C. Diez
recorridos completos del alcance pasan normal y ASan/UBSan: TAG/P2,
Demolition/P2, Royal0/9, START durante attract, semilla alternativa, eventos
agrupados y error de recurso. Inicialización exige RAM inicial y callbacks
explícitos; no hay selección, créditos ni semilla inyectados por producción.
RED: módulo ausente; GREEN: contratos de init, rechazo de solapamiento/entrada
temprana y fallo terminal sin repetición. El harness sólo genera inputs/eventos;
los propietarios originales producen participantes, equipos, RNG y flags.
408 fixtures crédito/START comparadas con68000: RAM persistente, D/A y carry
PASS. No extrapolar estos helpers a equivalencia ROM de toda la ruta.
Revisión final por GPT-6 Astra: dos P2 de validación, sin bloqueo del estado
de producción en el alcance. Final: fixed DemolitionP2 expectation — RED
de auditoría esperaba10/11; la ROM normaliza11/10 desdeP2. Se corrige sólo el
harness y se añade la ruta a normal/ASan/UBSan: GREEN. Final: fixed SH-2
symbol validation — ld-r retenía símbolos sin relocaciones de secciones
descartadas; objcopy elimina los innecesarios conservando la API. Build final
PASS,183464B, sólo memcpy/memset pendientes. Regresión de archivo contaminado:
miembro obsoleto descartado y SHA256 final idéntico. No P2 de revisión05 abiertos.
Suite completa con ROM: C11/ASan/UBSan,53 recorridos prematch y9testsPython PASS.
Originales2023/2023 intactos. Logs, configuración y hashes en evidence/session05;
snapshots de RAM se conservan en el paquete externo, sin ROM/BIOS.
Ruling: núcleo05 todavía no se conecta a renderer/IRQ/cold-state de Saturn;
el disco04 permanece como último visual. No se entrega un disco05 ni nuevos
vídeos. Coste: falta completar la plataforma/escenas antes de validar el flujo
con mando real emulado. Gameplay/progresión, presupuesto dinámico y beta OPEN.
