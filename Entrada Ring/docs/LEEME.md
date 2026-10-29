# WP01 · Entrada TAG · R02 con audio nativo

Esta revisión conserva la entrada TAG de R01 y sustituye su pista mezclada por síntesis y reproducción de voces en el SCSP de Saturn. Es una primera versión nativa funcional, con diferencias de timbre frente al YM2151 original todavía documentadas. La ROM y los eventos observados en el arcade son la referencia.

## Archivos y uso

- `WF-WP01-R02.cue` y `WF-WP01-R02.bin`: imagen de una pista MODE1/2352, con EDC/ECC comprobados en todos sus sectores. Mantener ambos archivos juntos y abrir/cargar el CUE.
- Región del ejecutable: USA. Configuración validada: Saturn NTSC, sin cartucho de RAM adicional.
- `START`: elige otra pareja aleatoria legal y reinicia la entrada.
- `A`: repite la pareja actual.
- Al terminar, la entrada se repite automáticamente. Se conservan las 29 parejas válidas de R01; Demolition permanece emparejado.
- `VERIFICACION.json` y `SHA256.txt`: identificación del disco y resultados de las pruebas.

El alcance sigue siendo la escena de pasillo de entrada al ring. Esta revisión se centra en su audio; no incorpora el combate ni la presentación posterior dentro del ring.

## Qué genera Saturn

Los gráficos conservan el controlador y los recursos de R01: VDP1 dibuja los personajes y VDP2 el entorno. Los 15 archivos gráficos y `wp01_scene.c` permanecen idénticos byte a byte.

La música usa seis voces FM de cuatro operadores: 24 slots SCSP. Sus notas, instrumentos, algoritmos, realimentación y cambios de registro proceden de la secuencia original de entrada. El SCSP produce las oscilaciones, modulación, envolventes y mezcla en tiempo real. La onda base es un único ciclo de seno derivado de las tablas OPM, repetido únicamente para proteger las direcciones de modulación. No contiene notas ni música grabadas.

Los slots 24–27 quedan destinados a las voces de muestras del OKI original. En esta escena se emplean las muestras de percusión y público, decodificadas individualmente desde el ADPCM de la ROM con sus niveles originales. Usar muestras individuales es parte de la reproducción nativa del SCSP; no se reproduce una grabación de la escena.

La conversión de 748 escrituras significativas de los chips originales produce 488 escrituras iniciales y 2.224 escrituras temporizadas al SCSP. El SH-2 envía esos registros mediante interrupciones FRT independientes del dibujo. No se ejecuta en Saturn el Z80 original ni un mezclador de música por software. Se reproduce la secuencia de esta escena, convertida previamente a instrumentos y registros nativos.

`SOUNDRAM.BIN` ocupa 346.208 bytes de los 524.288 disponibles y `AUDIOREG.BIN` ocupa 19.776 bytes. Los datos se cargan antes de la escena y permanecen residentes. El disco R02 no contiene `ENTRY.PCM`.

## Fidelidad y diferencias pendientes

La secuencia, instrumentos y muestras se han contrastado con el juego original en MAME 0.264. Se corrigió el estado inicial del decodificador OKI y se conservaron los disparos que se separan por pocas muestras. La temporización se basa en el reloj nominal NTSC de 320 píxeles, corroborado con el manual Sega SMPC.

La síntesis SCSP no es idéntica al YM2151: difieren la resolución y forma de las envolventes, la cuantización de frecuencia, los retardos de modulación/realimentación y la interpolación. El puente mantiene la estructura de los instrumentos, pero todavía se oyen diferencias de timbre. El inicio tampoco reconstruye la cola heredada de aproximadamente 17,6 ms del sonido de la selección anterior. No se presenta esta revisión como audio idéntico al arcade.

La repetición automática conserva la duración de escena de R01, 424 fotogramas Saturn. Reiniciar la escena corta el último golpe que aún esté sonando; la ventana de referencia extraída del arcade es ligeramente más larga. El informe de audio delimita los intervalos comparados.

## Verificación realizada

- El controlador de escena vuelve a coincidir con 756 estados de actores capturados en MAME. La prueba de parejas valida las 29 combinaciones legales.
- En la ejecución instrumentada de Yabause se observaron 1.322 fotogramas activos, tres entradas completas y comienzo de la cuarta: cero errores y cero VBlank perdidos. Otra ejecución probó dos cambios de pareja durante la música, también sin errores ni VBlank perdidos.
- El máximo observado para una interrupción de audio fue 96 ticks FRT, aproximadamente 0,114 ms. Es una medición de esa prueba en emulador; no es una medición de una consola física ni del coste total de audio por fotograma.
- El mismo BIN arrancó en Mednafen 1.29.0 con BIOS USA real y sin expansión de RAM. El vídeo entregado procede de esa ejecución.
- En el audio original de la captura se localizaron los 19 disparos PCM de cada una de las tres primeras vueltas, con dispersión relativa de 0–1 muestras a 44,1 kHz. No hubo saturación digital. La comparación con el modelo SCSP respalda la continuidad de las seis voces FM y las muestras, con las limitaciones expresas del informe.
- El desplazamiento de R01 se conserva: 333 intervalos de refresco NTSC, aproximadamente 5,566 s, frente a 5,571 s de la referencia arcade. La prueba de R02 no perdió presentaciones. La velocidad física aún debe confirmarse en la consola del usuario.

La prueba en Saturn físico está pendiente. El BIN/CUE es el mismo que se verificó y grabó en el emulador; PAL no forma parte de la configuración validada.

## Vídeo

El MP4 muestra tres entradas sucesivas. Conserva exactamente los 1.278 fotogramas elegidos y sus marcas temporales del MOV de Mednafen; no se ha acelerado, interpolado ni sustituido el sonido. Se recortaron los bordes de la captura y se amplió la imagen a 960×720 con vecino más cercano. El audio es el capturado por Mednafen, codificado a AAC, sin normalización ni fundidos. El corte final cae en un intervalo de silencio real anterior a la cuarta entrada.

El parámetro de captura `-nothrottle` sólo permite que Mednafen calcule los fotogramas más deprisa en el ordenador. Las marcas temporales nativas del MOV y del MP4 conservan la velocidad de Saturn.

## Fuentes y trazabilidad

El ZIP de fuentes contiene la copia independiente R02, sus recursos, conversores de eventos e instrumentos, pruebas y evidencias. `COMPILAR.md` explica cómo reconstruir el disco y regenerar sus dos bancos de audio. Los WAV de las carpetas de investigación son exclusivamente referencias de validación; no se empaquetan como música del programa.

La verificación final comprueba los hashes de 83 archivos protegidos de las fuentes originales y R01. No se ha modificado ningún repositorio ni trabajo anterior. Los archivos de investigación conservan sus fechas y estados intermedios; `VERIFICACION.json` y los informes finales de `research_events/final_emulator/` describen el resultado integrado.

Referencias primarias de hardware y modelos:

- Sega, SCSP User's Manual, ST-077-R2-052594: https://antime.kapsi.fi/sega/files/ST-077-R2-052594.pdf
- Sega, SMPC User's Manual, ST-169-R1-072694, tabla 1.1: https://antime.kapsi.fi/sega/files/ST-169-R1-072694.pdf
- Hitachi, SH7604 Hardware Manual, registros FRT: https://antime.kapsi.fi/sega/files/sh7604.pdf
- MAME 0.264, controlador de WrestleFest y chips de sonido: https://github.com/mamedev/mame/tree/mame0264
- YMFM, tablas y síntesis OPM: https://github.com/aaronsgiles/ymfm
- Mednafen, modelo SCSP de referencia: https://github.com/libretro/beetle-saturn-libretro/tree/master/mednafen/ss

Los hashes de las versiones efectivamente utilizadas constan en las evidencias y manifiestos incluidos. Las licencias de los modelos y tablas acompañan a sus fuentes.
