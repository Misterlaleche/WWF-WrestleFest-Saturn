# WWF WrestleFest — Audio 2 — entrega independiente

**Estado: desarrollo verificable; NO PRODUCTION READY. HARDWARE PENDING.**

Esta carpeta consolida y corrige Audio 2 desde `audio-2` / `db18be40a1d79de200408207573e9f45bf73bf2c`. El nombre de la rama `audio-2-production-ready` identifica la rama de entrega solicitada; no certifica su estado. R04 y las implementaciones históricas permanecen intactos.

## Entrada para Codex

1. Leer [CODEX-INTEGRATION.md](CODEX-INTEGRATION.md): API, propiedad del slave, publicación de comandos, parada y recuperación.
2. Leer [AUDIO-FINAL-AUDIT.md](AUDIO-FINAL-AUDIT.md): diferencias entre implementación, pruebas de lógica, ejecución nativa, fidelidad audible y aceptación pendiente.
3. Usar `tools/generate_resources.py` con el ZIP original del propietario. No hay ROM ni muestras incluidas.
4. Reproducir `tools/run_host_tests.py`, la referencia actual `tools/reference/reproduce_full_board_references.py`, la comprobación de inicios `tools/reference/verify_full_board_sfx.py` y el diagnóstico nativo `tools/build_saturn.py` / `tools/run_native_test.py`. `reproduce_all_references.py` conserva los oráculos históricos, con su alcance documentado.
5. Aplicar únicamente la frontera de audio al runtime. No sustituir gameplay, gráficos ni el bucle del juego por `tests/saturn_harness.c`.
6. Para continuar el desarrollo pendiente, usar [NEXT-SESSION.md](NEXT-SESSION.md): candidato exacto, primer experimento de banco activo, reproducción del fallo de fade y regresión requerida.

El motor sintetiza FM con los 32 slots SCSP. El 68000 sirve ADPCM y el DSP mezcla cuatro voces PCM sin consumir otros slots. El SH-2 slave ejecuta el controlador original y traduce sus registros; no enlaza Nuked-OPM. La referencia Nuked se utiliza solamente en el PC para comprobar la fidelidad.

## Lo que contiene

- `include/`: API pública versión1 y adaptador Yaul del perfil estático.
- `src/`: motor, secuenciador, transporte y driver 68000/SCSP; sólo fuentes utilizadas por los perfiles nuevos.
- `tools/`: generador desde ROM, compiladores, captura exclusiva Mednafen1.32.1, comparación sin normalización y comprobaciones.
- `tools/environment/`: restauración del entorno con versiones/hashes, sin paquetes binarios ni BIOS.
- `tools/reference/`: ejecución del Z80 original con Timer A/B, busy y bancos OKI; A53 y A15 conservados como regresiones históricas. Todo se reconstruye con el ZIP original sin buscar entregas antiguas.
- `tests/`: regresiones de colas, plazos, API, recursos, referencias ROM y preparación PCM.
- `docs/`: contratos, investigación y decisiones.

Los ELF, mapas y discos de diagnóstico se generan fuera del repositorio. Contienen datos originales y se distribuyen sólo de forma privada. Las capturas WAV se entregan por separado; en GitHub sólo se conservan código, procedimientos, manifiestos y resultados sin audio protegido.

## Bloqueos que Codex debe conservar visibles

La auditoría del controlador completo encontró reinicios adicionales en 17 comandos por una interpretación incorrecta de `F1`; la prueba de inicios físicos y su corrección están en [F1-ROM-SEQUENCE-AUDIT.md](docs/F1-ROM-SEQUENCE-AUDIT.md). Una segunda diferencia sigue pendiente: el OKI original aplica el banco global a las voces activas, mientras el motor conserva datos del banco de inicio. [ACTIVE-OKI-BANK-AUDIT.md](docs/ACTIVE-OKI-BANK-AUDIT.md) mide una diferencia de señal causada sólo por ese comportamiento.

Además, los ensayos de fade, carga simultánea y combate conservan fallos documentados. La existencia de 18 músicas y 98 comandos ejecutables no equivale a fidelidad o rendimiento aprobados. No sustituir un criterio fallido por el resultado positivo de un perfil anterior.

## Regla de aceptación

Un `RUNNING` sin error no basta. Se requieren cero descartes FM, cero underruns, ausencia de bloqueos y pérdidas, catálogo/transiciones completos, rendimiento de combate con entradas idénticas, comparativas disponibles y aprobación auditiva del propietario. Un resultado de escritorio, una ventana de30s o una ejecución aislada no reemplaza esas pruebas. El hardware físico sigue pendiente hasta la prueba del propietario en Saturn NTSC-U con MODE.
