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
