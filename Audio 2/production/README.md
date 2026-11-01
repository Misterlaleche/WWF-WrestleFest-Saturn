# WWF WrestleFest — Audio 2 — entrega independiente

**Estado: desarrollo verificable; NO PRODUCTION READY. HARDWARE PENDING.**

Esta carpeta consolida y corrige Audio 2 desde `audio-2` / `db18be40a1d79de200408207573e9f45bf73bf2c`. El nombre de la rama `audio-2-production-ready` identifica la rama de entrega solicitada; no certifica su estado. R04 y las implementaciones históricas permanecen intactos.

## Entrada para Codex

1. Leer [CODEX-INTEGRATION.md](CODEX-INTEGRATION.md): API, propiedad del slave, publicación de comandos, parada y recuperación.
2. Leer [AUDIO-FINAL-AUDIT.md](AUDIO-FINAL-AUDIT.md): diferencias entre implementación, pruebas de lógica, ejecución nativa, fidelidad audible y aceptación pendiente.
3. Usar `tools/generate_resources.py` con el ZIP original del propietario. No hay ROM ni muestras incluidas.
4. Reproducir `tools/run_host_tests.py`, la referencia autocontenida `tools/reference/reproduce_all_references.py` y el diagnóstico nativo `tools/build_saturn.py` / `tools/run_native_test.py`.
5. Aplicar únicamente la frontera de audio al runtime. No sustituir gameplay, gráficos ni el bucle del juego por `tests/saturn_harness.c`.

El motor sintetiza FM con los 32 slots SCSP. El 68000 sirve ADPCM y el DSP mezcla cuatro voces PCM sin consumir otros slots. El SH-2 slave ejecuta el controlador original y traduce sus registros; no enlaza Nuked-OPM. La referencia Nuked se utiliza solamente en el PC para comprobar la fidelidad.

## Lo que contiene

- `include/`: API pública versión1 y adaptador Yaul del perfil estático.
- `src/`: motor, secuenciador, transporte y driver 68000/SCSP; sólo fuentes utilizadas por los perfiles nuevos.
- `tools/`: generador desde ROM, compiladores, captura exclusiva Mednafen1.32.1, comparación sin normalización y comprobaciones.
- `tools/environment/`: restauración del entorno con versiones/hashes, sin paquetes binarios ni BIOS.
- `tools/reference/`: referencia A53 y modelo A15 reconstruibles con un único ZIP original; no requieren buscar entregas antiguas.
- `tests/`: regresiones de colas, plazos, API, recursos, referencias ROM y preparación PCM.
- `docs/`: contratos, investigación y decisiones.

Los ELF, mapas y discos de diagnóstico se generan fuera del repositorio. Contienen datos originales y se distribuyen sólo de forma privada. Las capturas WAV se entregan por separado; en GitHub sólo se conservan código, procedimientos, manifiestos y resultados sin audio protegido.

## Regla de aceptación

Un `RUNNING` sin error no basta. Se requieren cero descartes FM, cero underruns, ausencia de bloqueos y pérdidas, catálogo/transiciones completos, rendimiento de combate con entradas idénticas, comparativas disponibles y aprobación auditiva del propietario. Un resultado de escritorio, una ventana de30s o una ejecución aislada no reemplaza esas pruebas. El hardware físico sigue pendiente hasta la prueba del propietario en Saturn NTSC-U con MODE.
