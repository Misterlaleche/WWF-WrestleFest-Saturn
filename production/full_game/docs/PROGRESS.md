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
