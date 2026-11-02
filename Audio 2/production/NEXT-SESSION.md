# Continuación exacta de Audio 2

**NOT PRODUCTION READY · HARDWARE PENDING · aprobación auditiva pendiente.**

## Alcance confirmado por el propietario

Continuar sólo el audio aislado. El propietario quiere escuchar los audios y
ha reservado la integración en ROM Graphics Final a Codex. No modificar
TAG, Royal, gameplay, gráficos, frontend, WP1979/WP1980 ni otras escenas.
No volver a pedir autorización para decisiones técnicas dentro de Audio 2.

Trabajar sobre `audio-2-production-ready`; leer `ESTADO.json`,
`AUDIO-FINAL-AUDIT.md` y `CODEX-INTEGRATION.md`. R04/A53, `audio-2` y los
experimentos anteriores se conservan. La copia local recuperada es parcial:
para publicar, usar el árbol remoto real; no empujar un historial local que
borre archivos no recuperados. No fusionar a `main`.

## Candidato seleccionado

| Elemento | Identidad o contrato |
|---|---|
| Firmware 68000/SCSP, sin cambio | `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090` |
| `src/reference_driver/z80-native.c` | `d9bd93a7cfdf8e5dbb022bce6df1dc132e6caf7e9c3c05a18e75e2debd9602e0` |
| `src/wf_music_seq.c` | `8d833f42349267376e0d29ff5b2c0db0fa31af5c544252e7306e87ec7bfd66bc` |
| `src/wf_music_module.c` | `4514091b2ce9019b6ba7ed62ea8fcf65d9c658a6a857bae498a6a2fbc0052b69` |
| Compilación SH-2 | Núcleo Z80 `-O2`; secuenciador `-Os` |
| API / mailbox / módulo de juego | API1 / mailbox ABI3 / módulo dinámico ABI4; contratos distintos |
| Código caliente + estado | 15.008 + 3.220 = 18.228 bytes; límite18.432, margen204 |
| Recurso de efectos | 440.710 bytes; generado desde el ZIP original, fuera de Git |
| Emulador | Mednafen1.32.1 independiente, relojes sin modificar |

La identidad larga correcta del módulo está también en `ESTADO.json` y en
los manifiestos. El secuenciador conserva grant512, presupuesto global4096,
lead4096Fs, FIFO/rings y plazos originales. No seleccionar la ventana
experimental de24KiB, el DSP ZOH ni los experimentos negativos por su nombre.

## Avances que no deben investigarse desde cero

La causa del fallo `1F` aislado está corregida en `d9bd`: lifts exactos de
CPU/RAM/ciclos y accesos TL, retorno F5F, predicción de cuota `08C1` y servicio
anticipado que evita fragmentarla. La elisión de cálculo temporal para
escrituras que el mapper ya ignoraba conserva literalmente su salida.
`docs/NATIVE-ZERO-DROP-MUSIC-D9BD.md` documenta el contraste causal y sus límites.

Ya pasan14 grupos host, con18×(600s+8s), y las18 capturas nativas cortas de30s+8s
más la transición de9 comandos. Los fades activos de05/08/09 a1s también pasan.
05/08 ejecutan después su final natural FB; no atribuir ese cierre sólo al fade.
La matriz nativa larga también pasa:18 ×(600s+8s) y transición9/9, todos los
contadores de error a cero. El registro está en
`docs/evidence/native-music-d9bd-complete600s.json`.

Las partituras originales ya están observadas con el modelo full-board:
05/08/09 tienen final natural;12 músicas presentan recurrencias visibles;
0B/0C/11 tienen ciclos por canal con longitudes distintas. El análisis no
establece un período PCM global ni un bucle de señal nativa exactamente igual.
Consultar `docs/ARCADE-COMPLETE-MUSIC-STRUCTURE.md` y su generador, que prueba
que la observación no altera CPU, RAM, entregas, MMIO ni PCM.

## Primer bloqueo nativo: dos avisos de mezcla

El replay aislado actual de138s entrega64/64 palabras originales, con cero
pérdidas FM, underruns, rechazos y errores worker/API, pero dos avisos tardíos.
Es FAIL. Es una secuencia de comandos observada, no una ejecución de combate.
La investigación debe localizar el servicio exacto que pierde el plazo,
conservar el coste de la instrumentación y no ampliar buffers ni tolerancias.

## Segundo bloqueo: banco OKI activo

El defecto nativo está demostrado:2.663 muestras decodificadas y14.680 frames
de mezcla cambian al conservar el banco de inicio de una voz. El banco
original es global y afecta a las voces activas. No repetir esa investigación.

El prototipo host ya existe en `tests/oki_address_replay/` y se reproduce con
`tools/reference/probe_oki_address_replay.py`. Resuelve las517.450 direcciones
requeridas, incluidas337 bytes ausentes en el pack anterior; comprueba estados
ADPCM, checkpoints, paridades, reparación de rings y el replay causal de64
comandos. **No está integrado en el68000.** No etiquetar sus pruebas host como
una corrección nativa. Leer su README y `NATIVE-2048-READ-ONLY.md` antes de elegir
una reserva o modificar el DSP.

El siguiente experimento debe medir resolución de dirección y estado ADPCM
nativos, junto con el momento de cambio y reparación de PCM ya preparado.
Unificar la ruta musical fechada y la ruta SFX inmediata. Cambiar sólo `s->bank`
no corrige muestras escritas. La ruta inmediata no garantiza92,88ms de aviso;
esa ventaja sólo existe para eventos musicales preparados con antelación.

El mapa WFBA requiere704 bytes y cabe en la ventana del pack, pero eso no
certifica coste ni ABI de runtime. El ring actual representa hasta512ms a8kHz;
el lead musical4096Fs representa92,88ms a44,1kHz. Reducir rings a2048 o cambiar
el DSP sólo puede seleccionarse después de probar direcciones, fase, sentinel,
frontera de lectura/escritura, memoria, linker y plazo. No hay permiso para
ocultar una pérdida aumentando tolerancias ni omitiendo trabajo original.

## Reproducción de músicas y capturas largas

Después de restaurar el entorno y generar los recursos como indica
`CODEX-INTEGRATION.md`, usar directorios de salida nuevos:

```bash
python3 tools/run_host_tests.py \
  --rom-zip "$WF_AUDIO_ROM" --resources "$WF_AUDIO_BUILD/resources" \
  --firmware "$WF_AUDIO_BUILD/firmware/SCSP.ELF" \
  --out "$WF_AUDIO_BUILD/host-d9bd-600" --seconds 600

python3 tools/run_music_matrix.py \
  --resources "$WF_AUDIO_BUILD/resources" \
  --firmware "$WF_AUDIO_BUILD/firmware/SCSP.ELF" \
  --sdk "$WF_AUDIO_ENV/sh2eb-elf" \
  --environment-root "$WF_AUDIO_ENV" --bios "$WF_AUDIO_BIOS" \
  --reference-report "$WF_AUDIO_BUILD/full-board/reference-report.json" \
  --out "$WF_AUDIO_BUILD/native-d9bd-600" \
  --score-seconds 600 --tail-seconds 8 --transitions \
  --presentation-speed 4 --displays 94,95,96,98 --wall-seconds 1800 \
  --profile-label cddd-exact-TL-return-timestamp-early08c1-d9bd93a7
```

Las opciones llevan un espacio entre nombre y valor; las variables deben
apuntar a recursos locales reales del propietario. `--presentation-speed 4`
es pacing upstream, no sobreaceleración de Saturn. La comparación de pacing
conservó2.646.000 valores PCM idénticos; puede usarse velocidad1 con un timeout
adecuado. No confundir tiempo de pared con tiempo emulado.

El test largo R01 conserva el MMIO histórico TimerB. La referencia full-board
incluye TimerA/B, busy/IRQ y banco global. Mantener esta diferencia de alcance:
igualdad R01 no implica igualdad temporal completa con la placa original.

El exportador `tools/prepare_music_audio_pairs.py` debe recibir una matriz
larga terminada y aprobada. Extrae el WAV real de Mednafen y genera la referencia
original con las admisiones observadas, sin normalizar ni ajustar la señal
por correlación. Los archivos protegidos son privados. Las identidades y
los manifiestos se pueden publicar; las muestras, estados y trazas crudas no.

## Regresión e integración posterior

Después de cada cambio de runtime: pruebas diferenciales,98 órdenes contra
inicios físicos originales, decoder completo, banco activo, matriz nativa,
mezcla, presión/recuperación y carga corrupta. Conservar cualquier fallo
anterior a un reinicio. Recompilar todos los objetos afectados; ningún PASS
histórico acredita un ELF nuevo.

Codex deberá integrar y repetir las parejas TAG/Royal con entradas idénticas.
El perfil histórico `c408` fallaba rendimiento/plazos; no se ha reejecutado
combate con `d9bd` durante esta continuación. Royal todavía necesita una cinta
de entradas por llamada: conservar política/estado/productores no sustituye
esa prueba. El cartucho4MiB sigue siendo opcional, nunca requisito de audio.

La aprobación auditiva y Saturn NTSC-U con MODE permanecen pendientes. No
convertir métricas estadísticas, silencio final ni contadores limpios en
aprobación de timbre, tempo o señal completa.

## Preservación y publicación

No sobrescribir salidas, capturas ni experimentos negativos. Los controles
que fallaron y las interrupciones de infraestructura conservan sus hashes.
La publicación sólo modifica `Audio 2/production/`, con commits identificables
y el PR en draft, sin merge.

La revisión automática bloqueó una entrega privada por faltar autorización
explícita del destino. Primero preparar ZIP y manifiesto concretos; después
pedir únicamente esa autorización. El sí a corregir audio aislado no autoriza
ese destino ni supone aprobación auditiva. No cambiar de medio para eludir
el bloqueo. No afirmar que se sigue trabajando cuando no haya sesión activa.
