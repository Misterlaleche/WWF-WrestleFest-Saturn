# Estado de implementación — 2026-11-12

> Historical development record. Results and measurements below apply to the revisions named in this document. See [current status](STATUS.md) for P1-03B. Some original capture evidence is external and is identified by its original path below.

## Criterio de aceptación

Destino: Saturn NTSC real, CD-R o Terraonion MODE y cartucho RAM de 4 MB.
Motor elegido: Jo Engine 2026.3. Se utiliza el núcleo nativo y los recursos
originales del proyecto. La velocidad de combate de TAG y Royal, los píxeles
y el tiempo visible de cada escena son criterios independientes y obligatorios.
**La aceptación completa sigue pendiente.**

## Variante entregable

`playable-final-09`: `WF_FRONTEND=1 WF_PROFILE=1`, sin `WF_AUTORUN`.
Arranca desde RESET y utiliza mandos físicos a través de Jo. La prueba manual
de arranque en frío llegó a 120 ticks de la demostración original, sin
inyección de entradas, estados guardados ni escrituras del observador en RAM.
La captura no verifica una partida manual completa ni hardware físico.

Las variantes automáticas TAG/Royal utilizan las mismas fuentes congeladas.
Sus macros de entrada seleccionan recorridos deterministas; no son el disco
manual entregable. Ambas llegaron a su terminal y completaron el owner
posterior sin registrar faults del núcleo.

## Medición de las rutas automáticas completas

Se cuentan campos Saturn emulados; el tiempo de ejecución del ordenador no
es la fuente del rendimiento. La referencia de reloj es aproximadamente
57,4449 ticks/s. El endpoint WFT1 se toma después del ACK final, antes del
owner posterior.

| Medida | TAG | Royal |
| --- | ---: | ---: |
| Ticks de combate | 9.666 | 14.409 |
| Campos entre inicio y terminal WFT1 | 10.079 | 17.581 |
| Ticks por segundo | 57,3747 | 49,0322 |
| Porcentaje medio de la referencia | 99,8778 % | 85,3552 % |
| Peor ventana observada de al menos 600 ticks | 92,4712 % | 75,1191 % |
| Actores terminales, 2.412 bytes | Coinciden | Coinciden |
| RNG terminal | Coincide | Coincide |
| Underruns PCM de efectos | 0 | 0 |
| Underruns musicales | 2 | 0 |

Los dos underruns de TAG aparecen en la cue 08, observados en ticks 9.561 y
9.566. Las mayores demoras de inicio musical de los recorridos completos,
incluidos menús, fueron 739 y 741 ms. La media casi nominal de TAG incluye
ralentizaciones y recuperación posterior; no acredita cadencia uniforme.

El owner posterior de TAG ejecutó 836 ticks en 896 campos, frente a un
presupuesto nominal de 870–871 campos. El de Royal ejecutó 1.941 ticks en
2.023 campos, frente a 2.021–2.022. Son intervalos de ejecución del owner:
todavía hace falta comparar la primera imagen visible de cada escena y una
traza arcade equivalente. No se presentan como tiempos medidos de una PCB.

Fuente de estas cifras: análisis final (external evidence: `../evidence/final-capture-analysis-09.json`).
Las identidades de fuentes (external evidence: `../evidence/fullfrontend-source-freeze-09.json`)
separan estas pruebas de las capturas históricas.

## Implementación actual

- Los 65.536 tiles originales se conservan en un pack comprimido con bloques
  independientes de 4 KiB. ROM, sprites, efectos y prefijos musicales caben
  en el cartucho, con 22.912 bytes libres después de alineación.
- La caché de sprites del frontend tiene 138 bloques en LWRAM. El renderer
  publica tiles visibles mediante cuatro bancos VDP1, con fences de consumo.
  Fondos, ring y capas del escenario se gestionan en VDP2.
- Las seis escenas comprimidas están residentes. La preparación incremental
  produce hasta 512 bytes por paso durante esperas originales y devuelve su
  préstamo de caché antes del combate. La primera escena se prepara durante
  la carga inicial. Los gráficos no requieren lecturas de CD durante combate.
- Jo conserva la propiedad de SCSP, 68000 de audio, SMPC y sincronización de
  vídeo. Los efectos se decodifican a PCM; la música usa cues renderizadas
  fuera de Saturn desde la referencia Z80/YM/OKI y streaming desde CD.
- El workaround `-fno-peephole2` está verificado en compilación y enlace del
  GCC SH-2 incluido. La auditoría estática revisa heap, stack e ISR, incluido
  el uso de MACH/MACL por el descompresor optimizado.

## Trabajo pendiente

Royal necesita más rendimiento sostenido y TAG necesita eliminar sus pausas
puntuales. Quedan el underrun de la cue 08 y las demoras de inicio musical,
además de la equivalencia del fundido 1F, el estado YM entre cues y los cambios
de banco OKI durante voces ya decodificadas. Las cues tienen duración finita.

La equivalencia de todos los estados interactivos, la composición final de
imagen, todas las transiciones y el comportamiento del lector CD-R/MODE se
deben comprobar en hardware real. Los resultados del host y de Mednafen no
conceden esa aceptación.
