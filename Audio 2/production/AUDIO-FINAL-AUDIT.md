# Audio 2 — auditoría de entrega

**Estado: NOT PRODUCTION READY. HARDWARE PENDING.**

El nombre de la rama `audio-2-production-ready` identifica el encargo, no una certificación. Esta entrega conserva un motor nativo ejecutable, herramientas de regeneración y evidencia positiva y negativa. Los criterios pendientes o fallidos no se convierten en aprobados por disponer de una API, un ELF o un estado `RUNNING`.

La autoridad de datos sigue siendo el `wwfwfest.zip` original. R04, A53 y todas las implementaciones históricas permanecen intactos. Una coincidencia con un modelo histórico prueba esa coincidencia; no demuestra por sí sola que el modelo reproduzca correctamente el reloj o la salida física del arcade.

## Resultado y candidato seleccionado

El candidato de firmware es `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090`, combinado con las nueve instrucciones SH-2 admitidas tras comparación exacta (`z80-native.c`, SHA256 `c408e4a8ad5d7b54a55706f4c96fd9f4d39271dcd0fb5e2de14331af138f4fad`) y `wf_music_seq.c` compilado con `-Os`. El núcleo Z80 sigue con `-O2`. No se seleccionaron los lifts adicionales ni la ventana experimental de 24 KiB.

La referencia actual **full-board** ejecuta el Z80 original con Timer A/B, estado busy/IRQ YM y lectura ROM/busy/banco global OKI. Las 18 parejas musicales del candidato `cddd/scalar9` se generaron con esta referencia y con las capturas de la campaña nativa larga. La entrega privada anterior del perfil `621b5dfc` permanece identificada por separado; la actualización del ZIP requiere autorización del destino tras el rechazo de aprobación automática. Los oráculos A53/R04 y el perfil intermedio Timer-B-only se conservan con sus identidades y limitaciones explícitas.

La auditoría completa encontró dos defectos diferentes de efectos: 17 comandos repetían un inicio que la rama original `F1` suprime; esa corrección ya supera los 98 casos de inicios físicos. El banco global aplicado a voces activas **sigue sin reproducirse correctamente**. Una prueba causal aisló 2.663 muestras ADPCM y 14.680 frames de mezcla diferentes por ese motivo. Este defecto bloquea ROM-EXACT aunque los contadores de ejecución fuesen cero.

El replay final de 138 segundos con las 64 palabras TAG observadas acepta todas, conserva cero descartes FM y cero underruns, pero registra **dos avisos tardíos**. Es un resultado fallido. El mismo firmware con el SH-2 anterior había registrado cuatro avisos; los dos perfiles no se confunden. La prueba de presión y recuperación del candidato final acepta los 98 comandos, ejercita 97 respuestas BUSY, para y recupera el slave correctamente, pero conserva **dos underruns y cuatro avisos** anteriores al reinicio. La segunda época tiene sus contadores a cero; no borra el fallo de la primera.

Royal HW1 ya se recuperó y se ejecutaron parejas completas base/4 MiB: 8.474 pasos, 133 productores y estado idéntico en cada pareja. Con el candidato actual, la variante base descarta cuatro eventos FM y registra 37 avisos; la variante 4 MiB completa los 133 comandos con cero descartes/underruns, pero seis avisos. Ambas fallan la aceptación; véase [ROYAL-INTEGRATION-GATE.md](docs/ROYAL-INTEGRATION-GATE.md). El promedio de una ejecución cuyo audio se detiene no sirve para aprobar su coste sostenido. Dos cargas corruptas probadas de nuevo con el candidato actual en Mednafen se rechazan con API FAULT/RESOURCE antes del primer comando; [evidencia](docs/evidence/native-load-failures-final-cddd.json).

## 1. Recuperación y selección de fuentes

Punto de partida: `audio-2`, commit `db18be40a1d79de200408207573e9f45bf73bf2c`. Se recuperaron 415 archivos seleccionados, correspondientes a 304 blobs distintos, comprobando sus identidades Git. El verificador del paquete histórico aprobó sus 380 archivos. Se inspeccionaron README, ESTADO, INTEGRACION, COMPILACION, experimentos, base y evidencia de Audio 2; los documentos R04, validación física y propiedad de productores; PROJECT-STATE y la issue #2.

La comprobación posterior de las referencias remotas seguía mostrando `audio-2` en ese checkpoint y `main` en `aedca6827c7801a88dbb1eae280830cd4776b95f`. La publicación usa el árbol remoto real como base; el checkout local parcial nunca se utiliza para eliminar archivos remotos que no fueron recuperados.

| Componente | Selección y razón |
|---|---|
| Control Z80 musical | Ruta `z80-inline-tabla`: el estado CPU/RAM/timers y los eventos se contrastan con el intérprete R01 conservado. La selección no depende del nombre ni de la fecha de la carpeta. |
| PCM y comandos | Decoder nativo, preparación incremental y refill por urgencia. Las 114 muestras se contrastan con sus hashes completos; los inicios físicos de los 98 comandos se contrastan con el Z80 original. A15 se conserva como regresión de alcance limitado. |
| Transporte | Conserva los tamaños y plazos originales; añade servicio cooperativo, errores explícitos y contadores de pérdidas individuales. |
| FM SCSP | Puente nativo con correcciones demostradas de TL inicial y profundidad de modulación. La fidelidad de salida sigue siendo un criterio separado. |
| Referencia | R04/A53 se reproducen con sus hashes intactos. Las discrepancias halladas en su configuración se investigan en perfiles separados. |
| Carga del juego | Master y módulo ABI4 deben compilarse juntos con sus reservas reales; el diagnóstico estático no certifica esa integración. |

Todos los cambios de esta entrega residen bajo `Audio 2/production/`. No se sustituyen WP1979, WP1980, frontend, TAG, Royal, gameplay, gráficos, otras escenas ni la referencia R04.

## 2. Arquitectura implementada y coste que conserva

El SH-2 principal conserva un único propietario de la API y entrega las palabras originales. El SH-2 secundario ejecuta el controlador musical original mediante el intérprete optimizado y traduce/escribe registros SCSP en sus instantes programados. No sintetiza muestras con Nuked-OPM. El 68000 atiende el transporte PCM, el secuenciador de efectos, la decodificación ADPCM, la preparación y el relleno de rings. El SCSP genera los operadores FM con sus 32 slots, y su DSP mezcla cuatro voces PCM mediante un programa de 120 pasos.

Esto descarga la generación de muestras del SH-2, pero **todavía reserva el SH-2 secundario**. No se presenta como un servicio de coordinación de coste despreciable. El presupuesto del combate debe aceptarlo con una comparación reproducible con/sin audio. Nuked-OPM se usa exclusivamente en herramientas de referencia del ordenador.

El perfil instrumentado anterior consumió un 88,76 % de tiempo inclusivo en el secuenciador durante su ventana TAG, con llamadas de hasta 29,716 ms. La variante SH-2 seleccionada reduce un 5,407 % el coste inclusivo en una comparación nativa acotada y conserva los primeros 65.536 eventos/ciclos, además del test largo del host. Son medidas de sus perfiles concretos, no una certificación de disponibilidad del slave ni de todos los escenarios. [Pruebas y reproductor](tests/sh2-scalar-admissions.md).

No hay CD-DA, canciones pregrabadas, cartucho RAM obligatorio, transferencia DMA de audio sin medir ni recursos protegidos incluidos en el código publicado. El pack comprimido original se genera localmente, se carga en Sound RAM y se decodifica por bloques.

## 3. Pérdidas FM: causas y tratamiento

El ensayo histórico de 9.600 cuadros, `game_frame=457`, 361 descartes y 1.664 retrasos es fallido. El antiguo contador de descartes mide **lotes**, no necesariamente 361 notas ni 361 escrituras individuales. El estado `RUNNING` y error cero después de perder eventos tampoco es una prueba de integridad.

Se han corregido las siguientes causas demostradas:

1. Un lote largo del intérprete impedía atender a tiempo eventos cuya fecha ya estaba en cola. La ruta nueva da oportunidades de servicio cada 512 instrucciones ejecutadas, manteniendo exactamente ciclos, RAM y orden de eventos del controlador original.
2. La descarga de registros FM podía utilizar un reloj obsoleto. Se vuelve a observar el reloj durante lotes de 32 escrituras, sin convertir cada instrucción vacía en una lectura del bus de sonido.
3. Los caminos de cola llena o evento ya perdido podían continuar sin un fallo inequívoco. Ahora se conserva la primera causa y se cuentan eventos, lotes y rechazos por separado. No se reproduce una nota irrecuperablemente atrasada como si fuese correcta.
4. La preparación monolítica de 256 muestras PCM ocupaba hasta 294 Fs en el ensayo instrumentado. Se divide en bloques de 64 muestras conservando el estado exacto del decoder.
5. Un refill de 148–149 Fs podía comenzar justo antes de un grupo de eventos. Se admite ese trabajo únicamente cuando cabe antes del evento más cercano; el tamaño normal de refill sigue siendo 128 muestras.
6. Una búsqueda anticipada de 12 registros no veía un inicio PCM situado en el registro 14, aunque ya estaba en el ring. La búsqueda de 16 corrigió ese caso. La matriz ampliada detectó otro inicio en la posición 17; su corrección y medida se registran por separado, sin generalizar el resultado del primer comando.

Se conservan el ring de 256 registros, la FIFO FM de 512 entradas, la cache de preparación de 256 muestras y la anticipación temporal de 4096 Fs. No se incrementan los límites históricos de 256 Fs y 2205 Fs para convertir fallos en aprobados. Los presupuestos de admisión de trabajo son anteriores al evento y no tolerancias adicionales de reproducción.

El decoder por pares reduce el bloque nativo de 128 muestras de 136,726 a 93,095 Fs (31,91 %) y conserva 1.034.226 muestras, 24.942 comprobaciones de estado y 11.314 casos de borde. El refill admite 32, 64 o 128 muestras según el tiempo restante antes del siguiente evento; los máximos instrumentados medidos fueron 43, 65 y 110 Fs. La preparación futura recibe una oportunidad por vuelta y deja las otras dos al relleno de voces vivas. Los inicios SFX preparan 128 muestras; la preparación musical conserva 256. No se han ampliado rings, FIFO ni tolerancias.

El fallo de `1F` también se ha separado de la presión de cola: la FIFO FM observada ocupa sólo 0–22 entradas mientras crece el retraso del secuenciador. El camino original sigue modificando RAM y registros; no es un bucle vacío que se pueda saltar legítimamente. La optimización elegida no autoriza omitir ese trabajo. Las transiciones negativas de los perfiles anteriores se conservan, aunque sus pruebas lógicas de host sean exactas.

### Corte verificado del comando `0x01`

Los perfiles `probe09` instrumentado y `probe10` sin instrumentación completaron 35,034 segundos nativos, con 2.153 eventos de transporte cada uno. Ambos terminaron con cero descartes FM, cero rechazos, cero avisos de retraso, cero underruns reales de 32 bits y cero errores API/worker.

En el perfil instrumentado se midieron 159 lanzamientos preparados y ningún fallback; máximo de preparación de 64 muestras: 103 Fs; refill: 149 Fs; retraso máximo hasta completar un lanzamiento: 151 Fs. Esta evidencia se refiere a esa ejecución concreta. La matriz completa de 18 comandos detectó 56 avisos de retraso repartidos entre doce músicas del perfil10; sólo seis aprobaron todos sus contadores. Sus 18 capturas y 36 WAV comparativos de 30 segundos están completos y sus 54 contenedores (originales y recortes) se validaron. Por ello ese perfil **no cumple el criterio global de cero retrasos**.

Un cero en B03A no sustituye los contadores individuales. Un cero en el mailbox tampoco sustituye el campo real `wf_sfx_engine.underruns`, de 32 bits. Las herramientas actuales observan el campo correcto usando el mapa del firmware exacto.

### Matriz nativa larga del candidato actual

El firmware `cddd` y SH-2 `scalar9/seqOs` completaron los 18 ensayos de 600 segundos seguidos de ocho segundos tras `1F`, más la secuencia separada de transiciones. La reauditoría v2 vincula los 19 resultados a sus ELF, firmware, F5 y WAV: las 19 evidencias están completas, con cero errores de identidad/captura. **Cuatro músicas aprueban los controles de ejecución; catorce y la transición fallan.** Esto no es una aprobación de fidelidad.

| Comando ROM | FM descartados individuales | Avisos tardíos | Error worker final | Control de ejecución |
| --- | ---: | ---: | ---: | --- |
| `0x01` | 1 | 0 | 15 | FAIL |
| `0x02` | 3 | 34 | 15 | FAIL |
| `0x03` | 2 | 4 | 15 | FAIL |
| `0x04` | 4 | 3 | 15 | FAIL |
| `0x05` | 0 | 0 | 0 | PASS |
| `0x06` | 1 | 2 | 15 | FAIL |
| `0x07` | 1 | 0 | 15 | FAIL |
| `0x08` | 0 | 0 | 0 | PASS |
| `0x09` | 0 | 0 | 0 | PASS |
| `0x0A` | 1 | 9 | 15 | FAIL |
| `0x0B` | 5 | 2 | 15 | FAIL |
| `0x0C` | 2 | 25 | 15 | FAIL |
| `0x0D` | 1 | 1 | 15 | FAIL |
| `0x0E` | 1 | 8 | 15 | FAIL |
| `0x0F` | 1 | 4 | 15 | FAIL |
| `0x10` | 0 | 0 | 0 | PASS |
| `0x11` | 2 | 4 | 15 | FAIL |
| `0x12` | 5 | 1 | 18 | FAIL |

Las 18 músicas suman **30 eventos FM descartados, 97 avisos y cero underruns PCM**. `0x12` añade un rechazo de ring y termina con error18 (KEY_DEADLINE); los otros trece fallos musicales terminan con error15 (FM_DEADLINE). Los plazos duros continúan en2205Fs, aproximadamente50ms. Estos contadores son eventos de transporte/registros; no se equiparan a un número de notas. Todas las músicas admiten sus dos palabras inicial/`1F`.

La transición admite sólo4/9 palabras, descarta1 evento FM y registra2 avisos, con0 underruns y error15. Un subgate de análisis interpretaba filas vacías como retorno ACCEPTED; la regresión reproduce ese error y lo corrige. El resultado global antiguo ya era FAIL y se conserva como tal.

Los cuatro PASS son `05`, `08`, `09` y `10`. Los tres primeros terminan naturalmente al principio de la ventana; no representan diez minutos continuos de música. `10` conserva continuidad y cola en su ensayo. Todos los últimos dos segundos resultan digitalmente silenciosos, pero el silencio causado por un fail-stop no demuestra un fade correcto. Los controles previos observados permanecen limpios; para03/04 existe un intervalo entre observaciones que cruza la entrega de `1F`, por lo que no se atribuye un instante preciso al inicio del fallo. Los tiempos y el alcance de bucles se detallan en [MUSIC-STRUCTURE-AND-LONG-VALIDATION.md](docs/MUSIC-STRUCTURE-AND-LONG-VALIDATION.md).

La primera campaña sufrió ENOSPC durante11/12. Sus archivos incompletos se conservan. Los dos reintentos usan exactamente los mismos ELF/ISO y superan la auditoría de captura; además, sus PCM coinciden byte a byte con todo el prefijo recuperable de cada intento anterior. La colección reconciliada registra esta procedencia sin reescribir la campaña fallida. [Informe de los19 casos](docs/NATIVE-LONG-FINAL-CANDIDATE.md) y [metadatos de auditoría](docs/evidence/native-long-final-cddd-scalar9.json).

## 4. Fidelidad: pruebas positivas y límites

### Secuencias y PCM

- Las 18 músicas se ejecutaron durante 600 segundos más ocho segundos de fade por comando en el test diferencial del host: 10.962 checkpoints comparan CPU, RAM, timers, ciclos y hash/contador de eventos con R01. No se sustituyen bucles por grabaciones.
- Las transiciones originales `01,06,12,1F,03,1F,02,01,1F` también coinciden en 89.011 eventos. El comando `00` se verifica en el nivel de módulo/API que lo implementa, separado de la entrada del intérprete.
- La comparación independiente A53 de 18 × 30 segundos cubre 923.055 eventos YM/OKI/banco, incluyendo sus ciclos. Los primeros doce segundos de los 18 renders A53 coinciden con los hashes históricos.
- La decodificación completa cubre 114 entradas de muestra y 1.034.226 frames contando aliases. El modelo A15 comprueba su estado lógico y prioridades, pero se ha demostrado que interpreta mal 17 reinicios `F1`; no es el oráculo de inicios físicos ni de banco activo. El test original completo cubre los 98 comandos y falla con la implementación antigua antes de aprobar la corrección. La preparación incremental conserva las muestras y el estado del decoder.

Estas pruebas no demuestran por sí solas el timbre, un loop completo de cada composición en Saturn, el rendimiento bajo carga ni la exactitud de toda la electrónica del arcade.

El test diferencial largo R01 y la regresión A53 conservan el MMIO histórico basado en Timer B. La comparación posterior con el modelo full-board verifica el orden/valor de los prefijos YM no-control de las 18 ventanas de 30 s, pero encuentra diferencias de ciclos y de borde de ventana. No se llama ROM-EXACT temporal a una igualdad obtenida sólo contra el modelo parcial.

### Inicios `F1` y banco activo

El original llega a `JP NZ,0523` con la bandera de continuación activa y omite todo nuevo stop/start. Los comandos afectados son `38,41,43,46,49,4C,51,5C,61,69,6A,6B,6E,6F,70,75,7B`. La corrección omite los eventos codificados con flag1, sin suprimir indebidamente los nuevos inicios flag0/3. El fixture privado se genera desde la ejecución original; no se publica la secuencia derivada de la ROM. [Auditoría F1](docs/F1-ROM-SEQUENCE-AUDIT.md).

El banco OKI original cambia la ventana global de lectura, incluidas las voces activas. El motor actual conserva datos del banco elegido al iniciar la voz y PCM preparado por adelantado. El caso observado se corrige conceptualmente con lectura por dirección original y estado de decodificación en la frontera, no cambiando sólo un identificador de banco. La auditoría verificó las 517.450 direcciones necesarias: un recurso adicional de 704 bytes cubriría el mapa y los 337 bytes faltantes; un esquema de checkpoints ocuparía al menos 528 bytes. Estos números demuestran factibilidad de almacenamiento, **no un algoritmo de tiempo real ya implementado**. Se distinguen el ring de 4.096 muestras PCM a 8 kHz y el lead de 4.096 Fs a 44,1 kHz. [Auditoría del banco y reproducción](docs/ACTIVE-OKI-BANK-AUDIT.md).

### Traducción FM

Se demostró un factor cuatro excesivo en la profundidad de modulación histórica. Para las entradas normalizadas verificadas, la corrección usa MDL9 con una fuente duplicada, MDL10 con dos fuentes distintas y `FB+1` para el feedback representable. Los valores anteriores eran 11, 12 y `FB+3`. Una comparación independiente de 726 combinaciones de signo/nivel falla con los valores antiguos y pasa con la corrección.

El nivel TL inicial se corrige de 255 a 254 para coincidir con la sombra OPM de 127 y evitar que una escritura repetida elidida deje un estado distinto en hardware.

El SCSP no es un YM2151: cuantización de frecuencia y envolvente, reloj interno, interpolación, latencia entre operadores, feedback, LFO, ruido y mezcla requieren evaluaciones distintas. Los MDL0–4 no modulan; por tanto el mapeo directo no representa FB1–3. Los primeros 30 segundos de las 18 trazas usan feedback 0/5/7; esa observación no certifica todas las duraciones posibles. El manual original de Sega describe también las dos generaciones de sound stack y el retardo de conexión entre slots: [registro de modulación](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_23.htm), [conexión FM](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_fm2.htm).

La comparación real del comando01, perfil10, continúa mostrando diferencias: centroide espectral de aproximadamente 943,56 Hz en Saturn frente a 554,68 Hz en A53, y error espectral de potencia de 4,685 dB en la banda analizada. La diferencia RMS es −1,048 dB. No se han normalizado las muestras para esconderla. La alineación residual DAC aún no está calibrada. Estas métricas no sustituyen la escucha ni prueban por sí solas cuál de las dos implementaciones está equivocada.

La investigación por canales distinguió un bajo cercano al modelo de otro canal con exceso de energía alta. El contraste independiente del reloj **confirmó un error en A53 y en el mapeo nativo heredado**: se omitía el divisor por dos anterior a los 32 operadores del YM2151. Una nota real de la ROM producía aproximadamente 1397 Hz en lugar de 699 Hz. Se corrigió el mapeo nativo sin cambiar sus notas ni la temporización del controlador; se generaron las 18 referencias nuevas con reloj correcto y PCM16 original. Véase [ARCADE-CLOCK-AUDIT.md](docs/ARCADE-CLOCK-AUDIT.md), con fuentes primarias fijadas y prueba roja/verde. R04/A53 conservan sus bytes y hashes. Las comparaciones antiguas se etiquetan como evidencia del perfil histórico, no como oráculo de tono arcade.

Con la referencia full-board y la captura nativa corregida `621b5dfc`, el comando01 todavía presenta centroide de 505,05 Hz en Saturn frente a 372,35 Hz en referencia, error espectral de 3,487 dB y diferencia RMS de −1,124 dB. No se ha aplicado normalización. Los cambios de referencia no eliminaron la diferencia de timbre. La frecuencia nativa se verificó en 4.194.304 combinaciones con error máximo de 1,468 cents; esa prueba no certifica feedback, LFO, envolvente ni mezcla.

El candidato actual `cddd/scalar9` conserva diferencias de señal en el mismo comando01: centroide498,154 Hz en Saturn frente a372,349 Hz en referencia, error espectral3,442 dB, diferencia RMS−1,133 dB y correlación de envolvente0,486. El centroide describe la distribución espectral, no la afinación de una nota. Los [resultados de escucha medidos de18 músicas y98 comandos](docs/evidence/listening-metrics-final-cddd-scalar9.json) conservan hashes, ventanas, actividad sin umbral y parámetros sin normalización. Estas métricas no sustituyen la escucha.

Los fragmentos musicales se alinean por el reloj de comando y el lead declarado de 4.096 Fs, aproximadamente 92,88 ms. El replay mixto conserva ese retraso relativo en el audio entregado. No se afirma alineación DAC exacta ni latencia de inicio idéntica a la placa arcade.

### Interpolación PCM y escala de captura

Una comparación nativa A/B de `0x21` cambió exclusivamente la interpolación del programa DSP. Los dos ELF difieren en 89 bytes, todos dentro del programa DSP; los coeficientes, slots, direcciones y cuatro rings PCM observados en F5 coinciden. El error espectral del caso disminuye de 17,082 a 2,808 dB al pasar de interpolación lineal a retención de la muestra original (ZOH). Esa variante sigue siendo experimental y no reemplaza el firmware seleccionado ni sus matrices.

El código de Mednafen 1.32.1 aplica después del SCSP una ganancia `27/32`, equivalente a −1,475724 dB. El déficit RMS del experimento ZOH queda a +0,015758 dB de esa predicción. Es una escala del emulador, no una calibración de la placa arcade ni de la salida analógica Saturn. Los WAV entregados conservan sus amplitudes sin compensación. [Prueba causal, fuente y regeneración](docs/PCM-INTERPOLATION-AND-CAPTURE-GAIN.md).

### Dos referencias PCM históricas diferentes

A53 usa su pack firmado de PCM8 y estado inicial ADPCM −2. El modelo de efectos A15 y el decoder nativo auditado usan PCM16 y estado inicial 0. Son baselines distintos. Sus hashes, ganancia heredada y método de muestreo figuran en los informes; no se intercambian en silencio ni se llama captura de placa a un render de software.

## 5. API, memoria, ABI y errores

La API pública de versión1 ofrece inicialización, comandos originales, servicio, VBlank, consulta, parada y recuperación. `BUSY` significa que no se publicó nada y obliga al propietario original a conservar el comando. Un rechazo por prioridad de la ROM se distingue de una pérdida de transporte.

Se validan completamente CRC/estructura del pack y las ventanas ELF antes de escribir hardware. El firmware debe anunciar el perfil musical esperado. Se comprueban ownership, heartbeat del worker y progreso DSP; el contexto no puede sustituirse mientras el worker anterior siga vivo. Parar no equivale a borrar el puntero de entrada del slave: se espera su retorno real. Una recuperación hace un arranque frío; no promete reanudar notas ya perdidas.

El mapa y los presupuestos están en [MEMORY-AND-ABI.md](docs/MEMORY-AND-ABI.md). El mailbox nativo ABI3, el módulo musical ABI4 y la API C versión1 son contratos distintos. No se admite cambiar un número de ABI para cargar objetos incompatibles. El perfil de juego debe reservar el código caliente, su estado, la pila slave y las ventanas VDP1 fuera del heap y del renderer.

El ensayo TAG completo conserva idénticos los7648 bytes de estado y los64 productores originales. Sin audio mide57,902 pasos/s; con el perfil10 mide55,337 pasos/s, con19 underruns y22 eventos tardíos. El primer incidente es agotamiento de una voz, separado de una pausa global posterior de546Fs. Véase [COMBAT-INTEGRATION-GATE.md](docs/COMBAT-INTEGRATION-GATE.md). La integración aislada conserva las funciones de productor y su secuencia de entrada. No añade sonidos a lugares cuyo origen no está demostrado. El catálogo de comandos implementados no es un cierre de las filas pendientes del mapa ROM-EXACT de productores.

La pareja TAG del candidato actual conserva los 7.372 pasos, los actores y los 64 productores. Completa 64/64 comandos con cero pérdidas FM y cero underruns, pero un aviso tardío. Mide 55,330359 pasos/s frente a 57,901837 del control: añade 354 VBlanks, un 4,6475 % de tiempo. El percentil95 del intervalo entre pasos pasa de uno a dos VBlanks; el máximo pasa de dos a tres. Falla rendimiento y plazos aunque la continuidad PCM haya mejorado.

Las reauditorías v2 separan cuatro hechos: API operativa, trabajo solicitado entregado, ausencia de pérdidas/retrasos e identidad del código realmente cargado. La condición de comparabilidad completa exige los cuatro. TAG y Royal4MiB conservan audio operativo, pero sus avisos impiden certificar coste de audio íntegro; Royal base además detiene el worker. Se conservan las tasas observadas y los informes v1, sin convertirlos en un aprobado.

## 6. Estado de aceptación

| Criterio | Estado de este corte |
|---|---|
| Recuperación y conservación histórica | Verificado |
| Generación local desde los hashes ROM correctos | Verificada; recursos protegidos fuera de Git |
| API, carga corrupta, colas y recuperación de software | Host aprobado; dos rechazos corruptos nativos aprobados. La prueba de presión recupera correctamente, pero conserva dos underruns/cuatro avisos anteriores al reinicio |
| 18 músicas: equivalencia de secuenciador durante 600s + fade | Aprobada en host; no certifica salida SCSP |
| 98 efectos: decoder/estado/prioridad e inicios físicos | PCM/estado aprobados dentro de sus modelos; 98 inicios físicos aprobados contra Z80 original; banco activo sigue fallando |
| Native35s comando01, perfil10 | Aprobado en sus contadores de estabilidad |
| Native18 músicas, perfil10 | Completa:6 aprobadas,12 fallidas,56 avisos de retraso; salida de tono histórico |
| Capturas WAV de18 músicas y98 efectos/voces |18 parejas del candidato actual generadas,36WAV/30s;98 efectos con196WAV y auditoría ELF/F5/WAV aprobada. Se conservan reintentos11/12 por ENOSPC y0x38 por automatización, con idénticos ELF/ISO. Actualización de entrega privada bloqueada por autorización del destino; los ZIP anteriores siguen identificados |
| Fidelidad musical física/arquitectónica | Pendiente; discrepancias demostradas en revisión |
| TAG cuatro luchadores con/sin audio, candidato actual | Completo7372 pasos: estado idéntico,0 underruns/0 pérdidas FM,1 aviso y55,330logic/s; falla aceptación |
| Royal HW1 bajo carga, candidato actual | Parejas completas. Base:4 pérdidas FM/0 underruns/37 avisos y fallo de worker. 4MiB:0 pérdidas/0 underruns/6 avisos,133 comandos completos,58,453logic/s. Política original y estado/productores idénticos; no existe cinta de entradas por llamada |
| Matriz nativa18 ×600s +1F8s y transiciones | Cerrada:4 músicas PASS,14 FAIL; transición4/9, FAIL. Cero underruns en esta matriz;30 descartes FM/97 avisos musicales. Bucles y composiciones completas con exactitud de señal siguen pendientes |
| Aprobación auditiva del propietario | Pendiente |
| Saturn NTSC-U con MODE | HARDWARE PENDING |

## 7. Reproducción y entrega

Seguir [CODEX-INTEGRATION.md](CODEX-INTEGRATION.md), el instalador de [entorno](tools/environment/README.md) y las [referencias autocontenidas](tools/reference/README.md). Los comandos de compilación, versiones, mapas y SHA256 se escriben junto a cada ejecución. Las capturas sólo se aceptan cuando el WAV nativo se ha finalizado correctamente; un proceso que sale con código cero no basta si RIFF/data están incompletos.

Los WAV principales son PCM16 a 44.100 Hz. La señal Saturn procede de `soundrecord` de Mednafen1.32.1 real, sin preload, parche de reloj, inyección de memoria ni sustitución por un renderer de PC. Se conservan el arranque, los contadores, el estado nativo F5, el ejecutable exacto y el intervalo recortado. La lectura externa de memoria es únicamente de observación. Los archivos con audio o datos derivados de la ROM se entregan privadamente, fuera de GitHub.

El auditor nativo reforzado lee símbolos del propio ELF, enlaza todos los segmentos inicializados SCSP y las funciones SH-2 con F5, comprueba los contadores reales de RAM y conserva los errores observados antes del último registro. Sus 22 regresiones son sintéticas y no necesitan ROM. Los 97 primeros SFX válidos y el retry separado de0x38 superan esa auditoría; la campaña inicial fallida no se reescribe. El replay final continúa fallando por sus dos avisos bajo las mismas comprobaciones más estrictas.

Se observaron archivos intermedios de referencia truncados, con causa aún no establecida. Los WAV entregados conservaron sus hashes; los intermedios afectados se regeneraron en salidas nuevas y coincidieron con las identidades completas archivadas. La comprobación detectó el problema y se conservó la evidencia anterior. [Integridad de las salidas y guardas reproducidas](docs/REFERENCE-OUTPUT-INTEGRITY.md).

Los generadores de recursos y firmware ahora rechazan salidas existentes, dentro del código o enlazadas a ellas antes de escribir. La prueba negativa con las herramientas anteriores detectó 14 subcasos fallidos; las seis regresiones nuevas pasan y la reconstrucción protegida conserva exactamente el firmware `cddd`. El runner host incorpora esas guardas y las 22 pruebas sintéticas de identidad de captura: sus 11 grupos pasan en el ensayo de herramientas de un segundo por comando. Ese ensayo no sustituye los nueve grupos del perfil de runtime verificado durante 600 segundos. [Resultados de herramientas](docs/evidence/final-tooling-host-smoke.json).

No fusionar el PR mientras exista un criterio de software fallido o pendiente. [NEXT-SESSION.md](NEXT-SESSION.md) establece el primer experimento pendiente, los comandos de reproducción y la regresión que debe seguir a una corrección. La siguiente sesión debe partir de los hashes y manifiestos de este corte y conservar tanto la evidencia negativa como la positiva.

## 8. Artefactos privados preparados y autorización de entrega

Los dos ZIP de validación están cerrados y verificados después de escribir: artefactos/informes (4.371 archivos seleccionados) y127 estados F5. Incluyen el firmware seleccionado, ELF, mapas, discos de prueba, configuración, comandos y resultados de las18 músicas, transición,98 SFX, mezcla, lifecycle, corrupción de recursos y seis capturas de combate. Los informes originales no se editan; los manifiestos identifican las rutas preservadas. Los SHA de cada miembro y los hashes cruzados permiten detectar una mezcla de partes. La herramienta de selección tiene siete pruebas sintéticas y excluye ROM/BIOS originales, árboles de fuentes y medios que se entregan aparte.

La revisión automática rechazó actualizar el ZIP privado de efectos por exigir autorización explícita del destino de entrega. Se verificó que sus196WAV eran idénticos a los ya entregados; un segundo intento con esa comprobación también fue rechazado. No se cambió de medio para eludirlo. Quedan preparados cuatro ZIP: músicas del candidato actual, revisión de metadatos SFX y las dos partes de validación. Las entregas anteriores siguen identificadas separadamente. [Manifiesto público de archivos, tamaños, hashes y estado](docs/evidence/private-delivery-status.json).

Esta autorización de destino es independiente de la aprobación auditiva y no aprueba el motor. Los fallos de software indicados en esta auditoría siguen bloqueando su aceptación.
