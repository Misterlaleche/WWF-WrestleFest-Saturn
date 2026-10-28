# Combate con entrada desde 805be2a

> Ejecución solicitada expresamente por el usuario; continuar sin pausas de aprobación.

**Goal:** resolver raw$1C y continuar la ruta con entrada hasta el resultado original; probar SH-2/Mednafen y entregar vídeo, sin BIN/CUE.
**Architecture:** owners C nativos derivados de ROM intacta; oráculo68000 solo HOST. Conservar dispatcher, lectura del mando, vídeo VDP1/VDP2 y resultados originales.
**Tech Stack:** C11, Python/Musashi, SH-2/Yaul, Mednafen1.32.1.
**Spec:** solicitud de este turno y `validation/RUNTIME18-ACTIONS3F5F-COMBAT-CHECKPOINT.md` (base805be2a).

## Restricciones

ROM SHA1 a726c46f350723ea73158bc78055d17d418b14e2. `sources/` solo lectura. Sin frontend/audio, código provisional, atajos de daño/resultado o promesas ROM-EXACT/hardware físico no probadas. Mantener regresión sin entrada3008.

## Revisar y probar

- Anchuras BYTE/WORD, temporizadores y límites de coordenadas.
- Enlaces/objetivos válidos y liberación de ambos actores.
- D0-D7, RNG, RAM completa representada y efectos alcanzados.
- Capacidad real de recursos/SH-2 y lecturas de gameplay.
- Entrada real del mando frente al replay de diagnóstico; mostrar lo que efectivamente se prueba.

## Task 1: Acción1C

- [x] Copiar snapshot natural3373/slot1 inmutable y verificar hash.
- [x] Nuevo `tests/r18/action1c_verify_rom.py` + bridge: RED por owner ausente; ROM12614 sin parches como autoridad.
- [x] Implementar descriptor/callback en `src/runtime/src/wf_r17_actions_r11.c`; GREEN con ramas y continuación natural.
- [ ] Repetir `allies_run_probe.py --attack --all-actors --trace-tiles --require-complete`. Resolver cada siguiente fallo con snapshot y ciclo ROM RED→GREEN; nunca convertir bloqueo en éxito.

## Task 2: Integración y memoria

- [ ] Consolidar demandas reales de tiles/páginas dentro de la capacidad y probar recursos byte a byte.
- [ ] `make -C src/runtime test`, oráculos afectados y recorridos completos normal/ASan con y sin entrada.
- [ ] Revisión independiente; resolver hallazgos materiales con pruebas.

## Task 3: Saturn y vídeo

- [ ] Compilar SH-2 estable con SDK fijado; medir memoria y ejecutar Mednafen.
- [ ] Verificar respuesta de la ruta real del mando y resultado del combate; distinguir reproducción automatizada de una sesión manual.
- [ ] Capturar vídeo a velocidad real, inspeccionar visualmente y verificar RAM/VDP1/VDP2.
- [ ] Informe reproducible, hashes y checkpoint local, entrega sin BIN/CUE.

## Registro

Base limpia805be2a. Bloqueo reproducible previo:3373/slot1/raw1C en dispatch12614. Se mantiene la rama de continuación existente para conservar el checkpoint y sus pruebas. Revisión de aprobación de planes no interrumpe la ejecución ya solicitada por el usuario.

## Progreso demostrado

Raw1C:3305 casos;52:11606;58:2987;6D:5223;09:2485 con snapshots reales727/4519;5D:711. Todos oráculos de ROM intacta, con RAM/FG/D0–D7/RNG; no son partidas Saturn. Ruta principal alcanza4771/raw67, implementación con RED en curso. Ruta adicional con movimiento posterior1000/1100 y ataque1250 alcanza2028/raw75; otro owner en curso.

Prueba SH-2 aislada `work/runtime18/controlled-live-preflight`: enlaza, final RAM060FC1D4. En Mednafen la lectura real del pad solo mantiene una pulsación durante un frame. La biblioteca Yaul fijada demuestra que held=current&(current^previous), pressed=current. Se cambia controls() a pressed después de RED;1244 muestras HOST pasan. Sigue pendiente repetir nativo con el arreglo. Driver envía X keyboard→pad Mednafen→SMPC; jamás escribe RAM; la prueba de reset usa semilla0 y se declarará automatizada.

Capacidad: unión de demandas reales sinentrada3008+principal4771=5451 tiles, supera5120 slots. Solución necesaria en curso: diccionario exacto de bloques32B, metadatos/índices en LWRAM existente y scratch128B, sin gráficos aproximados. Header/capacidades/formato legacy deben conservar tests y fallback. No consolidar hints/HOT antes del recorrido completo.

Los tests anticipados6E/6F son solo derivados y están RED por owners ausentes; no integrarlos ni declararlos demostrados naturalmente si no se alcanzan. Los artefactos y reportes de cada fase se guardan en `validation/results/runtime18/action1c-controlled/`. No reemplazar los vídeos/paquetes anteriores.

## Continuación posterior

Añadidos y GREEN:34(2671),state4/1BE7C(9178),CPU1D6E4(2229),78(4685 con ambos snapshots). CPU1D6E4 reutiliza owner1F45A y corrige preservación del byte alto del target al crear enlace. Revisión cruzada sin hallazgos pendientes de esos owners. No se ha alcanzado todavía un resultado con entrada en este turno.

Últimos recorridos congelados `after78-main`5677 y `after78-late`7845 convergen al guard CPU1C2CC, FLAGS34bit4. Se investiga la conexión al prefijo ya implementado, que selecciona raw79; no retirar otros guards sin evidencia. Raw79 está anticipado únicamente hasta que haya snapshot real postCPU.

La unión real creció a6630 tiles, diccionario32B664992B: supera640KiB. Split32 previo(6395IDs) funcionaba y tenía112slots, pero sus cifras ya no acreditan la unión nueva. En curso R18D16/32 según tamaño real, índices112KiBHWRAM compensados por reducir CDFS4→1 ventana16KiB, presupuesto casi neutral(+60B). Debe volver a pasar codec/ASan/CDFS/SH-2. No se ha tocado aún el HOTSPR canónico R18H4883 ni los hints/páginas206 históricos.

El observador de memoria fue extendido para R18D32 con indicesHWRAM y diccionarioLWRAM; hay que actualizarlo a16/32 y112KiB antes de prueba final. El driver nativo conserva eventos por X keyboard→pad emulado→SMPC, sin escritura de memoria, y el source de controls() usa pressed. Preflight antiguo falla en6D por ser anterior; demuestra el bug de held, no es el nuevo checkpoint ni vídeo final.

## Continuación natural y entrada activa

GREEN posteriores: estado0 FLAGS34.bit4(4404), raw79(7611), raw21(777). Ruta principal con prefijo60/180 alcanza5918/raw2E pendiente. Ruta late-pad completa endpoint14409/result8001 sin bloquear gameplay; falla residencia con assets antiguos (ROM4/Sprites13030), aún no prueba Saturn. Demanda ROM observada completa183 páginas; unión natural207.

Ruta de entrada deliberada al ring (DOWN1000..1049 y A cada60frames desde1200) alcanza1921/raw21 humano0 y1983/raw05 slot1. Se añade variante ring-active-pad con ataques hasta20000 para comprobar entrada durante todo el combate, sin escribir HP, posiciones ni resultado. Raw05 está en RED desde snapshot natural. Monolito reservado al implementador05; raw2E se desarrolla como fragmento independiente.

Demanda total medida al endpoint tardío7414 IDs, diccionario16B666816B, map118624B. La partición antigua640KiB/112KiB no lo contiene: solución medida en curso reduciendo exclusivamente reserva de escena al máximo real114944B y reasignando12032B, con mapa116KiB. No hay consolidación de assets hasta recorrido activo completo y verificación de capacidad.

Raw05 completoGREEN:4578 casos,48ticks naturales hastaestado3,33sitios ydescriptorA1. Ruta ring-active-pad avanza hasta3047/slot4/raw50, snapshot01052; port50enRED. Uniónincluyendoesademanda7945IDs/210páginas, superanlareparticiónsimple: mediciónde4B(63125chunks,252500Bdict,508480Bmapa) con112KiBHWRAM+spill393792BLWRAM cabe646400B redondeados; implementaciónexacta en curso.

La suite general `make-j4test` termina exit0. La regresión histórica ROM encontró enlaceharness faltante de wf_state5_approach_1f45a al mezclarCPUactual con dependenciaf88acad; se corrige la congelación de4scriptsCPUpara llevar state5_exec.c yheaderactuales, conSHAregistrado. No se cambianfixtures niROMpara obtenerGREEN. Reejecución completó1027continuacionesCPU y1317target-ringside; suite aúncorriendo.
