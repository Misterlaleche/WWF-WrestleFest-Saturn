# Audio 2 — auditoría de entrega

**Estado: NOT PRODUCTION READY. HARDWARE PENDING.**

El nombre de la rama `audio-2-production-ready` identifica el encargo, no una certificación. Esta entrega conserva un motor nativo ejecutable, herramientas de regeneración y evidencia positiva y negativa. Los criterios pendientes o fallidos no se convierten en aprobados por disponer de una API, un ELF o un estado `RUNNING`.

La autoridad de datos sigue siendo el `wwfwfest.zip` original. R04, A53 y todas las implementaciones históricas permanecen intactos. Una coincidencia con un modelo histórico prueba esa coincidencia; no demuestra por sí sola que el modelo reproduzca correctamente el reloj o la salida física del arcade.

## 1. Recuperación y selección de fuentes

Punto de partida: `audio-2`, commit `db18be40a1d79de200408207573e9f45bf73bf2c`. Se recuperaron 415 archivos seleccionados, correspondientes a 304 blobs distintos, comprobando sus identidades Git. El verificador del paquete histórico aprobó sus 380 archivos. Se inspeccionaron README, ESTADO, INTEGRACION, COMPILACION, experimentos, base y evidencia de Audio 2; los documentos R04, validación física y propiedad de productores; PROJECT-STATE y la issue #2.

La comprobación posterior de las referencias remotas seguía mostrando `audio-2` en ese checkpoint y `main` en `aedca6827c7801a88dbb1eae280830cd4776b95f`. La publicación usa el árbol remoto real como base; el checkout local parcial nunca se utiliza para eliminar archivos remotos que no fueron recuperados.

| Componente | Selección y razón |
|---|---|
| Control Z80 musical | Ruta `z80-inline-tabla`: el estado CPU/RAM/timers y los eventos se contrastan con el intérprete R01 conservado. La selección no depende del nombre ni de la fecha de la carpeta. |
| PCM y comandos | Decoder y secuenciador nativos, más preparación incremental derivada del experimento `pcm-servicio`; se verifica contra todas las muestras y el modelo A15 independiente. |
| Transporte | Conserva los tamaños y plazos originales; añade servicio cooperativo, errores explícitos y contadores de pérdidas individuales. |
| FM SCSP | Puente nativo con correcciones demostradas de TL inicial y profundidad de modulación. La fidelidad de salida sigue siendo un criterio separado. |
| Referencia | R04/A53 se reproducen con sus hashes intactos. Las discrepancias halladas en su configuración se investigan en perfiles separados. |
| Carga del juego | Master y módulo ABI4 deben compilarse juntos con sus reservas reales; el diagnóstico estático no certifica esa integración. |

Todos los cambios de esta entrega residen bajo `Audio 2/production/`. No se sustituyen WP1979, WP1980, frontend, TAG, Royal, gameplay, gráficos, otras escenas ni la referencia R04.

## 2. Arquitectura implementada y coste que conserva

El SH-2 principal conserva un único propietario de la API y entrega las palabras originales. El SH-2 secundario ejecuta el controlador musical original mediante el intérprete optimizado y traduce/escribe registros SCSP en sus instantes programados. No sintetiza muestras con Nuked-OPM. El 68000 atiende el transporte PCM, el secuenciador de efectos, la decodificación ADPCM, la preparación y el relleno de rings. El SCSP genera los operadores FM con sus 32 slots, y su DSP mezcla cuatro voces PCM mediante un programa de 120 pasos.

Esto descarga la generación de muestras del SH-2, pero **todavía reserva el SH-2 secundario**. No se presenta como un servicio de coordinación de coste despreciable. El presupuesto del combate debe aceptarlo con una comparación reproducible con/sin audio. Nuked-OPM se usa exclusivamente en herramientas de referencia del ordenador.

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

### Corte verificado del comando `0x01`

Los perfiles `probe09` instrumentado y `probe10` sin instrumentación completaron 35,034 segundos nativos, con 2.153 eventos de transporte cada uno. Ambos terminaron con cero descartes FM, cero rechazos, cero avisos de retraso, cero underruns reales de 32 bits y cero errores API/worker.

En el perfil instrumentado se midieron 159 lanzamientos preparados y ningún fallback; máximo de preparación de 64 muestras: 103 Fs; refill: 149 Fs; retraso máximo hasta completar un lanzamiento: 151 Fs. Esta evidencia se refiere a esa ejecución concreta. La matriz completa de 18 comandos detectó 56 avisos de retraso repartidos entre doce músicas del perfil10; sólo seis aprobaron todos sus contadores. Sus 18 capturas y 36 WAV comparativos de 30 segundos están completos y sus 54 contenedores (originales y recortes) se validaron. Por ello ese perfil **no cumple el criterio global de cero retrasos**.

Un cero en B03A no sustituye los contadores individuales. Un cero en el mailbox tampoco sustituye el campo real `wf_sfx_engine.underruns`, de 32 bits. Las herramientas actuales observan el campo correcto usando el mapa del firmware exacto.

## 4. Fidelidad: pruebas positivas y límites

### Secuencias y PCM

- Las 18 músicas se ejecutaron durante 600 segundos más ocho segundos de fade por comando en el test diferencial del host: 10.962 checkpoints comparan CPU, RAM, timers, ciclos y hash/contador de eventos con R01. No se sustituyen bucles por grabaciones.
- Las transiciones originales `01,06,12,1F,03,1F,02,01,1F` también coinciden en 89.011 eventos. El comando `00` se verifica en el nivel de módulo/API que lo implementa, separado de la entrada del intérprete.
- La comparación independiente A53 de 18 × 30 segundos cubre 923.055 eventos YM/OKI/banco, incluyendo sus ciclos. Los primeros doce segundos de los 18 renders A53 coinciden con los hashes históricos.
- La decodificación completa cubre 114 entradas de muestra y 1.034.226 frames contando aliases. Los 98 comandos y los ensayos mixtos comprueban bancos, prioridades, simultaneidad, supresión y terminación contra el modelo A15. La preparación incremental conserva las muestras y el estado del decoder.

Estas pruebas no demuestran por sí solas el timbre, un loop completo de cada composición en Saturn, el rendimiento bajo carga ni la exactitud de toda la electrónica del arcade.

### Traducción FM

Se demostró un factor cuatro excesivo en la profundidad de modulación histórica. Para las entradas normalizadas verificadas, la corrección usa MDL9 con una fuente duplicada, MDL10 con dos fuentes distintas y `FB+1` para el feedback representable. Los valores anteriores eran 11, 12 y `FB+3`. Una comparación independiente de 726 combinaciones de signo/nivel falla con los valores antiguos y pasa con la corrección.

El nivel TL inicial se corrige de 255 a 254 para coincidir con la sombra OPM de 127 y evitar que una escritura repetida elidida deje un estado distinto en hardware.

El SCSP no es un YM2151: cuantización de frecuencia y envolvente, reloj interno, interpolación, latencia entre operadores, feedback, LFO, ruido y mezcla requieren evaluaciones distintas. Los MDL0–4 no modulan; por tanto el mapeo directo no representa FB1–3. Los primeros 30 segundos de las 18 trazas usan feedback 0/5/7; esa observación no certifica todas las duraciones posibles. El manual original de Sega describe también las dos generaciones de sound stack y el retardo de conexión entre slots: [registro de modulación](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_23.htm), [conexión FM](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_fm2.htm).

La comparación real del comando01, perfil10, continúa mostrando diferencias: centroide espectral de aproximadamente 943,56 Hz en Saturn frente a 554,68 Hz en A53, y error espectral de potencia de 4,685 dB en la banda analizada. La diferencia RMS es −1,048 dB. No se han normalizado las muestras para esconderla. La alineación residual DAC aún no está calibrada. Estas métricas no sustituyen la escucha ni prueban por sí solas cuál de las dos implementaciones está equivocada.

La investigación por canales distinguió un bajo cercano al modelo de otro canal con exceso de energía alta. El contraste independiente del reloj **confirmó un error en A53 y en el mapeo nativo heredado**: se omitía el divisor por dos anterior a los 32 operadores del YM2151. Una nota real de la ROM producía aproximadamente 1397 Hz en lugar de 699 Hz. Se corrigió el mapeo nativo sin cambiar sus notas ni la temporización del controlador; se generaron las 18 referencias nuevas con reloj correcto y PCM16 original. Véase [ARCADE-CLOCK-AUDIT.md](docs/ARCADE-CLOCK-AUDIT.md), con fuentes primarias fijadas y prueba roja/verde. R04/A53 conservan sus bytes y hashes. Las comparaciones antiguas se etiquetan como evidencia del perfil histórico, no como oráculo de tono arcade.

### Dos referencias PCM históricas diferentes

A53 usa su pack firmado de PCM8 y estado inicial ADPCM −2. El modelo de efectos A15 y el decoder nativo auditado usan PCM16 y estado inicial 0. Son baselines distintos. Sus hashes, ganancia heredada y método de muestreo figuran en los informes; no se intercambian en silencio ni se llama captura de placa a un render de software.

## 5. API, memoria, ABI y errores

La API pública de versión1 ofrece inicialización, comandos originales, servicio, VBlank, consulta, parada y recuperación. `BUSY` significa que no se publicó nada y obliga al propietario original a conservar el comando. Un rechazo por prioridad de la ROM se distingue de una pérdida de transporte.

Se validan completamente CRC/estructura del pack y las ventanas ELF antes de escribir hardware. El firmware debe anunciar el perfil musical esperado. Se comprueban ownership, heartbeat del worker y progreso DSP; el contexto no puede sustituirse mientras el worker anterior siga vivo. Parar no equivale a borrar el puntero de entrada del slave: se espera su retorno real. Una recuperación hace un arranque frío; no promete reanudar notas ya perdidas.

El mapa y los presupuestos están en [MEMORY-AND-ABI.md](docs/MEMORY-AND-ABI.md). El mailbox nativo ABI3, el módulo musical ABI4 y la API C versión1 son contratos distintos. No se admite cambiar un número de ABI para cargar objetos incompatibles. El perfil de juego debe reservar el código caliente, su estado, la pila slave y las ventanas VDP1 fuera del heap y del renderer.

El ensayo TAG completo conserva idénticos los7648 bytes de estado y los64 productores originales. Sin audio mide57,902 pasos/s; con el perfil10 mide55,337 pasos/s, con19 underruns y22 eventos tardíos. El primer incidente es agotamiento de una voz, separado de una pausa global posterior de546Fs. Véase [COMBAT-INTEGRATION-GATE.md](docs/COMBAT-INTEGRATION-GATE.md). La integración aislada conserva las funciones de productor y su secuencia de entrada. No añade sonidos a lugares cuyo origen no está demostrado. El catálogo de comandos implementados no es un cierre de las filas pendientes del mapa ROM-EXACT de productores.

## 6. Estado de aceptación

| Criterio | Estado de este corte |
|---|---|
| Recuperación y conservación histórica | Verificado |
| Generación local desde los hashes ROM correctos | Verificada; recursos protegidos fuera de Git |
| API, carga corrupta, colas y recuperación de software | Tests de host aprobados con fuente estable |
| 18 músicas: equivalencia de secuenciador durante 600s + fade | Aprobada en host; no certifica salida SCSP |
| 98 efectos: decoder/estado/prioridad contra modelo independiente | Aprobado en host |
| Native35s comando01, perfil10 | Aprobado en sus contadores de estabilidad |
| Native18 músicas, perfil10 | Completa:6 aprobadas,12 fallidas,56 avisos de retraso; salida de tono histórico |
| Capturas WAV de18 músicas y98 efectos/voces | Generación nativa en curso; sólo manifiestos finalizados acreditan disponibilidad |
| Fidelidad musical física/arquitectónica | Pendiente; discrepancias demostradas en revisión |
| TAG cuatro luchadores con/sin audio | Completo7372 pasos: estado idéntico, pero19 underruns,22 tardíos y55,337logic/s; falla aceptación |
| Royal bajo carga con idéntica entrada | Pendiente de escenario/input verificable |
| Bucles completos, transiciones y estrés prolongado nativos | Pendiente de cubrir toda la matriz |
| Aprobación auditiva del propietario | Pendiente |
| Saturn NTSC-U con MODE | HARDWARE PENDING |

## 7. Reproducción y entrega

Seguir [CODEX-INTEGRATION.md](CODEX-INTEGRATION.md), el instalador de [entorno](tools/environment/README.md) y las [referencias autocontenidas](tools/reference/README.md). Los comandos de compilación, versiones, mapas y SHA256 se escriben junto a cada ejecución. Las capturas sólo se aceptan cuando el WAV nativo se ha finalizado correctamente; un proceso que sale con código cero no basta si RIFF/data están incompletos.

Los WAV principales son PCM16 a 44.100 Hz. La señal Saturn procede de `soundrecord` de Mednafen1.32.1 real, sin preload, parche de reloj, inyección de memoria ni sustitución por un renderer de PC. Se conservan el arranque, los contadores, el estado nativo F5, el ejecutable exacto y el intervalo recortado. La lectura externa de memoria es únicamente de observación. Los archivos con audio o datos derivados de la ROM se entregan privadamente, fuera de GitHub.

No fusionar el PR mientras exista un criterio de software fallido o pendiente. La siguiente sesión debe partir de los hashes y manifiestos de este corte, repetir primero el caso concreto fallido y conservar tanto la evidencia negativa como la positiva.
