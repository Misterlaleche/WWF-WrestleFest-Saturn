# RUNTIME-18 — trabajo gráfico y resultado dentro del ring

Continuación de `f61dc98`. Arranque directo con mando, sin frontend ni audio.
Se reduce trabajo de los SH-2 conservando la composición original, y el final
del combate muestra el mensaje elegido por la ROM dentro del ring.

## Cuellos medidos y cambios

- El lector conserva 64 líneas de 32 bytes de metadatos ROM inmutables en
  HWRAM. Cada línea permanece dentro de una página de 1 KB; su carga no pide
  una página CD adicional. Init y ambas precargas invalidan las cachés.
- El parser conserva ocho bodies decodificados, con hasta 32 filas cada uno.
  La caché ocupa 1636 bytes y usa dirección, header y tratamiento del prefijo
  como clave. Coordenadas, espejado, clipping, paletas, referencias, orden y
  efectos D6/D7 se calculan con el estado actual. Bodies mayores y fallos
  siguen el decoder original. No se guardan comandos finales del luchador.
- VDP2 mueve el ring al cambiar el scroll. Si sus coordenadas de tiles y
  overlays permanecen residentes, se evita recorrer los 1024 nombres de
  patrón de cada capa. Los registros de scroll siguen actualizándose.
- Cuatro máscaras de bancos pendientes evitan volver a comprobar paletas sin
  cambios. Las escrituras reales del bus y `$2AEA` invalidan el banco; el primer
  envío incluye bancos inicialmente a cero. Un aborto de sprites no confirma
  sus paletas. Reset/bind invalida las cuatro ventanas. Son 16 bytes nuevos.

No se omiten fases, frames, comandos, texturas o decisiones de gameplay. Se
conservan el modo VDP1, la cadencia lógica, prioridades, cámara y animaciones.

## Medidas del Saturn emulado

Mednafen 1.32.1, mismos intervalos, entradas y semilla; VBlanks emulados,
no FPS del ordenador con `nothrottle`.

| Secuencia | f61dc98 | Nuevo | Reducción de VBlanks |
|---|---:|---:|---:|
| 30–120: 90 frames | 111 | 99 | 10,8 % |
| 900–1080: 180 frames | 203 | 198 | 2,5 % |

Ambos tramos nuevos requieren 1,1 VBlanks por frame. La partida normal completa
4391 frames en 4919 VBlanks, antes de la presentación final. **No se acreditan
60 FPS sostenidos.** No se extrapola la mejora inicial a toda la partida.

El coste medido del ring baja de 20867 a 7526 ticks FRT/128 en el tramo inicial.
Las llamadas de comprobación FG/BG de paletas pasan de 11182 a 773 ticks;
el profiler nuevo mide el helper de bancos pendientes en la misma frontera.
Los tiempos de presentación están anidados en runtime: no deben sumarse.

## Avance visible y evidencia ROM

Después del endpoint real `$1178/$1186 → $11B6`, se ejecuta la presentación
`$11CE..$122B`: espera 33 VBlanks, borra el texto anterior con `$2503C/$8050`,
espera 33, selecciona `$50` o `$51` según los resultados de los cuatro jugadores
y el byte de progresión, y espera 65. Usa el renderer de texto real.

La derrota muestra **GAME OVER**; la ruta ganadora utiliza **CONGRATULATIONS**
con la excepción de progresión nueve que establece la ROM. Mednafen verifica
131 VBlanks de presentación y el texto visible en el disco normal.

Es un segmento completo de presentación: no se afirma implementar el owner
entero `$11B6`, sus clears previos `$0092..$009E`, ni la transición `$122C+`.
El modo de arranque directo conserva el ring después de la presentación.
La nueva página ROM 151 se precarga tras observar una lectura CD real al
añadir el mensaje; son 172 páginas. No hay un sustituto para una dependencia.

Los mensajes INSERT COIN, REGAIN POWER y BUY-IN se conservaron al confirmar su
procedencia en FG0 y la ROM. No se borraron rótulos originales para cambiar
artificialmente el aspecto del combate.

## Verificación y límites

- 365 casos contra instrucciones originales `$D1FC`, sin caché, en frío y
  caliente: mismos records consumidos y D6/D7. Lecturas 10660 → 3873.
- 96 casos `$11CE..$122B` en Musashi sin reemplazar decisiones ni texto:
  FG0 completo, D0–D7 y fronteras físicas iguales. El oráculo ejecuta también
  `$21E6` y conduce su entrada física, contando los 131 flancos originales.
- 1540 casos del planificador contra la referencia 5ab045c; todos los IDs de
  16 bits, colisiones, orden, slots, clipping y límites.
- Frame 120: 786432 píxeles RGB idénticos a f61dc98. Comandos, texturas y HUD
  reales de VRAM, y registros de cámara, iguales en frames 120 y 1080.
  Esto es una comparación de muestras con el checkpoint, no una certificación
  universal del framebuffer del arcade.
- RED/GREEN de trabajo repetido: palabra ROM 2000 → como máximo 32 bytes de
  LWRAM por 1000 lecturas; body packed 14 → 5 lecturas; segundo commit de
  paletas sin cambios 16 → 0 llamadas. Se verifican errores, reset, claves,
  bancos iniciales cero, stride/padding, allocator, flash, aborto y reintento.
- Suite general, CORE 48/48 y 48/48 ASan/UBSan; tests de transporte, HUD, DMA,
  ring y nuevas cachés. Pipeline ROM independiente R17 y auditorías de fuente
  generada; display/HUD y paletas del árbitro contra ROM.
- Partidas HOST y ASan, incluida la presentación: 4391 sin entrada y 14409 con
  entrada controlada; cero lecturas CD de programa/sprites durante esas pruebas.
- Disco normal Mednafen: frame 4391, resultado 8001, cuenta 0300, reloj 0021,
  sin owner/dependencia fatal; resultado publicado y VRAM comprobada.
- Revisión independiente sin Critical/Important confirmados. Enlace SH-2 sin
  símbolos sin resolver. Extremo normal HWRAM `060FEE74`: 4492 bytes hasta el
  límite; perfil `060FF3B4`: 3148 bytes. Pool Yaul incluido en BSS.

**Saturn físico con CD-R: pendiente del usuario.** No se declara ROM-EXACT
universal, pixel-perfect universal, ni aptitud física ya demostrada.

## Reproducir y comprobar el paquete

```sh
make -C src/runtime test
python3 validation/run-host.py src/runtime --out validation/results/runtime18/core-host
python3 validation/run-host.py src/runtime --sanitize --out validation/results/runtime18/core-sanitizers
make -C src/runtime test-runtime18-rom-read-work-asan test-runtime18-metasprite-cache-asan
make -C src/runtime test-runtime18-palette-work-asan test-runtime18-present-palette-work-asan
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match
make -C src/runtime test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
python3 src/runtime/tests/r18/verify_plan_equivalence.py
python3 src/runtime/tests/r18/verify_result_screen_rom.py
DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 R18_PROFILE_START=900 R18_PROFILE_STOP=1080 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

Los tests ROM requieren los módulos fijados en `validation/requirements-rom.txt`.
El SDK Docker está fijado por digest en el script; SDK, BIOS y Docker son
dependencias externas. El ZIP incluye fuente, cinco assets canónicos, ROM de
referencia, Git bundle, discos y evidencia `checkpoint-render-core/`.

La reconstrucción fría debe producir el mismo A.BIN. El resultado de esa
comprobación se guarda en `export-verification.json`; el ZIP incluye su
manifiesto SHA256 y el par normal sometido a Mednafen. Los timestamps ISO
pueden variar. Abrir/grabar el CUE con su BIN al lado: MODE1/2352, una pista.
Se validan sectores EDC/ECC, IP y payloads ISO9660.

Disco normal probado:

- A.BIN: `87f5fbd954bb0057d4274cfbe5824193b335474b34f53f1266b1f3c5ab5779f5`
- BIN: `8f32237c822392527b25990695e1ed5596e8928a191f4e7c486a6c41722d102f`
- CUE: `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`
