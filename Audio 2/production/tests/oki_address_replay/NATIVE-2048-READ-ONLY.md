# Anillos de 2048: análisis de memoria y frontera de banco

**PENDIENTE NATIVO. Sólo lectura; no se ha cambiado el firmware, el DSP, el
loader ni el contrato de recursos.** El prototipo host demuestra resolución
y reconstrucción exactas cuando el consumidor entrega una frontera conocida,
monótona y sin concurrencia. El runtime todavía no entrega esa frontera.

## Memoria: la alternativa es concreta

El perfil `cddd1b01…` ocupa 12.200 bytes en el segmento bajo, hasta `0x2FA8`,
y termina su BSS en `0xFF40`: quedan 88 y 192 bytes, respectivamente, antes
de los límites actuales. El objeto aislado del prototipo, compilado con GCC
13.3.0 `-m68000 -Os`, tiene 2.332 bytes de código y 106 de constantes. Todavía
necesita resolver `__modsi3`, `__umodsi3` y `memset`; no se ha enlazado ni
medido. Por tanto, esos huecos actuales no bastan para instalarlo tal cual.

Reducir cada ring de 4096 muestras más un sentinel a 2048 más un sentinel
cambia el stride de 8194 a 4098 bytes:

| Región | Perfil actual | Alternativa propuesta |
| --- | --- | --- |
| Cuatro rings PCM16 con sentinel | `0x3000..0xB008`, 32.776 B | `0x3000..0x7008`, 16.392 B |
| Espacio recuperado | — | `0x7008..0xB008`, **16.384 B** |
| Reloj y constantes DSP | `0xB010..0xB018` | conservar |
| Cabecera y 256 eventos musicales | `0xB020`, `0xB100..0xB900` | conservar |
| Mailbox, seno y pila 68000 | contrato actual | conservar |

La extensión de direcciones de 704 B continúa cabiendo en la cola actual
del recurso. Con stride de checkpoint 128, un ring de 2048 requiere 17
checkpoints por voz: 272 B para cuatro voces. El mismo bookkeeping del
prototipo supondría 448 B para cuatro voces más 16 B del mapa; esta cifra
es aritmética de estructuras, no una reserva instalada. Los cursores DSP,
control de publicación, cachés de preparación y pilas deben presupuestarse
además. Hay espacio plausible en los 16 KiB para código y estado, pero no
queda probado el plazo de ejecución.

Fuentes: `docs/MEMORY-AND-ABI.md`, `src/wf_music_m68k.ld`, el manifiesto
`production-final-firmware/firmware-build.json` del perfil cddd y los objetos
privados `m68000-object-metrics.json` / `prototype-m68000.su` del experimento.

## Cambiar sólo tamaño/stride sería incorrecto

`tools/build_dsp_mixer.py` conserva una fase de 24 bits, con 12 bits
fraccionarios. El programa actual centra un índice de 4096 posiciones para
el desplazamiento **ADRS con signo de 12 bits**. `wf_scsp_dsp_load()` apunta
MADRS al punto medio de cada ring, `base/2 + 2048`. El DSP lee la posición
actual y la siguiente para interpolar; el sentinel representa la siguiente
posición al final del ring.

Si se reduce únicamente el stride, la segunda mitad de las direcciones
actuales leerá otro ring o el espacio que se pretende dedicar a código.
Hace falta demostrar un direccionamiento módulo 2048 que conserve el avance
de fase, su fracción y la interpolación. La frecuencia nativa sigue siendo
`44100 * 743 / 4096`, aproximadamente 7999,585 Hz; cambiar el módulo de
direcciones no autoriza cambiar esa frecuencia.

Una opción aritmética para investigar es centrar en 1024 y formar
`sign24(2 * (fase - 0x400000)) / 2` antes de ADRL, preservando por separado
la fase y FRCL originales. Su índice sería el índice original módulo
2048. Requeriría revisar el pipeline y los pasos extra; **no hay aquí un
programa DSP nuevo ensamblado ni validado**. El programa actual usa 120 de
128 pasos. También debe probarse el acceso `NXADDR` al sentinel y prohibirse
cualquier lectura PCM fuera de `0x3000..0x7008`.

La fuente independiente `mednafen/src/ss/scsp.inc`, `RunDSP()`, confirma que
ADRS se extiende con signo a 12 bits, que `TABLE=1` evita el enmascaramiento
del ring DSP y que el acceso a RAM queda pendiente hasta otra instrucción.
Reducir RBL no resuelve este caso: con `TABLE=0` su mínimo es 8192 palabras
y además se añade el contador rotatorio MDEC.

## Cuándo llega realmente el banco

**Ruta musical.** `wf_music_seq.c` emite `WF_MUSIC_BANK` al observar E800.
`wf_music_module.c` establece la línea temporal en reloj DSP actual +4096
frames. `music_service()` espera hasta que el timestamp esté vencido y
entonces sólo asigna `music_bank`. El adelanto nominal es 92,879819 ms,
equivalente a 743 intervalos PCM nativos. No demuestra que cada registro
esté disponible con todo ese adelanto: puede llegar después, y el firmware
actual sólo lo aplica al vencer. El preview actual prepara inicios futuros;
no repara voces activas por adelantado.

**Ruta SFX.** `wf_audio2_command()` publica `WF_NATIVE_ARCADE_WORD` en el
mailbox. Ese mensaje contiene la orden y su argumento; no contiene una
frontera futura de reproducción. `service()` lo atiende entre servicios
musicales y llama a `wf_scsp_sfx_command()`. Esta función asigna `s->bank`
en ese momento y lo copia en la secuencia nueva. No existe un aviso previo
de 4096 frames para ese cambio. Las dos variables actuales tampoco son un
único banco global aplicado a todas las voces.

En el modelo original, E800 cambia el banco antes de la próxima lectura
codificada. A 8 kHz, esa lectura puede estar a menos de 125 microsegundos;
el mínimo garantizado es **cero**, porque el evento puede caer junto al
siguiente tick. La mitad de capacidad del ring no crea ese aviso previo.
No se propone retrasar el banco, alterar prioridades, mutear, cancelar notas
ni redefinir retroactivamente su timestamp.

Fuentes: `src/wf_audio2.c::wf_audio2_command`,
`src/wf_music_module.c` (inicialización de `event_time`),
`src/wf_music_firmware.c::{music_service,service}`,
`src/wf_scsp_sfx.c::wf_scsp_sfx_command` y
`tools/reference/{capture_board_replay.c,reference_oki.h}`.

## Por qué el cursor actual no certifica una reparación sin carreras

`wf_scsp_sfx_advance()` calcula `played` desde un reloj global y
`phase += elapsed * 1486`, desplazando después 13 bits. El instante inicial
se estima con una lectura de reloj más `start_guard`; el firmware DSP usa
guard=1. No se lee una posición real de consumo por voz. El DSP sigue
ejecutándose entre stores del 68000 y puede leer tanto la muestra actual
como la siguiente mientras el 68000 restaura estado o escribe PCM.

El guard normal de 64 muestras sirve a otro contrato: al rellenar, el slot
de N sólo reemplaza N−R cuando esa muestra antigua quedó suficientemente
atrás. No demuestra que sea seguro sobrescribir la primera muestra afectada
por un banco en el frente de lectura. Las 8 muestras de guard inicial tampoco
constituyen aviso para un banco.

Ejemplo de carrera: se observa el reloj, llega el banco inmediato, se
restaura desde un checkpoint y, durante ese trabajo, el DSP lee la muestra
que se intenta corregir o su vecino de interpolación. Escribir después no
puede cambiar un frame ya emitido. Empezar más adelante omitiría muestras
afectadas. Comprobar el reloj al terminar detecta el incumplimiento, pero
no lo repara. Además, la interpolación puede haber usado una muestra como
vecina antes de que avance el índice entero; la frontera debe definirse por
su **primera lectura efectiva**, no sólo por el contador entero.

Leer TEMP directamente tampoco suministra ahora un snapshot atómico:
`RunDSP()` direcciona TEMP con MDEC, y su acceso MMIO de 24 bits se divide
entre palabras. No existe en el runtime un protocolo que vincule esos datos
con una generación y frame por voz. La fuente local de Mednafen demuestra
ese problema de observación; no sustituye una prueba del Saturn físico.

Una futura exportación de cursor por el DSP podría dar un límite útil si
se demuestra su época, cuantización, antigüedad máxima y relación con las
lecturas actuales/siguientes pendientes. Para cada lote, habría que acotar
también el avance del consumidor durante los stores. Sólo se puede publicar
un rango cuyo primer uso esté después de terminar **todos** sus stores,
incluidos el sentinel y todas las voces afectadas. Si ese rango ya alcanza
la primera lectura exigida por el evento, se debe conservar el fallo como
fallo de deadline. No hay una corrección temporal equivalente por escribir
silenciosamente en una frontera posterior.

## Coste de cola y deadline

| Política host de reparación completa | 4096 muestras | 2048 muestras |
| --- | ---: | ---: |
| Horizonte nominal a 8 kHz | 512 ms | 256 ms |
| Estado hasta la frontera, por voz | ≤127 | ≤127 |
| Cola preparada a regenerar, por voz | ≤4096 | ≤2048 |
| Operaciones máximas, cuatro voces | 16.892 | 8.700 |

El prototipo medido con ring de 4096 necesita 8348 operaciones en el evento más
costoso de la cinta real, con dos voces activas. La reconstrucción host es
exacta, pero no mide ciclos 68000.

Como calibración, `music_work()` documenta 110 frames DSP para un bloque
secuencial de 128 muestras del decoder actual. Extrapolar ese coste da unos
1760 frames (39,91 ms) por cola de 2048, o unos 7480 frames (169,6 ms) para cuatro
colas y restauraciones máximas. **No es un benchmark del resolvedor nuevo
ni una cota WCET**: no incluye sus misses, checkpoint stores ni arbitraje.
Incluso esa estimación ya exige trabajo por trozos: un bloque completo
superaría el guard de servicio de 512 frames, y cuatro colas superarían el
adelanto musical de 4096 frames. La capacidad del ring es runway de PCM;
no es el deadline de la primera muestra cambiada.

## Experimento propuesto, separado de la corrección

1. Crear un perfil privado explícito de imagen residente. Mantener
   reloj/transporte/mailbox, y asignar de forma verificable código/estado
   dentro de `0x7008..0xB008`. El loader actual exige tres PT_LOAD y rechaza
   código fuera de sus reservas actuales. Debe aceptar un contrato nuevo
   concreto, por ejemplo un cuarto segmento con límites propios; no se debe
   eludir su validación ni confundirlo con el ABI4 del módulo SH-2.
2. Antes de reparar bancos, probar sólo el direccionamiento DSP de 2048 y, si
   cabe en su presupuesto, observación de cursores. Verificar exhaustivamente
   direcciones/fracciones/sentinel en host, después equivalencia nativa sin
   bancos y canarios en cada frontera de propiedad. Rechazar cualquier
   variación de frecuencia/interpolación o acceso a la zona de código.
3. Medir cuánto tarda obtener un cursor utilizable, reconstruir estado y
   publicar el **primer** pequeño bloque, además del coste total de la cola.
   Barrer las fases de llegada de banco, ambas paridades, wraps, cambios
   consecutivos y cuatro voces activas. Conservar el deadline original y
   todos los contadores actuales. Medir especialmente el mailbox inmediato.
4. Si no puede demostrarse que el primer bloque es visible antes de su
   lectura exigida, cerrar el experimento como bloqueo de publicación/tiempo.
   No ampliar buffers, aumentar plazos ni añadir latencia para producir PASS.

**Decisión actual:** la reducción a 2048 tiene una ventaja de memoria real,
pero requiere un cambio DSP y de perfil de memoria. No elimina el bloqueo
del banco inmediato ni convierte `played` en un cursor de consumo probado.
La corrección nativa sigue pendiente; el prototipo host queda reproducible.
