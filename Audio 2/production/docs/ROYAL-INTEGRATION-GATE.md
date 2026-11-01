# Royal HW1 — integración aislada y resultado negativo

**NOT ACCEPTED. HARDWARE PENDING.** Este documento sustituye únicamente la
afirmación anterior de que no existía un escenario Royal recuperable. No cambia
WP1980, sus gráficos, gameplay ni su política original de entradas.

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
| Heap libre tras reserva | 5568bytes | 21152bytes |

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

## Ejecución real en Mednafen1.32.1

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

## Reproducción y siguiente paso

`integration/build_combat_pair.py --scenario royal --royal-cart0` usa la fuente
HW1 indicada y selecciona el contrato base. Escribir la opción como
`--royal-cart 0` en la consola. Para la variante opcional usar`--royal-cart 1`.
El capturador selecciona explícitamente`none` o`extram4` a partir del manifiesto
y espera`finished_presentation=2`. No hay sustitución por otro emulador.

Antes de una nueva prueba completa, corregir el caso breve de transición/FM
tardío y medir por separado el secuenciador, sus callbacks y las transferencias
al SCSP. Repetir estas parejas con el firmware/fuente congelados. Conservar la
evidencia negativa y no presentar la mera supervivencia del combate como
integridad del audio.
