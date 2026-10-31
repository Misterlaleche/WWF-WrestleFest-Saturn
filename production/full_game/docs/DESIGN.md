# Integración del juego completo — diseño de ejecución

Autoridad: SPEC.md (encargo íntegro del usuario), ROM programa SHA1
`a726c46f350723ea73158bc78055d17d418b14e2`, evidencias auditadas,
Saturn/Yaul, código anterior. Base Git `aedca6827c7801a88dbb1eae280830cd4776b95f`.

El destino es un único ejecutable Saturn NTSC, con detección automática de
4 MiB, el mismo gameplay en RAM interna y ampliada, sin reproducción de audio.
Las carpetas originales permanecen byte a byte intactas. Todo cambio nuevo
vive en production/full_game. Los recursos se preparan antes de escenas y
combates; una carga pendiente o fallida nunca se presenta como gameplay.

## Decisiones

Se integran núcleos existentes mediante adaptadores; se descartan concatenar
discos, reiniciar harness completos o inventar escenas faltantes. El usuario
ha elegido integración de fuentes y autorizado implementación autónoma.
Un controlador poseerá reloj, VBlank, input, CD, VRAM y memoria. Los módulos
reciben servicios y datos explícitos. Cada escena entrega la propiedad de
VDP1/VDP2 antes de cargar la siguiente. Audio queda como eventos observables,
sin backend de reproducción. El reloj debe ser independiente de audio.
El adaptador actual conserva eventos de Presentadora; los puntos prepare/start/
stop y la línea temporal de registros de Entrada siguen OPEN en el adaptador.
Su implementación original permanece disponible para trasladar ese contrato.

La cadencia racional ya usada por Entrada es 37609/39168 actualizaciones por
campo NTSC (7 MHz/(448*272), frente a 315 MHz/(11*1820*263)). No se consume una
actualización por cada campo ni se descartan ticks/poses cuando hay atraso.
Los atrasos son medibles; este diseño por sí solo no demuestra rendimiento.

## Primer checkpoint y límites verificables

Se enlazan directamente los núcleos C aprobados de Entrada y Presentadora con
un adaptador nuevo. Entrada conserva seis estados de preparación y 402 estados
del núcleo, su estado previo para sprites y su llamada terminal. Presentadora
recibe los cuatro IDs lógicos originales y conserva su limpieza terminal sin
tick visible extra. La carga de cada escena se confirma explícitamente.

La semilla es dato explícito; no se define una semilla de arranque inventada.
Entrada exige un callback RNG proporcionado por el propietario de la lógica;
no se utiliza su fallback de entropía nativa. El estado RNG de Presentadora
se entrega explícitamente tras la carga: hasta enlazar el propietario ROM del
traspaso, la equivalencia RNG del recorrido completo queda OPEN.

Este primer enlace es una prueba ejecutable host y compilable SH-2 del código
de integración. No contiene un renderer maestro ni se etiqueta como juego,
disco o beta completa. Los recursos se recuperaron en el checkpoint02 y sus hashes están verificados.

Se regeneran únicamente recursos cuyos IDs/tablas y formatos están disponibles
y cuyo hash coincide con el aprobado. Una coincidencia de imagen aproximada
no es suficiente. Recursos originales, ROM y BIOS permanecen externos a Git.

## Integración siguiente

ZIP aprobados recuperados y bloques reconstruidos en checkpoint02; adaptar renderer y
carga a plataforma única; importar frontend TEST03; hacer dinámico el presupuesto
de Royal (las capacidades máximas de sus dos builds no caben juntas en base);
unir los motores con contratos de estado reales; completar las rutas ROM abiertas;
medir todas las fases y ventanas solicitadas en Mednafen 1.32.1, None y 4MiB.
No se infiere un paso por la mera existencia de un documento o resultado histórico.

## Aceptación

La beta completa exige todos los recorridos de SPEC.md, pruebas diferenciales,
un disco universal real, hashes, vídeos sin alterar y métricas nuevas. La prueba
física seguirá siendo una validación distinta. Hasta entonces ESTADO.md mantiene
las rutas OPEN y los bloqueos concretos.
