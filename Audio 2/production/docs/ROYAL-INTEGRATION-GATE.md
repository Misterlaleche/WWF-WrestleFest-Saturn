# Royal HW1 — integración aislada y resultado negativo

**NOT ACCEPTED. HARDWARE PENDING.** Este documento sustituye únicamente la
afirmación anterior de que no existía un escenario Royal recuperable. No cambia
WP1980, sus gráficos, gameplay ni su política original de entradas.

**Corte final cddd + scalar9 / secuenciador `-Os`: ambas variantes siguen
fallando.** La base pierde el trabajador tras17 comandos; la de4MiB completa
los133 comandos sin pérdidas FM ni underruns PCM, pero registra6 eventos
tardíos. Los informes v2 conservan las velocidades observadas y rechazan
certificar el coste de trabajo de audio íntegro y dentro de plazo en ambos
casos. La evidencia anterior bf0 permanece separada y sin modificar.

## Fuente recuperada y aislamiento

La fuente es `wp1980_hw1_fuentes_y_pruebas.zip`, SHA256
`afa7132a241e06208bd47b10487e2361e9cba59e5085a17e4d3bc0a3c135eb70`.
Se recuperaron1444 archivos; el escenario correcto está en
`wp1980/work/hwfix01/native`. La inicialización Royal F94 y la política
`wp1980_input` son las originales de ese paquete. No se creó una simulación
alternativa de Royal ni se inventaron productores.

El constructor conserva305 archivos de juego y todos los recursos gráficos
copiados. Solo sustituye el backend de audio, añade su contrato de memoria y
los observadores de la integración. Los ELF control/audio de cada variante
difieren en un único byte de datos: `wf_audio2_enabled`. Sus instrucciones son
idénticas. El control conserva las reservas de memoria para que la comparación
no cambie la disponibilidad de RAM.

La política de entrada es determinista a partir del estado del juego y los
mandos físicos, mantenidos liberados. No existe en esta prueba una cinta que
registre cada byte de entrada en cada llamada. Por ello se informa por separado
la igualdad del estado final y de todos los eventos, sin convertirla en una
prueba byte a byte de una cinta de entrada inexistente.

## Contratos de memoria probados

El pool gráfico ampliado HW1 utiliza VDP1 hasta`0x05c78020`. Por ello no admite
el módulo musical en la ventana VDP1 de TAG. No se eliminó esa protección.

| Región | Royal base | Royal con cartucho4MiB |
|---|---|---|
| Programa frío y ROM local | VDP2`05e26000..05e3b000`,84KiB | Igual |
| BSS privado | VDP2`05e3b000..05e3e000`,12KiB | Igual |
| Compartido ABI4 | VDP2`05e3e000..05e3f000`,4KiB | Igual |
| Código/estado rápido | VDP1`05c78020..05c7c820`,18KiB | Reserva HWRAM del linker,18KiB |
| Pila slave | HWRAM,8192bytes | HWRAM,8192bytes |
| Heap libre tras reserva en los ELF capturados | 6304bytes | 21152bytes |

La alternativa inicial que reservaba26KiB completos en HWRAM de la variante
base falló al enlazar: su final era`0x06103240`, fuera de HWRAM. Se conserva ese
fallo; no se suprimió el límite ni se añadió un cartucho implícito.

La auditoría de propietarios encontró el hueco VDP2 después de las tablas de
nombres del banco1. El código original reserva cero bytes para Gouraud/CLUT en
VDP1 y no escribe en su cola libre. La pila permanece en HWRAM. El perfil base
es experimental: colocar código de audio en VRAM puede reducir su rendimiento.
La macro`WF_AUDIO2_WORKSPACE_VDP1` permite exclusivamente la dirección exacta
`0x05c78020`; no abre rangos arbitrarios. Requiere el perfil VDP2 explícito.

Master y módulo se compilan juntos. El loader rechaza otro perfil aunque lleve
el mismo número ABI4. Los tests de TAG, Royal/HWRAM y Royal/VDP1 verifican cuatro
segmentos, copia y puesta a cero exactas,352 imágenes malformadas por perfil,
rechazo del workspace de otra clase y ausencia de escrituras parciales.

## Candidato final: cuatro capturas completas

Las parejas finales usan exactamente el snapshot de TAG
`audio2-build/combat-prerequisites/integration/final-scalar9-cddd/source/`,
incluidas únicamente las nueve admisiones escalares y `wf_music_seq.c` a `-Os`.
El firmware es
`cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090`.
No se adoptó código de24KiB, lifts adicionales, buffers mayores ni nuevos
flujos de instrumentación. El perfil base continúa sin cartucho; el otro
usa explícitamente el perfil gráfico original con cartucho4MiB.

Los305 archivos originales y los recursos gráficos permanecen iguales en
cada copia. Control/audio difieren en un solo byte de datos y tienen código
ejecutable idéntico. Las cuatro ejecuciones usan Mednafen1.32.1 independiente,
velocidad de presentación normal y ningún salto de cuadros. Se observa el
endpoint original de8474 pasos y `finished_presentation=2`; no termina por
timeout. Cada pareja tiene el mismo BIOS y emulador.

| Resultado final observado | Base, sin cartucho | Variante4MiB |
|---|---:|---:|
| Pasos lógicos | 8474 | 8474 |
| Productores originales | 133 | 133 |
| Aceptados / completados | 17 / 17 | 133 / 133 |
| Omitidos / retenidos en cola al final | 115 / 1 | 0 / 0 |
| Descartes FM / lotes descartados | 4 / 1 | 0 / 0 |
| Rechazos FM / ring | 0 / 0 | 0 / 0 |
| Lotes FM inválidos | 1 | 0 |
| Avisos tardíos `B028` | **37** | **6** |
| Underruns PCM reales de32bits | 0 | 0 |
| Error worker / API | **15 / 5** | 0 / 0 |
| Primer fallo detectado, contador de vídeo | 2639 | 0 |
| Control, pasos lógicos/s | 50,890051 | 59,510037 |
| Audio2, pasos lógicos/s observados | 50,691600 | 58,453362 |

Los contadores de PCM cero no validan la variante base: el trabajador deja de
procesar casi todo el trabajo solicitado. En4MiB sí se mantiene activo hasta
el final, mejora concreta frente al anterior bf0, pero seis entregas tardías
siguen incumpliendo el criterio de audio. La API viva no demuestra fidelidad,
cero jitter ni trabajo completo dentro de plazo.

| Cadencia del combate, control → Audio2 | Base | Variante4MiB |
|---|---|---|
| Primer/último VBlank, control | 1164 → 11126 | 1657 → 10176 |
| Primer/último VBlank, Audio2 | 1556 → 11557 | 2045 → 10718 |
| VBlank transcurridos | 9962 → 10001 | 8519 → 8673 |
| Segundos nativos | 166,515847 → 167,167736 | 142,396147 → 144,970277 |
| P50 / P95 / P99, en VBlank | 1/2/4 → 1/3/4 | 1/1/1 → 1/1/2 |
| Máximo, en VBlank | 7 → 8 | 2 → 3 |
| Cambio de duración observado | +0,391488 % | +1,807724 % |

Se excluye BIOS/CD y se resta el primer VBlank lógico. Los últimos1000 paquetes
completos del MOV dan59,8261379Hz para base y59,8260568Hz para4MiB, diferencia
de una muestra de audio en esa ventana. En cada pareja ambos perfiles dan el
mismo reloj. No se usa60Hz nominal ni tiempo de pared como rendimiento del
juego. El máximo gap es133,7208ms en base Audio2 y50,1454ms en4MiB Audio2.

La base incluye gran parte del combate después del fallo, por lo que su
pequeña diferencia porcentual no representa coste de audio sostenido. La
variante4MiB añade154 VBlank, 2,574129s, con el trabajador activo; sigue siendo
la observación de un candidato con entregas tardías. Ninguna tasa certifica
el coste de un sistema de audio correcto. Además, la variante4MiB no satisface
el requisito de integración sin expansión y no sustituye a la base.

### Identidad de trabajo, memoria y límites

Dentro de cada pareja coinciden byte a byte los7648 bytes de runtime, los2412
bytes de actores y todos los133 pares frame/comando. El hash de productores
es3706974640, con palabra inicial original `0x3104`. El runtime base tiene SHA256
`c298420ae6a57c6cb6ef0ad2258c3e14e7137ff5ae818dda6a1edeaffd600e15`;
el de4MiB,
`16ca26715dc47033b53130faee5813a77f8673ed94e4b82274dd74139d752613`.
El hash común de actores es
`656f7934c1c1b1cbac5afef25e4d17c8503c050c011effc734a0e00e1fb6f884`.
No se afirma igualdad del runtime entre las dos configuraciones gráficas.

La política original `wp1980_input.c` conserva SHA256
`b30ec4abd28e14557a2d8a445109644881c047e434b68369b97f509a7e3d47f1`.
Los mandos siguen liberados. Esta es igualdad de política y observación del
estado/eventos; no existe un registro completo de bytes de entrada por llamada.

Código/constantes rápido14624 + estado3220 = **17844/18432 bytes** en ambas
variantes. La base mantiene hot en `05c78020..05c7c820`, pila HWRAM en
`060fc760..060fe760` y heap6304 bytes. La variante4MiB mantiene hot en
`060f4560..060f8d60`, pila en `060f8d60..060fad60` y heap21152 bytes. No cambian
los límites VDP2 ni los propietarios gráficos. El código frío ocupa83148 bytes
en base y83156 en4MiB. Todos esos payloads y los tres segmentos de firmware
coinciden con el estado nativo final. Cada pila conserva7648 bytes iniciales
del canario, con544 bytes modificados desde arriba; no es una prueba universal
contra desbordamiento. El hueco VDP2 final y el backcolor observado coinciden
entre los perfiles; no se inventa una comparación antes/después ni un canario.

La latencia máxima API es50001µs nominales en base y33334µs en4MiB: tres y dos
incrementos de VBlank, respectivamente. Es telemetría cuantizada; no mide
jitter preciso de FM, latencia del DAC ni percentiles de las escrituras FM.

### Reauditoría v2 y evidencia final

La revisión independiente encontró que el analizador v1 confundía trabajador
activo con coste comparable de trabajo íntegro. V2 separa
`audio_remained_operational`, `requested_work_integrity`,
`zero_loss_and_deadlines` e `immutable_code_verified`. En base los tres primeros
sonfalse; en4MiB solo el de pérdidas/plazos esfalse. La identidad de código
estrue en ambos. **`full_match_audio_cost_comparable=false` en ambos perfiles.**
Las cifras nativas anteriores permanecen como observaciones con esos límites.

V2 comprueba símbolos usados contra los ELF reales SH2/68000, incluidos los
locales/LTO; liga el módulo DEBUG a sus payloads empaquetados y verifica en
memoria texto maestro no mutable y payloads completos de módulo/firmware.
Los47 casos de contrato y símbolos pasan; la condición incorrecta anterior
tiene RED reproducido sobre el TAG real. No se recompiló ni recapturó el juego
para corregir el informe. Los informes/logs v1 originales se conservan.

- [Royal base final, informe v2](evidence/royal-hw1-base-cddd-scalar9.v2.json).
- [Royal4MiB final, informe v2](evidence/royal-hw1-4m-cddd-scalar9.v2.json).

Los originales completos están en
`audio2-build/royal-prerequisites/final-scalar9-cddd-base/` y
`audio2-build/royal-prerequisites/final-scalar9-cddd-cart/`, con
`COMBAT-RESULT.v2.json`, comandos exactos de construcción/captura/análisis,
manifest del snapshot compartido, ELF/mapas/ISO, layouts, MOV/WAV/F5/RAM y
timestamps de vídeo. Los payloads protegidos permanecen privados.

## Ejecución anterior bf0 en Mednafen1.32.1

Firmware sin instrumentación de timing:
`bf0a38041589e56003b3719899652d2431c28ead429e927b787a78062bec84bd`.
Este corte usa el decoder optimizado por pares y precede a los experimentos de
selección urgente/refill parcial. No trasladar sus contadores a otro firmware.

| Resultado observado | Base, sin cartucho | Variante4MiB |
|---|---:|---:|
| Pasos lógicos hasta final |8474|8474|
| Comandos originales producidos |133|133|
| Comandos aceptados por Audio2 |18|53|
| Comandos omitidos tras el fallo visible |115|80|
| Eventos FM descartados |9|16|
| Lotes FM descartados |1|1|
| Avisos tardíos de transporte |39|428|
| Underruns PCM reales de32bits |1|1|
| Error del worker |15|15|
| Primer fallo: contador de vídeo |2636|5740|
| Control sin audio, lógica/s |50,890051|59,510037|
| Ejecución Audio2, lógica/s global |50,737190|58,997637|

**Los promedios de la fila Audio2 no certifican el coste de audio sostenido.**
Incluyen tiempo posterior al fallo y al cese del worker. El analizador expone
`audio_remained_operational=false` y`full_match_audio_cost_comparable=false`.
La variante base ya queda por debajo de la referencia arcade sin audio. El
cartucho opcional no satisface el requisito de integración sin expansión.

En cada pareja coinciden los7648bytes del runtime, los2412bytes de actores y
los133 pares frame/comando originales. El final de juego y presentación se
observa realmente; no se detuvo por un timeout supuesto. Se conservan WAV,
MOV, F5, WorkRAMH, símbolos y mapas privados. La captura base muestra la pantalla
original GAME OVER sin alteración visible del escenario.

Al final, todos los bytes cargados de código frío y rápido coinciden con los
ELF correspondientes, incluso el código ejecutado desde VDP1. La pila conserva
7640bytes iniciales de su canario y presenta552bytes modificados desde arriba.
Esto es una observación de uso, no una prueba universal de ausencia de overflow.
El hueco posterior del banco1 y el color de fondo coinciden entre variantes;
no se afirma un canario que nunca se escribió.

Informes públicos sin ROM ni audio:

- [Royal base](evidence/royal-hw1-base-bf0.json).
- [Royal4MiB](evidence/royal-hw1-4m-bf0.json).

## Reproducción y trabajo pendiente

`integration/build_combat_pair.py --scenario royal --royal-cart0` usa la fuente
HW1 indicada y selecciona el contrato base. Escribir la opción como
`--royal-cart 0` en la consola. Para la variante opcional usar`--royal-cart 1`.
El capturador selecciona explícitamente`none` o`extram4` a partir del manifiesto
y espera`finished_presentation=2`. No hay sustitución por otro emulador.

Las parejas cddd anteriores completan la repetición con el mismo corte final
que TAG. Los comandos exactos están conservados en cada directorio privado;
el analizador usa `--input-policy` con el archivo original, nunca una cinta
inventada. Para una futura revisión quedan pendientes el fallo del perfil
base y las entregas tardías, además de fidelidad y hardware. Conservar la
evidencia negativa y no presentar la mera supervivencia del combate como
integridad del audio.
