# P1-03 — música limitada en banda y fundido original

## Problema

Las 18 músicas de FIX2R se capturaban observando el YM2151/OKI a una línea de
tiempo de 176,4 kHz, pero el capturador conservaba una de cada cuatro muestras
para crear el WAV de 44,1 kHz. Esa decimación no aplicaba un filtro
antialias. La energía por encima de 22,05 kHz se plegaba sobre la banda audible
y alteraba el timbre. El defecto afectaba a todo el catálogo, incluida la
música de título `MU02.WAV` y la música de Legion of Doom `MU06.WAV`.

## Corrección

- La placa de referencia se captura a 176.400 Hz.
- La conversión offline a 44.100 Hz utiliza libsoxr con precisión 33, sin
  dither y con filtrado limitado en banda.
- Los 18 WAV se regeneran de la misma manera; no se ecualiza ninguna pista de
  forma individual.
- Se conservan PCM mono, 16 bits, 44.100 Hz, tamaños y extents del disco.
- El comando musical `$1F` ya no detiene bruscamente la música. El runtime
  realiza 100 pasos de nivel total SCSP durante cinco segundos y ejecuta el
  key-off al final. Un nuevo cue cancela el fundido y recupera el nivel 127.
- La parada de emergencia P0-01 sigue teniendo prioridad y cancela el fundido.

## TDD y evidencia

- La prueba sintética de 10 kHz conserva la banda útil: 0,000 dB de variación.
- Una señal de 30 kHz que antes se plegaba sobre 14,1 kHz queda atenuada
  93,8 dB.
- La prueba de frecuencia de salida verifica 44,1/176,4 kHz y rechaza
  geometrías inválidas.
- La prueba del runtime verifica inicio, progreso, final, cancelación por cue y
  cancelación de emergencia del fundido.
- La reproducción completa del driver Z80 con `$1F` a los cinco segundos cae
  aproximadamente 74 dB durante los cinco segundos siguientes y queda en
  silencio cerca del sexto segundo. La evidencia está en
  `build/p1-audio/evidence/fade-reference.json`.

## Resultados del catálogo

- 18 pistas regeneradas.
- 627.719.902 bytes de música.
- Ninguna muestra saturada en el catálogo generado.
- `MU02.WAV`: `874815bebb80030280f73175d446753d65b49494cd5743884663eca0e989f2b5`
- `MU06.WAV`: `f794ed1810b3e81c9f4758ab4c0edd4ff0374176395e4544ff84537e9058d247`

## Memoria

La instrumentación y el fundido elevan `_end` a `0x060D9E10`. Quedan 8.688
bytes antes de `0x060DC000`, por encima de la puerta P1-02 de 8 KiB. No se
reduce ninguna capacidad gráfica, de gameplay o de audio.

## Candidata física

`WrestleFest_Saturn_P1_03_AUDIO` es una candidata para MODE, no una entrega
final. Debe comprobarse en Saturn real:

1. Claridad de la música del título.
2. Combate contra Legion of Doom sin ruido agresivo ni cortes.
3. Fundidos y cambios de tema.
4. Ausencia de underruns o pausas nuevas.
5. Persistencia de la corrección GFX1.

No había emulador Saturn instalado en el entorno. La validación física sigue
pendiente. Las pistas largas siguen siendo capturas finitas de 600 segundos;
los bucles exactos quedan fuera de este bloque.
