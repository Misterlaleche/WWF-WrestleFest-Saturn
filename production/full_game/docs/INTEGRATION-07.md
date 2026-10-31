# Integración07 — temporización, escenas y combate

**Goal:** corregir retraso de arranque06 y continuar la misma sesión a las
escenas aprobadas y al runtime de combate, preservando estado original.
**Architecture:** un main/CD/SMPC/FRT; adaptadores de transporte separan cargas
permitidas por SPEC6 de ticks originales. Sin reiniciar deuda en una escena.
**Spec:** ../SPEC.md. Base:a295ce6026891a6b94ae6f55c3e27a3cf24aafd0.
**Execution:** superpowers:executing-plans, inline y una revisión final independiente.

No preguntas rutinarias: usuario autoriza seguir y SPEC15 da autonomía.
Rama y checkout actuales aislados de originales; conservar06 y sus evidencias.

## Investigación medida

Muestras06: BOOT$264E2 añade41 ticks, $26844 añade24, $26772 añade12.
Espera$1E7C:384 actualizaciones/384 campos, deuda disminuye16.
SETUP$2A88 acumula7–10 ticks por carga de pantalla. El camino de paletas
publica1024 escrituras individuales, cada una recorre el bus genérico validado.
Hipótesis: transporte escalar y preparación sin barrera de carga, no reloj.

## Tareas

- [x] Transporte rápido de escrituras simples con prueba diferencial frente
  a wf_r18_video_batch original: bytes, dirty, errores y atomicidad idénticos.
  Modificar sólo build_session.py y nuevos src/wf_video_bus.c/tests; propietarios
  originales intactos. Medir misma ruta Saturn; no aceptar deuda descartada.
- [x] Presupuestar preparación oculta: usar barreras explícitas sólo donde
  SPEC6 permite carga; conservar residuos/fracción y deuda previa. Pruebas de
  reloj con carga, reanudación, errores y sin ticks visibles añadidos.
- [x] Adaptar handoff original AC0: registrar puntos/IDs/RNG de entrada y
  presentadora; comparar adaptadores aprobados con RAM/contexto vivo. No
  sustituir callbacks por éxito ficticio ni fabricar equipos.
- [x] Conectar renderer/recursos de escenas al main compartido; preservar
  última salida, latencia de sprites y reloj57.44485. Mednafen con mando.
- [x] Conectar F8C y primer flujo de gameplay con sus bindings/cachés reales.
  Verificar memoria sin cartucho y4MiB, errores de recursos, estados activos.
- [x] Suite host/ASan/UBSan, comparación visual y temporal, revisión final,
  estado honesto y checkpoint publicado con recursos externos conservados.

## Review Focus

Fast path no debe omitir dirty, corrupción/validación ni callbacks de ring.
Cargas no deben borrar deuda existente ni cambiar los fundidos originales.
Escenas no deben duplicar RNG/ticks al delegar un propietario ya ejecutado.
No sobrescribir PROGRAM/tabla-ROM aún viva al cargar texturas de combate.
No confundir primer gameplay con TAG/ROYAL/progresión completos.

## Avance medido

- Fast palette bus: diferencial completo de estado/dirty/errores/ring PASS
  con ASan/UBSan. RED conservado en work/session07/video-bus-red.log.
- Pacer: cargas anidadas, fase/deuda previa, wrap físico y waits inválidos
  RED→GREEN. RESET inicia reloj en reset_ready original, sin tick fabricado.
- Mismo recorrido de mando en disc02: max deuda3 (06:101), AC0 deuda0.
  Sólo clears síncronos y bind26E66 son cargas; los fades y waits siguen activos.
- Atlas antiguo falla en Entrada: tile48785, error8 en7CDE. Atlas por escena
  front3115/entry776/ring1452: ruta acreditada None llega F8C sin error.
- Join de combate: 36 bindings, transportes mundo, cámara y RAM viva,
  runtime completo preservado byte a byte; ASan/UBSan PASS.
- Build completo sin LTO excede HWRAM221568B. Se conserva LTO hasta el enlace
  final (como Yaul aprobado), y workspace mutuamente excluyente front/combate.
  Staging original de texto usa scratch frontend antes de F8C y rango LWRAM
  reservado después. No lectura de PROGRAM tras entregar sus rangos al cache.

Ruling: usar propietarios vivos7B70/A654 con texturas derivadas del decoder
aprobado, sin ejecutar además wp01/wp02 — evita duplicar RNG/esperas; la
comparación visual de las rutas aprobadas sigue siendo evidencia necesaria.
Ruling: la ruta automática puede entrar AC0 sin esperar una nueva muestra host
TITLE — la prueba mantiene START físico tras crédito, respetando aceptación
original. No se añade demora de título para facilitar el harness.

- Combate09: TAG None y Royal4MiB llegan180 actualizaciones/180 ACK, con mando,
  sin fallos de propietarios. Captura revela rastros VDP1; modo de borrado
  del backend sustituido por clear completo aprobado, medido en próxima pasada.
- El primer enlace entra en0x060F5434 (43KiB libres); workspace excluyente,
  pool de índices81.920B y14 páginas extra por modo, con fallback CD exacto.
  LTO por sí solo no reduce el límite; -Os y leases sí. Rendimiento de combate
  no declarado resuelto:09 añade92 ticks TAG y338 Royal entre frames10–180.
- Royal4MiB usa ahora ROYRAW exclusivamente después de la validación completa
  de cartucho; None/1MiB conservan el formato comprimido y fallback CD.
- Prueba RED combate: disc04 llegaF8C y no produceframes, timeout reproducido.
  Join host conserva todo el runtime salvo sus bindings; ASan/UBSan PASS.

Alcance de cierre07: arranque sin deuda persistente, escenas y primeros frames
TAG/Royal verificables. Progresión, resultados y retorno del attract/créditos
con combate activo todavía requieren integrar los owners postmatch. No se
marca beta completa ni cadencia de combate como resuelta.

## Revisión y cierre parcial

Revisión GPT-6 Astra/high sin Critical/Important/Minor confirmados; condición
borrado/disc11 resuelta con TAGNone yRoyal4MiB PASS e inspección de capturas.
Asserts de offsets y capacidad añadidos; recompilación byte-idéntica al programa
90bfdda228c53cd19391e4c9641589895af7d75bea7cf9ca41791345502bc59b.
Final: Ruling: sprites/timing completos de escenas quedan abiertos — este
checkpoint publica integración y evidencia parcial; coste: aún no certifica
fidelidad completa de las escenas aprobadas.
Final: Ruling: cadencia de combate pendiente — se conserva y publica toda
la deuda; coste: combate lento, especialmente Royal sin expansión.
Final: Ruling: resultados/progresión/retorno attract y créditos activos quedan
para la siguiente integración — fallo explícito al límite; coste: recorrido
incompleto, no apto para una partida completa ni beta final.
Final: Ruling: audio/multitap/prueba física pendientes — alcance parcial
verificado en emulador; coste: esas funciones y hardware no acreditados.
Final: Ruling: conservar rama y publicar checkpoint, sin borrar checkout ni
planes/evidencia — SPEC12/15 y autorización del usuario prevalecen sobre menú
rutinario de finishing-a-development-branch; coste: almacenamiento adicional.

- Final11-attract PASS sin crédito/START: BOOT542ticks con máximo3 pendientes;
  TITLE original posterior acumula máximo76 y no se declara corregido.
- Final11: TAGNone/1MiB yRoyalNone/4MiB PASS180frames/ACK; selección sin deuda
  persistente. RoyalNone termina con582 pendientes;4MiB360;TAG92.
- Borrado inspeccionado en capturas11 limpio. Fondo Presentadora39950/39950;
  Entrada52/58012 diferencias con captura asíncrona, conservadas como OPEN.
