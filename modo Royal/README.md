# modo Royal — WP1980 HW1

**CANDIDATE · pruebas específicas de host y Mednafen conservadas · HW1 pendiente de Saturn física. La velocidad mínima arcade todavía no está acreditada.**

Esta carpeta conserva el código de la revisión **WP1980 Royal HW1** entregada al usuario y explica cómo reconstruirla, comprobarla y continuar con Codex. Es una incorporación independiente al repositorio: no cambia los trabajos anteriores ni declara integrado el juego completo.

La raíz de producción de esta revisión es **`wp1980/work/hwfix01/`**. El árbol `wp1980/native/` que puede restaurarse del paquete privado es la versión anterior y sirve como referencia RED de los defectos corregidos. No compilar esa raíz por error para obtener HW1.

## Leer primero

1. [ARQUITECTURA.md](ARQUITECTURA.md): funcionamiento, código de entrada y salida, gráficos, memoria, controles y audio.
2. [COMPILAR.md](COMPILAR.md): restauración, SDK, compilación secuencial de ambas variantes, pruebas y Mednafen.
3. [CONTINUAR-CON-CODEX.md](CONTINUAR-CON-CODEX.md): punto de partida, límites y siguiente trabajo de rendimiento.
4. [Informe HW1 original](wp1980/deliverables/hw1/INFORME_HW1.txt) y [prueba física](wp1980/deliverables/hw1/PROBAR_HW1_EN_SATURN.txt).
5. [ESTADO.json](ESTADO.json), [FUENTES-SHA256.json](FUENTES-SHA256.json), [RECURSOS.json](RECURSOS.json) y [verificación del empaquetado](VERIFICACION-EMPAQUETADO.json).

Antes de integrar, leer también los contratos existentes de [README](../README.md), [PROJECT-STATE](../PROJECT-STATE.md), [HANDOFF](../HANDOFF.md), [pipeline Saturn](../docs/SATURN-PIPELINE.md), [MODE + ROSTER](../docs/MODE-ROSTER-HARDWARE-VALIDATION.md) y [audio](../docs/AUDIO-HARDWARE-VALIDATION.md). Sus propietarios de estado y recursos siguen vigentes. La ROM original prevalece sobre código, informes y pruebas.

## Qué ejecuta esta implementación

El SH-2 ejecuta código C nativo para el estado, los propietarios de acciones, CPU, colisiones, reglas, animación y presentación de Royal. El arranque directo usa un estado inicial trazable al punto `0xF94` de la ROM, `ROYINIT.BIN`, para sustituir el recorrido de menús de este banco de pruebas. Después calcula el combate en vivo.

La demostración automática solo decide entradas ordinarias del jugador 1. El adaptador `wp1980_input.c` busca un rival activo y pulsa direcciones/A/B; no carga una película de combate ni reemplaza el cálculo de los actores. Una entrada física de P1 entrega el control manual al mismo motor. Este adaptador de demostración, el arranque capturado y la espera final no deben convertirse en la campaña del juego integrado.

## Resultado medido y conservado

| Medición HW1 | Con RAM 4 MB | Base, sin expansión |
| --- | ---: | ---: |
| Actualizaciones de combate | 8.474 | 8.474 |
| Campos de pantalla durante el combate | 8.618 | 10.022 |
| Tiempo de combate | 144,050784 s | 167,518793 s |
| Media de actualizaciones por segundo | 58,826476 | 50,585369 |
| Peor ventana observada de 99 actualizaciones | 55,353142/s | 41,709762/s |
| Peor ventana observada de 59 actualizaciones | 54,303712/s | 37,954207/s |
| Presentación de GAME OVER | 98 campos | 98 campos |
| Lecturas adicionales de CD en el resultado observado | 0 | 0 |

Fuentes: [medición 4 MB](wp1980/research/hardware-result-01/cart-execution-analysis.log), [medición base](wp1980/research/hardware-result-01/base-execution-analysis.log) y [observación terminal independiente](wp1980/research/hardware-result-01/independent-observation/summary.json).

Son actualizaciones de juego medidas con contadores nativos y la calibración de PTS de Mednafen, **59,826123415380 campos/s**. No son FPS calculados con el tiempo de procesamiento del PC. Base reutiliza explícitamente la calibración del vídeo 4 MB del mismo modo de pantalla. Las ventanas solo incluyen extremos realmente observados: no prueban el mínimo de cada instante.

Ambas variantes conservan los **2.412 bytes de los nueve actores** y RNG terminal `B8A86C74` de la referencia, terminan en actualización 8.474 y completan GAME OVER sin fallo fatal. Es evidencia de las rutas reproducidas, no de todas las semillas, mecánicas, mandos o estados posibles.

Las imágenes anteriores se probaron físicamente y el usuario observó problemas de efectos y un fallo al cerrar Royal. **HW1 es una revisión posterior comprobada en Mednafen 1.32.1; su repetición en la consola sigue pendiente.** La música permanece desactivada. Las pruebas de cola de audio no equivalen a una comprobación acústica física.

## Qué corrigió HW1

- Fija desde la carga las páginas de ROM del resultado Royal, eliminando la dependencia fría identificada en la paleta y conservando el flujo original de GAME OVER.
- Ajusta a 4 KiB el staging de cabecera de los atlas de este combate para respetar los límites existentes de RAM; los metadatos completos permanecen en su área asignada.
- Limita las decodificaciones PCM contiguas al final físico de cada anillo de voz, evitando escribir sobre la guarda o la voz siguiente.
- Conserva un diagnóstico de CD congelado y la dirección real del fallo de paleta en las páginas HW1/R06.

El [diff original de HW1](wp1980/research/hardware-result-01/hw1-source.patch), el [informe](wp1980/deliverables/hw1/INFORME_HW1.txt) y las pruebas incluidas explican el alcance. Esta entrega a GitHub conserva esas fuentes; no introduce una optimización nueva en el motor. La reconstrucción independiente de la variante 4 MB del 30 de octubre reprodujo exactamente el programa entregado: 976.116 bytes, SHA-256 `5b7aaac6eb9a014d60f804b60984908526cbc13ea4fbc78219f664fff7adb634`; conservó 56.076 bytes de margen HWRAM y 52.192 hasta la pila secundaria. Esto no promete el mismo hash de ISO con otros timestamps.

## Recursos privados y conservación de fuentes

Los **356 inputs de producción** se conservan por hash: **337 archivos UTF-8** están en Git y **19 recursos binarios** se restauran del paquete privado. También se incluyen herramientas, pruebas y evidencia textual seleccionada, sin duplicar los registros voluminosos.

Paquete exacto: [wp1980_hw1_fuentes_y_pruebas.zip](https://chatgpt.com/library?file_id=libfile_f6afc43063a88191ac2e44793a99433c&account_id=personal)

```text
SHA-256 afa7132a241e06208bd47b10487e2361e9cba59e5085a17e4d3bc0a3c135eb70
```

Un agente sin acceso a ese archivo necesita que el usuario le proporcione el ZIP exacto. El paquete no contiene BIOS ni SDK. Git no contiene ROM, packs protegidos, PCM, ELF, imágenes BIN/CUE, fotografías, vídeos ni volcados de memoria.

Desde esta carpeta:

```bash
python3 tools/restore_resources.py /ruta/wp1980_hw1_fuentes_y_pruebas.zip --scope build
python3 tools/restore_resources.py --scope build --verify
```

`build` verifica los 356 inputs y restaura los 19 recursos externos. `all` reconstruye los 1.444 miembros del paquete, incluyendo la referencia anterior y los datos de las pruebas. El restaurador valida primero el ZIP, los miembros y todos los destinos; rechaza archivos diferentes y enlaces simbólicos, y no reescribe una fuente idéntica. La restauración completa debe hacerse en una copia de trabajo aislada si no se desea materializar todo el archivo privado.

**El SDK usado no admite espacios en la ruta de compilación.** Conservar el nombre Git `modo Royal` y copiar `wp1980/` a una ruta nueva sin espacios antes de compilar, como indica [COMPILAR.md](COMPILAR.md).

## Pendiente

Velocidad mínima de arcade, HW1 en Saturn NTSC real, efectos completos y sincronizados en consola, integración con el frontend y audio canónicos, más rutas manuales/semillas, y PAL. La suite general heredada `make test` está bloqueada porque falta `tests/test_core.c`; no se declara un PASS global.
