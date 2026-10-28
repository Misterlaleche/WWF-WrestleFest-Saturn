# WrestleFest Saturn: acciones y residencia de sprites (actualizado 28-10-2026)

Estado de desarrollo interno. No es una entrega BIN/CUE ni una declaración de
100 % de movimientos y animaciones.

## Barra de cierre total

`░░░░░░░░░░ 0/143 selectores certificados (0 %)`

## Respuesta de colisión `$24AA8` (28-10-2026)

Se completaron sus dos rutas, que anteriormente bloqueaban o dejaban la
limpieza de agarre incompleta. Pasaron 4.096 comparaciones controladas con la
ROM. La ruta de limpieza apareció naturalmente en Royal HOST instrumentado y
su estado capturado coincidió con la ROM en una comparación aislada. El juego
continuó hasta 14.000 frames en ese recorrido. El ejecutable SH-2 compiló y
pasó la comprobación de memoria con 18.700 bytes de heap frente a 16.384
reservados. No se ha probado en Saturn ni acredita todas las variantes. La
evidencia está en `CURRENT-STATE-LATEST.md`; barra estricta `0/143`.

## Respuesta de colisión `$24924` (28-10-2026)

Se corrigió la limpieza incompleta del agarre en la rama de orientación igual.
Coincide con la ROM en 3.072 variantes controladas; la rama opuesta pasó 4.167.
Cuatro recorridos HOST instrumentados no alcanzaron la rama nueva, por lo que
su aparición natural sigue pendiente. La evidencia y los límites se detallan
en `CURRENT-STATE-LATEST.md`. Barra estricta `0/143`.

## Respuesta de colisión `$24868` (28-10-2026)

Se completó la respuesta de agarre de luchadores opuestos, anteriormente
bloqueada en la ruta de temporizador bajo e incompleta en otras rutas. Pasaron
8.192 comparaciones controladas con la ROM, dos guardas de punteros y las
regresiones indicadas en `CURRENT-STATE-LATEST.md`. Una trayectoria TAG HOST
avanzó 14.000 frames y una Royal acabó en el frame 9.322; no acreditan que se
haya alcanzado esta rama ni sustituyen Saturn real. El ELF SH-2 enlazó y pasó
las comprobaciones de pilas y presupuesto: 18.956 bytes de heap frente a los
16.384 reservados. Barra estricta `0/143`.

## Incidente TAG real: colisión (28-10-2026)

Las cinco fotos de R05 apuntan a `$24062` con `$24408`, pero la pantalla antigua
no identificó el par ni la segunda respuesta, necesaria para identificar la
rama exacta. Se amplió el diagnóstico en caso de fallo; no se puede recuperar
esa información de las fotos ya tomadas. En la investigación se corrigió el
uso del vínculo equivocado en la respuesta `$24A90`; 30 casos controlados
coinciden con la ROM y una prueba del detector completo pasa. Una partida
HOST TAG con Hogan y Boss Man avanzó 14.000 frames con selección automatizada. No se
ha demostrado que esta fuera la causa concreta del bloqueo físico. El ELF SH-2
actual enlazó y pasó las comprobaciones de memoria y pila, con 19.212 bytes de
heap para una reserva de 16.384. Detalles y limitaciones en
`CURRENT-STATE-LATEST.md`. Barra: `0/143`.

## Interrupciones de agarre (28-10-2026)

Se corrigieron dos respuestas incompletas de colisión (`$24A42` y `$24A06`),
restituyendo la limpieza y los cambios de estado y vínculo de los luchadores.
Pasaron respectivamente 1.024 y 3.072 comparaciones controladas con la ROM,
y los rechazos de punteros inválidos; Royal HOST llegó a GAME OVER en el
frame 9.322 y TAG avanzó 14.000 frames. Esto no acredita nuevas apariciones
naturales de esas ramas, fluidez, emulación Saturn ni consola física. Quedan
otras ramas abiertas. El ejecutable SH-2 enlazó, pasó la comprobación de pilas
y deja 20.300 bytes de heap frente a los 16.384 reservados. Los informes,
límites y comparación frente a R05 están en `CURRENT-STATE-LATEST.md`.
Barra estricta: `0/143`.

## Dirección y colisiones del combate (28-10-2026)

La tabla duplicada del preprocesador tenía tres valores distintos de la ROM:
arriba y las diagonales superiores. Se sustituyó por la tabla común cotejada
con `$F2D6`; 32 casos de entrada para caminar y agarre móvil coinciden en RAM
y D0–D7. También se corrigieron registros y la lectura ROM `$00A3` en la
carrera, con 384 variantes controladas (32 con RNG) y doce capturas reutilizadas.

La trayectoria Royal de agarres encontró dos bloqueos en el frame 3834. Las
ramas `$24A88` y `$249BE/$249E6` se implementaron y cotejaron con la ROM en
49 y 289 casos. La misma trayectoria HOST llegó al resultado en el frame 6332,
y un TAG de agarres avanzó 14.000 frames. Evidencia detallada y límites en
`CURRENT-STATE-LATEST.md`; no son pruebas de velocidad ni Saturn física.
El ELF SH-2 actualizado enlazó y pasó pila/presupuesto de memoria con 21.452
bytes de heap disponible; se guardó en `validation/local-20261028/control-collision-build/`.
Las seis pruebas afectadas por hashes de código se actualizaron en
`validation/local-20261028/revalidate-collision-final`; barra `0/143`.

## Continuación de Royal tras GAME OVER (28-10-2026)

La R05 se detenía en un bucle después de `$1B1E`. La continuación implementada
reúne los participantes, llama al ranking y vuelve al frontend; se ha añadido
la solicitud de guardado a la ruta Royal. La prueba diferencial cubre 128
variantes y las 16 máscaras de participantes (`royal-gameover-collect-rom.json`);
una partida HOST natural terminó en frame 2872, llamó al ranking y regresó al
título (`royal-natural-post/host.log`). Una regresión TAG pasó 337 casos.
El ELF SH-2 enlazó y pasó las verificaciones de pilas y presupuesto de heap;
quedaron 21.900 bytes disponibles para heap frente a 16.384 reservados.
No es una prueba de iniciales, guardado BUP ni Saturn física. Los detalles y
la comprobación de hashes del inventario están en `CURRENT-STATE-LATEST.md`.

## `$07`: captura natural adicional en Royal (28-10-2026)

Se localizó una captura HOST histórica de Royal en la que el personaje 8
ejecuta naturalmente `$07`, desde el frame 3023 hasta el 3060. La continuación
de 38 pasos del ejecutable actual coincide con la ROM en acción, resolución,
física, RAM, primer plano, D0–D7 y descriptor; las poses de cada paso también
coinciden con la traza histórica y terminan en estado 0. La prueba es
`src/runtime/tests/r18/action07_royal_natural_verify_rom.py` y su informe está
en `validation/local-20261028/action07-royal-natural-rom.json`, indexado con
sus hashes en el inventario. Junto a la captura anterior del personaje 7,
hay evidencia natural para ambos personajes que aparecen en las tablas de
selección humana y CPU examinadas. Sigue sin demostrarse que no existan otras
asignaciones indirectas, ni la escena completa actual o la salida VDP1; no se
certifica el selector y la barra permanece en 0/143.

## Medición y reducción de lecturas gráficas (28-10-2026)

Se reprodujo la secuencia de 10.787 dibujos visibles procedente de una
captura HOST de combate TAG de 3.138 frames. Se mantuvieron los mismos datos
de ROM, poses y centros de pantalla con y sin caché, comparando el estado,
los registros D6/D7 y todos los registros de sprite emitidos en cada dibujo.
El resultado fue idéntico en las 10.787 comparaciones, sin poses bloqueadas.
La caché anterior hizo 45.402 llamadas al lector de metadatos, frente a
246.647 sin caché. Las consultas restantes se concentraban en cabeceras y
palabras de pose repetidas. Se probaron temporalmente tamaños de caché para
palabras de 32, 64, 128 y 256 entradas; 64 redujo la cifra a 20.376 por
384 bytes adicionales, y 128 solo ahorró 2.520 llamadas más por otros 384
bytes. Se incorporó la variante de 64 entradas al código de producción,
dentro de la caché que se invalida al cambiar la fuente ROM. Ocupa ahora
7.300 bytes, antes 6.916.

La prueba de regresión se ejecutó primero con el límite de 25.000 llamadas
sobre el código anterior y falló en 45.402. Con el cambio pasó en 20.376.
La prueba local de caché pasó con AddressSanitizer y
UndefinedBehaviorSanitizer, incluida una modificación de una palabra ROM
seguida de reinicio. La comparación gráfica directa con `$D1FC` volvió a
pasar 2.356 casos, que cubren 1.178 pares personaje/pose en ambos sentidos,
sin diferencias ni bloqueos. Evidencia:
`validation/local-20261027/metasprite-live-trace-wordcache-red.json`,
`metasprite-live-trace-wordcache-production.json`,
`action-pose-metasprite-after-wordcache.json` y los dos registros
`test-metasprite-wordcache*.log` del mismo directorio.

La métrica cuenta llamadas al lector de metadatos. El lector ya tenía una
caché de líneas de 32 bytes; por ello esta reducción no equivale a lecturas
de CD evitadas ni demuestra una ganancia concreta de FPS en la Saturn.
La traza usa el orden de poses HOST y coordenadas centradas. No se ha
generado otra BIN/CUE ni repetido aún una medición de frames en hardware.
La barra estricta permanece en 0/143.

## Selector `$00`: dos animaciones hasta la salida (28-10-2026)

El verificador de secuencias ejecutó la acción `$00` desde una entrada
construida hasta que el luchador vuelve al estado 0. Cotejó tras cada paso
los 16 KiB de RAM de juego, D0–D7 y el descriptor de animación con la ROM
arcade. Para los doce luchadores, el descriptor ordinario `$12850` recorrió
los cuadros 0, 1, 2, 3, 4 y `$FE` y salió en 39 pasos (468 en total). La
variante `$1286C`, seleccionada por los bits de rol y condición, recorrió
los mismos cuadros con los tiempos alternativos y salió en 33 pasos
(396 en total). Las poses visuales de esos cuadros son 0, `$92`, `$93`,
`$94`, `$93`; sus pares personaje/pose en ambos sentidos pertenecen a las
2.356 comparaciones gráficas directas ya pasadas con `$D1FC`.

Los informes `validation/local-20261026/action-interleaved/action00-base.json`
y `action00-alternate.json` guardan los tiempos de cada cuadro. Se renovaron
los otros 67 informes de secuencia por la ampliación del verificador y el
inventario de 143 selectores pasó de 67 a 69 informes vigentes.

La entrada del mismo selector se amplió a ocho condiciones de borde:
izquierda y derecha a ras de ring, ambas a una altura que impide corregir X,
ambos bordes de la zona de acompañantes y ambos bordes de la escena exterior.
Se fijó la velocidad X inicial a `$1234` y se comprobaron explícitamente
la corrección de X y su puesta a cero, o su conservación en el centro, tras
la primera acción. Las nueve posiciones, incluida la central, continuaron
hasta el estado 0 para los doce luchadores, con RAM y D0–D7 idénticos a la
ROM en cada paso. Junto con el descriptor alternativo hay diez informes,
120 trayectorias y 4.608 pasos de acción. El inventario incorpora ahora 77
informes vigentes. Los informes son trayectorias construidas: no intercalan
física ni planificador entre pasos, ni demuestran todas las combinaciones de
modo y contacto del resolutor, la imagen final VDP1 o una partida completa.
`$00` aún no cumple el criterio estricto de certificación; barra 0/143.

Se cruzaron después las secuencias de `$00` de dos combates HOST existentes
con la cronología del oráculo de acción. Hay 19 recorridos completos, cuatro
TAG y quince Royal: todos reproducen los 39 pasos y cambios de pose del
descriptor ordinario y terminan en estado 0. Otros nueve recorridos quedan
interrumpidos o parciales y se listan sin declararlos equivalentes. La
auditoría conserva los hashes de las trazas y de los informes ROM en
`validation/local-20261027/action00-natural-timeline-audit.json`.

También se volvió a ejecutar TAG con el código actual y se capturó el estado
HOST inmediatamente antes de la fase completa `$F4C2` del frame 1261, cuando
el luchador del hueco 4 ejecuta `$00`. La ROM original recorrió 975
instrucciones de esa fase; el port coincidió en 16 KiB de RAM de juego,
16 KiB de primer plano y D0–D7, sin dependencia bloqueada. Evidencia:
`validation/local-20261028/action00-tag-natural-1261/report.json` y
`f4c2-rom-compare.json`. Esta comprobación es de una fase natural, no de
todos los frames del combate arcade ni de Saturn física. Las variantes de
modo/colisión y la búsqueda de una aparición natural de la alternativa se
ampliaron después como se describe a continuación; barra 0/143.

La matriz de `$00` se amplió posteriormente a 96.384 estados controlados:
los doce personajes, siete valores de escena, dos lados y orientaciones,
posiciones y alturas que atraviesan límites, marcas previas de contacto,
opciones de rol, anulación del resolutor y casos con temporizador activo,
agotado o `$FF00`. La ejecución directa de la ROM y del port coincidió en
RAM, primer plano, D0–D7 y descriptor seleccionado. La herramienta identifica
51 condiciones en `$12888`, `$10B62` y `$280DC` y confirma ambos resultados
de cada una; también confirma ambos resultados de seis condiciones del
despacho y avance de animación. Una alteración deliberada de la velocidad en
una copia temporal provocó la diferencia esperada, comprobando que el
comparador detecta ese fallo. Informes y control negativo:
`validation/local-20261028/action00-resolver-matrix-rom.json` y
`action00-negative-control.log`. El inventario comprueba la vigencia de las
fuentes, captura y verificador de esta matriz. Son estados seleccionados,
no todos los valores numéricos; el modo de escena 7 queda fuera del contrato
nativo y no se ha probado su alcanzabilidad.

Desde la captura TAG del frame 1261 se cotejaron 39 fases sucesivas completas
`$F4C2`, incluidas las nueve ranuras, hasta la salida de `$00`, sin diferencias
en RAM, primer plano ni D0–D7. Se activó después la variante alternativa en
una copia de esa captura cambiando solo el bit 5 de la marca del luchador;
las 33 fases siguientes también coincidieron y llegaron al estado 0. La
marca de rol necesaria ya estaba activa en la captura original. Evidencia:
`validation/local-20261028/action00-tag-natural-1261/f4c2-sequence-39.json`
y `f4c2-alternate-sequence-33.json`. Entre fases no se ejecutaron entrada,
IA, colisiones ni árbitro. La alternativa se encontró después en una traza
histórica, como se detalla abajo; falta la imagen final VDP1 y no se certifica
`$00` todavía. La barra estricta permanece en 0/143.

Una búsqueda en 122 trazas HOST localizó una activación natural de la
alternativa en TAG, frames 4002–4034, personaje 3. La partida histórica se
bloqueó después en el frame 4189, en otro selector. Se tomó la captura
anterior al despacho del frame 4002 y se ejecutó la acción de la ROM y el
código actual durante los 33 pasos: coincidieron RAM, primer plano, D0–D7,
descriptor `$1286C`, poses, tiempo y salida al estado 0. Tras el primer
despacho se intercalaron el relevo/resolutor y física del luchador. La misma
captura, con los otros once IDs como variantes controladas, sumó 396 pasos;
los 48 pares personaje/pose están incluidos en el cotejo gráfico `$D1FC`.
Evidencia: `validation/local-20261028/action00-alternate-natural-rom.json`.
La captura original proyecta 8 KiB de RAM y 8 KiB de primer plano; el resto
empieza a cero. Otros actores y fases de frame no se intercalan, y la
activación pertenece a una versión HOST histórica: no se ha reproducido aún
de forma natural en una partida completa con el código actual ni en Saturn.
El inventario comprueba la captura, traza, gráficos y fuentes asociadas.

## Selector `$07`: duración y disponibilidad por luchador (28-10-2026)

Se cotejó una captura TAG histórica del personaje 7 desde su primer despacho
de `$07` hasta el regreso al estado 0. El código actual y la ROM recorrieron
los mismos 42 pasos y poses observados en aquella traza. Con los otros once
IDs como variantes controladas, la acción pasó en 38 pasos por personaje;
en total se compararon 460 pasos, intercalando el resolutor y la física del
actor después del primer despacho. Se compararon RAM, primer plano, D0–D7 y
descriptor en cada fase. El personaje 7 usa `$130E6` y el resto `$130FE`,
con una duración más corta para la segunda pose.

La ROM solo define las tres poses gráficas `$BF/$C0/$C1` para los personajes
7 y 8. Las tablas de elección humana comprobadas seleccionan `$07` cuatro
veces, únicamente para ellos; las dos familias de tablas CPU contienen seis
entradas, también solo para esos personajes. Las otras diez ejecuciones son
pruebas controladas de la rutina sin sprites originales para esas poses; no
demuestran que sean combinaciones alcanzables en una partida. Asignaciones
indirectas ajenas a esas tablas quedan por investigar. El informe
`validation/local-20261028/action07-natural-continuation-rom.json` enlaza
la captura, traza, fuentes, opciones y cotejo gráfico. La partida histórica
se bloqueó después en el frame 5026. Una partida HOST con el código actual
y perfil básico avanzó hasta el frame 1245, pero no pasó por `$07` durante
esa ventana. Sin escena completa actual ni salida VDP1 cotejada, `$07` no
se suma a los selectores certificados; barra 0/143.

El numerador exige ramas alcanzables cotejadas con la ROM, animaciones y
cuadros usados por los doce luchadores, y una ruta de combate o escena sin
bloqueo. Las pruebas de secuencia que siguen son avance verificable, pero
ningún selector cumple aún todos esos requisitos. No se usa el cociente de
IDs conectados ni de informes de prueba como porcentaje de terminación.

Una comparación nueva ejecuta la fase completa de los nueve huecos de
luchador `$F4C2` desde capturas sin alterar de combates HOST. La fase TAG
en los frames 1000 y 3000, y la fase Royal en el frame 1000, terminaron
sin diferencias respecto a la ROM en los 16 KiB de RAM de juego, 16 KiB
de primer plano, listas de dibujo y registros D0–D7. El primer intento
del TAG 3000 reveló una omisión real: al activar la escena exterior, la
ROM llama `$F8D8` tras cada luchador elegible. El port no colocaba el
marcador fijo ni cambiaba la lista de dibujo del luchador. Se implementó
esa rutina y se cotejaron sus tres rutas de posición, incluidos los
límites X `$28F/$290/$3AF/$3B0`, sin diferencias. La prueba local del
iterador `$F4C2` y un combate TAG completo de 3138 frames también pasan.
Evidencia: `validation/local-20261027/pre-f4c2-tag-pressure-1000/`,
`pre-f4c2-tag-pressure-3000/`, `pre-f4c2-royal-pressure-1000/` y
`tag-pressure-after-f8d8/`. Estas son capturas de HOST; no prueban aún
una trayectoria idéntica ni la fluidez de la Saturn física. La barra
estricta sigue en 0/143.

La ampliación de esa prueba a seis capturas TAG (frames 1000, 1500, 2000,
2500, 3000 y 3100) y cuatro Royal (500, 1000, 2000 y 2200) descubrió
otro fallo real en la fase Royal 2000: el estado principal `$0C` volvía
de la pose alternativa con índice `$FE`, pero el port calculaba el
siguiente índice como `(FE+1)&3=3`; el descriptor de tipo 2 de la ROM
lo reinicia a cero. Una variante controlada del estado `$0B` mostró la
misma diferencia. Se corrigieron ambos ejecutores de animación;
las diez fases completas coinciden ahora en RAM, primer plano, cola y
D0–D7. La captura Royal 2200 procede de una trayectoria HOST nueva tras
la corrección, y un combate Royal de 3500 frames siguió sin bloqueo.
Evidencia: los informes `f4c2-after-kind2.json` de las ocho capturas
anteriores, `pre-f4c2-royal-pressure-2000/f4c2-natural-after-fix.json`,
`f4c2-state11-after-fix.json` y
`pre-f4c2-royal-pressure-2200-after-kind2/f4c2-rom-compare.json` bajo
`validation/local-20261027/`. El test local `$F4C2` pasó de nuevo.
Esto añade cobertura de estados principales y del planificador;
todavía no certifica individualmente ninguno de los 143 selectores.

Se añadió una continuación controlada de `$F4C2` que, a partir de cuatro
capturas HOST, repite fases completas con las cabeceras de las listas de
dibujo reiniciadas entre llamadas. No ejecuta entrada, IA, colisión ni
árbitro entre fases. Las cuatro secuencias pasaron 768 fases y 722.848
instrucciones ROM cotejando los 16 KiB de RAM de juego, 16 KiB de primer
plano y D0–D7 tras cada fase. La Royal iniciada en el frame 2000 recorre
el relevo del estado principal `$0C` a `$FF` en el paso 204 y entrega al
actor enlazado el estado `$0C`; la secuencia sigue hasta 256 pasos sin desvío.
Los informes incluyen hash de la ROM, captura, verificador y las 256
fuentes/archivos congelados. Evidencia: `f4c2-sequence-256.json` de
`pre-f4c2-royal-pressure-2000/` y `pre-f4c2-tag-pressure-3000/`, y
`f4c2-sequence-128.json` de `pre-f4c2-tag-pressure-2500/` y
`pre-f4c2-royal-pressure-2200-after-kind2/`, todos bajo
`validation/local-20261027/`. Esta cobertura de planificador no equivale
a 768 frames arcade completos ni cambia la barra 0/143.

La auditoría directa de tablas de animación compara con la ROM los 180
descriptores generados, sus 519 pares de pose/duración y los 199 punteros de
los tres despachadores (13 de estado principal, 143 de acción y 43 de estado
4). Pasó sin discrepancias. De los 180 descriptores, 20 entregan el control a
una rutina propia; 18 IDs de acción usan uno de ellos y necesitan inspección
de sus ramas ejecutables. El informe `action-animation-table-audit.json`
incluye la secuencia de poses y tiempos de cada ID. Este cotejo verifica los
datos, no la ejecución de todas las ramas, la imagen final de los sprites ni
la fluidez en Saturn. El numerador de cierre sigue en 0/143.

La traducción de acciones guarda además 169 descriptores literales en C;
35 no aparecen en la tabla generada; algunas son alternativas elegidas durante
la ejecución. Se compararon sus direcciones, tipos, cantidades, tiempos y
poses con la ROM: 169/169 coinciden. Informe:
`action-runtime-descriptor-audit.json`. Un mapa conservador de llamadas une
esas alternativas con los selectores que podrían usarlas. En los personajes
que figuran en la matriz de elecciones, sus 203 referencias de pose existen
en la ROM; aportan 60 pares personaje/pose distintos que no estaban en la
tabla base. Todavía hace falta demostrar que cada rama elige el descriptor
correcto en combate.

El enlace entre acciones y recursos gráficos ya se cotejó para las elecciones
de personaje generadas: 1.201 referencias de pose, de las que 1.200 apuntan
a una definición de sprite ROM no vacía. La excepción es `$2F` con el set
gráfico 5: la pose `$020F` del descriptor inicial está marcada como ausente
(`$FFFE`) en la propia ROM. Su descriptor alternativo `$16C7E` contiene las
seis poses válidas de ese set. Se volvió a ejecutar el verificador de `$2F`:
5.548 casos y 40 puntos de rama cotejados con la ROM sin discrepancias.
Informes: `action-pose-link-audit.json` y `action2f-recheck.json`.

Una captura real del lanzamiento `$2F` (personaje 1, fotograma 3016)
continuó con resolución, acción y física intercaladas hasta el reposo en
162 pasos. La variante controlada del personaje 5, el otro que figura para
`$2F` en la matriz de elecciones, completó 125 pasos y utilizó el
descriptor alternativo válido. Los 14 pares visuales de estos dos personajes
se cotejaron con el constructor gráfico de la ROM en ambos sentidos.
También se ejecutaron variantes controladas de los otros diez ID: los doce
sumaron 1.537 pasos sin diferencias de RAM de 16 KiB, primer plano, D0-D7 ni
descriptor. Esas diez variantes no figuran en la matriz de elección de `$2F`;
26 de sus pares de pose no existen en los sets gráficos de la propia ROM.
No se consideran cobertura gráfica ni prueba de que tales combinaciones sean
alcanzables dinámicamente. Informes:
`action2f-interleaved-pose-discovery.json` y
`action2f-interleaved-all-characters.json`. Faltan los demás actores,
la selección dinámica completa y una partida TAG continua.
Se cotejó además la matriz humana completa de la ROM (252 filas) y las
144 tablas normales de elección CPU usadas por `$1DEA6`: `$2F` aparece
cinco veces en la matriz humana, solo para los personajes 1 y 5, y ninguna
vez en esas tablas CPU. Esto acota las diez variantes sin gráfico; aún no
descarta asignaciones indirectas desde otras rutinas. Informe:
`action2f-selection-sources.json`.

La captura real de `$2E` (personaje 1, fotograma 5918) continuó con
resolución, acción y física intercaladas hasta el reposo en 34 pasos. Las
variantes controladas de los doce ID también terminaron tras 34 pasos cada
una: 408 en total, sin diferencias en RAM de 16 KiB, primer plano, D0-D7
ni descriptor. Solo los personajes 1, 5 y 9 figuran para `$2E` en la
matriz de elecciones. Sus seis pares visuales coincidieron con el
constructor gráfico ROM en ambos sentidos; nueve pares de pose de las
variantes ajenas a la matriz carecen de definición en la propia ROM.
La matriz humana de 252 filas contiene seis elecciones `$2E` para esos
tres personajes. Las 144 tablas normales CPU contienen once, todas también
para 1, 5 y 9. El verificador aislado de `$2E` pasó además 3.874 casos,
34 pasos naturales de acción y 980 pasos derivados con contacto inyectado.
El fallo de limpieza de su DLL en Windows quedó cerrado y la prueba terminó
con código de salida cero. Informes: `action2e-recheck.json`,
`action2e-interleaved-pose-discovery.json`,
`action2e-interleaved-all-characters.json` y
`action2e-selection-sources.json`. Las asignaciones indirectas y el
combate completo siguen pendientes.

La captura real de `$2D` (personaje 6, fotograma 3614) continuó con
resolución, acción y física intercaladas hasta el reposo en 23 pasos.
Las doce variantes controladas sumaron 276 pasos sin diferencias en RAM
de 16 KiB, primer plano, D0-D7 ni descriptor. La matriz humana admite
`$2D` para los personajes 0, 5, 6, 10 y 11; sus diez pares visuales
observados se cotejaron con el constructor gráfico de la ROM en ambos
sentidos. Siete pares de pose de los otros personajes no existen en la
ROM. La matriz humana de 252 filas contiene 15 elecciones `$2D` para
los cinco personajes y las 144 tablas normales CPU contienen 14, también
restringidas a esos cinco. La prueba de ramas aisladas pasó 4.521 casos
frente a la ROM, incluidos límites de velocidad y contacto con cuerdas.
Su guardia de prueba se actualizó porque `$35` ya está implementado;
la ejecución terminó sin error. Informes:
`action2d-recheck.json`, `action2d-interleaved-pose-discovery.json`,
`action2d-interleaved-all-characters.json` y
`action2d-selection-sources.json`. Faltan los otros actores,
las asignaciones indirectas y el combate completo.

`$79` no figura en la matriz de elección directa: la ROM escoge un descriptor
distinto para cada uno de los doce sets mediante la tabla `$1A30C`. Se
cotejaron 7.611 estados del ejecutor, sus 20 puntos de rama, los descriptores
devueltos y 37 pasos de continuación de una captura real, sin diferencias.
Sus 36 fotogramas contienen 31 pares distintos personaje/pose, todos con
definición ROM válida; 30 no estaban en el inventario anterior. Informe:
`action79-recheck.json`. Desde la misma captura real se continuó después
`$79` con resolución, acción y física intercaladas para los doce ID de
luchador: 822 pasos en total (40–112 por personaje), sin diferencias en
RAM de 16 KiB, primer plano, D0–D7 ni descriptor tras cada acción. Los
54 pares personaje/pose efectivamente vistos están cubiertos por los cotejos
gráficos directos de acciones y de salida Royal, incluidos los once casos
de pose de entrada `$0068` que faltaban en el inventario gráfico de acciones.
La captura natural conserva el ID 1; los otros once son variantes controladas.
Evidencia: `action79-interleaved-all-characters.json`. Los demás actores y
el planificador de un fotograma completo aún no se ejecutan en esta prueba.

`$1E` y `$1F` comparten callback de liberación de objeto y cambian entre
dos descriptores según el estado del objeto. El verificador se amplió a los
doce luchadores, ambos descriptores, tres fotogramas y ambos sentidos; pasó
8.552 cotejos y 18 guardias. Dos capturas reales de presión TAG, una por cada
selector, se continuaron durante 29 pasos de acción cada una hasta el reposo,
comparando RAM, primer plano, registros y descriptor tras cada paso. Se corrigió además un guardia de la
prueba que seguía tratando `$35` como no implementado. Las seis poses por personaje
existen en la ROM (72 pares); 36 pares son nuevos para el inventario.
Informe: `action1f-all-character-recheck.json`. Después se intercalaron
resolución, acción y física en ambas capturas y en once variantes controladas
de personaje por captura: 348 pasos por selector, 696 en total. Cada ruta
vio 60 pares personaje/pose, todos comparados directamente con el constructor
gráfico de la ROM, incluidas las poses de entrada `$7C` y `$7F`. La captura
real conserva el personaje original; las variantes cambian solo su ID.
Informes: `action1e-interleaved-all-characters.json` y
`action1f-interleaved-all-characters.json`. Esto no equivale todavía a
una secuencia TAG completa.

Estas secuencias descubrieron un error real en el resolutor `$2851E`:
el port asignaba contacto central 4 con Y `$140` o menor, mientras la
ROM solo lo hace en la rama Y > `$140`. Se corrigió la condición y se
cotejaron 632 casos de ayudantes compartidos frente a la ROM, incluidos
los bordes Y `$13F/$140/$141` y X `$293/$294/$38B/$38C`, sin
diferencias. La prueba local del iterador `$F4C2` pasó de nuevo.
La causa y su corrección están en `wf_r17_resolver_280dc.c`;
informe: `action19-shared-helpers-direct-rom.json`.

`$6B` se cotejó en 5.766 estados, con entrada,
desplazamiento y llegada al destino. Coincidieron RAM, primer plano, registros
y descriptor en los cuatro caminos gráficos posibles para los doce luchadores;
2.880 estados llegaron al reposo. Se añadieron seis capturas originales del
modo demostración con los actores 0, 1 y 4, todas coincidentes. Las posiciones
de la matriz se construyeron para recorrer las ramas. Tres capturas naturales
se continuaron con resolución, acción y física intercaladas hasta el reposo:
36, 36 y 40 pasos, sin diferencias de RAM, primer plano, D0-D7 ni descriptor.
Informes: `action6b-ring-walk-matrix.json` y los tres
`action6b-interleaved-natural-*.json`; fuentes y hashes:
`src/runtime/tests/r18/action6b_manifest.json`.

`$68`, la salida del ring, pasó 8.071 cotejos con la ROM, incluidas siete
capturas originales del modo demostración sin entrada: fotogramas 803, 840,
900, 960, 1020, 1080 y 1140 del actor 4. Las capturas cubren las fases 0,
1, 2 y 3, aunque son muestras separadas, no todos los pasos intermedios.
La matriz sintética recorre los doce luchadores, los cinco descriptores
posibles y las rutas que permanecen en `$68` o pasan a `$69`, `$6A` y `$7A`.
RAM, primer plano, D0-D7 y descriptor coincidieron en cada caso. Desde la
captura natural del fotograma 803 se continuó después el mismo estado
proyectado con resolución, acción y física intercaladas: 445 pasos sin
diferencias, fases 0→1→2→3, relevo a `$69`, dos comandos de transferencia
originales (1 y 2) y reposo. La captura independiente del fotograma 960
repitió la parte final en 288 pasos. Ambas rutas comprueban RAM de 16 KiB,
primer plano, D0-D7 y descriptor tras cada acción. No ejecutan al resto de
actores ni al consumidor de la cola de transferencia que cambia la escena.
La continuación alternativa hasta `$6A` y `$7A`, y la prolongación controlada
de `$6A` hasta el reposo exterior, se describen a continuación.
Informes: `action68-ring-exit-matrix.json`,
`action68-interleaved-natural-803-through69.json` y
`action68-interleaved-natural-960-through69.json`; las fuentes y hashes de
las siete capturas constan en `src/runtime/tests/r18/action68_manifest.json`.

Se ampliaron las continuaciones de la captura natural 803 mediante variantes
controladas del ID de personaje para los doce sets. La ruta TAG
`$68`→`$69`→reposo pasó 5.481 pasos de resolución, acción y física en total
(429–514 por personaje). Al cambiar el bit de modo, la bifurcación Royal
`$68`→`$7A` coincidió durante 98 pasos por personaje (1.176 en total).
Al preparar las marcas de rescate TAG, `$68`→`$6A` coincidió durante 15
pasos por personaje (180 en total). Al prolongar esta última ruta, los doce
sets alcanzaron `$6B` y el reposo en 124–129 pasos cada uno (1.511 en total).
Para esa prolongación se reprodujo en el momento del relevo el modo de escena
exterior observado en otra captura natural, la del fotograma 1343: los bytes
de modo pasan de 0 a 2. Los hashes de ambas capturas constan en los informes.
Sin dicho cambio externo, ROM y port quedan por igual en `$6B`/estado 5:
el estancamiento del ensayo aislado no identifica un fallo diferencial del
ejecutor de acciones. Ese ensayo de cambio de modo no ejecuta al gestor real
de escenas ni al resto de luchadores; la prueba posterior del gestor se
describe más abajo. Tampoco prolonga `$7A` hasta una escena Royal completa.
Estos 48 informes construidos no son 48 capturas naturales ni autorizan
certificar `$68`, `$69`, `$6A`, `$6B` o `$7A`. Evidencia:
`action68-interleaved-natural-803-through69-character*.json`,
`action68-interleaved-natural-803-royal-character*.json`,
`action68-interleaved-natural-803-rescue-character*.json` y
`action68-interleaved-natural-803-rescue-through6b-outside-scene-character*.json`.

La retirada Royal `$7A` se continuó ahora desde una captura original del
fotograma 2276, actor 0. Atravesó los subestados 0→1→2→3→4→5 y borró el
actor tras 233 pasos de resolución, acción y física, comparados después de
cada etapa con la ROM. Variar solo el ID del luchador produjo 2.807 pasos
coincidentes en los doce sets (224–254 por set). En otros doce recorridos se
preparó una asignación de paleta en RAM; la rutina de liberación devolvió
a cero tanto el registro del personaje como el banco ocupado, también con
2.807 pasos coincidentes. En total se regeneraron los 80 informes de
secuencia `$68`/`$6B`/`$7A` con el verificador y el puente actuales; todos
conservan los hashes actuales y pasaron. La captura auténtica es solo la
del personaje 0 sin asignación; las demás son variantes controladas.
El puente de secuencia reproduce la liberación de paleta de `$2B58` sin
ejecutar el gestor de recursos completo. Por separado, el callback de paleta
real del port pasó 3.840 comparaciones con el `$2B58` intacto de la ROM,
incluidos los doce ID de luchador y los 256 valores de asignación; informe
`action7a-real-palette-recheck.json`. Ninguna de esas pruebas ejecuta el
resto de la partida, por lo que
`$7A` sigue sin certificarse. Evidencia: `action7a-interleaved-natural-2276-slot0.json`,
`action7a-interleaved-natural-2276-slot0-character*.json` y
`action7a-interleaved-natural-2276-slot0-character*-palalloc.json`;
procedencia en `src/runtime/tests/r18/action7a_sequence_manifest.json`.

Se extrajeron las poses visuales de cada paso de `$7A` y se cotejaron con
los tres descriptores ROM de la ruta, la pose explícita `$01D3`, la pose de
entrada capturada y la neutral: exactamente diez poses. Los doce recorridos
de personaje usaron ese mismo conjunto. El constructor gráfico nativo
coincidió con `$D1FC` para las diez poses, los doce sets y ambos sentidos:
240 casos sin bloqueo. La caché existente ocupó 6.916 bytes y redujo las
lecturas ROM de 5.220 en frío a 690 en caliente con la misma salida.
Es una medida de lecturas en host; todavía no acredita FPS ni composición
final VDP1 en Saturn. Evidencia: `royal-exit-poses-direct-rom.json` y
`visual_poses` en los informes `action7a-interleaved-natural-2276-slot0-character*.json`.

El puente de pruebas se amplió para la liberación de paleta en `$7A`. Como
su hash cambió, se volvieron a ejecutar las 67 pruebas intercaladas anteriores
de acciones; todas pasaron con el puente actual. El inventario regenerado
indexa por separado esas 67 pruebas y los 93 informes de ring/escena para
los selectores `$68`, `$69`, `$6A`, `$6B` y `$7A`. Son archivos de evidencia,
con variantes controladas y solapamientos; su suma no es un porcentaje de
implementación ni altera la barra estricta de 0/143. Evidencia:
`action-selector-inventory.json` y `action-interleaved/*.json`.

Se encadenó además el rescate TAG con el gestor de escena real. La variante
del personaje 2 nacida de la captura 803 llegó a `$6A` en 15 pasos y guardó
su RAM, primer plano y registros como estado interno de prueba. Desde ese
estado, el port ejecutó `$F98C` y sus cargas de mapa/paleta; la ROM original
ejecutó la misma ruta. El verificador pasó 30 escenarios, incluido este
relevo y una captura natural independiente, además de cinco casos que deben
bloquear un recurso de escena ausente o inválido. En el relevo encadenado
coincidieron byte a byte los 16 KiB de RAM y los 16 KiB de primer plano,
todos los registros D0–D7 y las superficies comparadas del mapa y paletas.
El estado resultante se continuó con resolución, acción y física intercaladas:
`$6A`→`$6B`→reposo en 132 pasos para el personaje 2. Once sustituciones
adicionales de personaje, junto con el 2 original, sumaron 1.601 pasos
coincidentes (129–142 por set). Los 93 informes de secuencia actuales
conservan los hashes del verificador y puente actuales y pasan.
Esto verifica la unión aislada acción→gestor de escena→acción, pero no
ejecuta los demás actores durante los pasos anteriores y posteriores ni
mide una partida completa en Saturn. Evidencia:
`ringside-transition-current-rom.json`,
`action6a-interleaved-postscene-803-slot4.json`,
`action6a-interleaved-postscene-803-slot4-character*.json` y los dos estados
internos `action68-rescue-*.state`.

Los seis descriptores ROM utilizados por `$68`/`$69`/`$6B` contienen 26 poses
distintas. Se comparó la construcción gráfica de cada una para los doce sets
y ambos sentidos: 624 casos frente a `$D1FC`, sin diferencias ni bloqueos.
Informe: `ring-route-poses-direct-rom.json`. Sigue faltando la imagen final
en VDP1 y el rendimiento de estas secuencias en Saturn real.

De la tabla base, las alternativas, `$79` y `$1E`/`$1F`, junto con
las poses de entrada de las capturas `$1E`/`$1F`/`$2D`/`$2F`, salen 1.178 pares distintos
personaje/pose disponibles.
El constructor nativo de sprites se cotejó con la rutina ROM `$D1FC` para
ambos sentidos de cada par: 2.356 casos, ningún bloqueo ni diferencia en
registros, cantidad de registros o campos usados. Informe:
`action-pose-metasprite-verify.json`. Esto no mide FPS ni representa todas
las parejas enlazadas y ramas que el callback puede escoger dinámicamente.

La medición actual de 2.356 construcciones arroja 84.247 lecturas ROM en
frío y 20.020 en caliente con la caché de 6.916 bytes. Es una muestra
ampliada; las cifras históricas que siguen corresponden a 2.278 casos y
no se comparan directamente con esta muestra.

La medición de esas 2.278 construcciones señaló lecturas repetidas de la
tabla de sets gráficos (`$38F14` y `$38FB8`). Una caché directa para los doce
luchadores añade 96 bytes a la caché existente (6.628 → 6.724 bytes) y evita
leer de nuevo el desplazamiento y la base del set. Con las mismas poses y
semilla de prueba, las lecturas ROM en caliente bajaron de 30.036 a 23.673;
en frío, de 81.639 a 81.506. Una segunda mejora reutiliza la cabecera ya
leída al despachar composiciones y hojas simples: las lecturas bajaron de
23.673 a 21.976 en caliente y de 81.506 a 79.809 en frío.
Una tercera caché directa de 32 offsets de pose añade 192 bytes (6.724 →
6.916 bytes) y redujo las lecturas en caliente de 21.976 a 18.860 en las
mismas 2.278 construcciones. En las 21.696 construcciones enlazadas del
centro bajaron de 309.434 a 267.306; las lecturas en frío no cambiaron.
Pasaron de nuevo la prueba local de caché, las 2.278
comparaciones gráficas directas con la ROM y 21.696 combinaciones de sprites
enlazados sin bloqueo. Es una reducción de operaciones medida en host, no
una medición de FPS en Saturn.
El cotejo de sprites enlazados pasó después de la prueba de bloqueo a una
comparación gráfica directa con la ROM: las 904 poses enlazadas de los doce
sets, con los doce posibles compañeros y ambos sentidos, sumaron 21.696
ejecuciones por posición. Se probaron centro, esquina superior izquierda,
límite inferior derecho y desbordamiento de coordenadas: 86.784 ejecuciones
en total. Coincidieron los registros de salida, la cantidad de sprites y
todos los campos consumidos por el presentador; ninguna se bloqueó. Esto no
prueba el resto de posiciones, la imagen final en VDP1 ni la fluidez física.
Informes: `paired-metasprite-direct-rom.json`,
`paired-metasprite-direct-rom-upper-left.json`,
`paired-metasprite-direct-rom-lower-right.json` y
`paired-metasprite-direct-rom-overflow.json`.
La versión con las tres mejoras compiló también como objeto SH-2 con GCC 13.2.0 en la máquina
Linux temporal, en modo independiente de biblioteca y con advertencias como
errores: 3.900 bytes de texto, sin datos ni BSS en ese objeto. La máquina se
apagó tras la prueba y su disco se usó en modo instantánea, sin persistir
cambios.
Informe: `metasprite-sh2-object-compile.json`. No se ha enlazado el
juego ni generado una nueva BIN/CUE, de acuerdo
con la petición actual.

Próximo trabajo: inventariar las poses elegidas dinámicamente por callbacks
y las bifurcaciones restantes por selector y personaje, ampliar las
trayectorias que no tienen relevo continuo,
y perfilar el coste del compositor/espera VDP1 durante combates largos antes
de cambiar más rutinas gráficas. Mantener la música nativa aplazada y no
preparar una nueva BIN/CUE mientras siga esta tarea.

## Secuencias intercaladas y trabajo de sprites (26-10-2026)

Se amplió el oráculo de `$11`, `$14` y `$35` para intercalar el relevo de estado
del planificador, el resolutor de límites `$280DC`, la acción `$1C03E` y la
física `$2208`. Cada paso compara con la ROM original 16 KiB de RAM de trabajo,
D0-D7 y el descriptor elegido por la acción. Las tres rutas se ejecutaron para
los doce personajes, con estados de ring válidos y un rival derribado/enlazado
en `$35`:

- `$11`: 831 pasos acumulados; doce transiciones al estado 4.
- `$14`: 692 pasos acumulados; doce transiciones al estado 4.
- `$35`: los cuatro valores del byte de modo (0, 1, 2 y 3) sumaron 6.290
  pasos; en cada valor los doce llegaron a la rama de impacto y salieron al
  estado 7. Las matrices de 5.760 variantes aisladas de `$35` pasaron tanto
  en modo 0 como en modo 2; las de `$11` y `$14` pasaron 4.608 variantes cada una.

Los informes reproducibles están en `action-interleaved/action11.json`,
`action14.json` y `action35-mode0.json` a `action35-mode3.json`. Son
trayectorias construidas de estos cuatro
dueños del motor, no partidas completas ni verificación de todos los contactos
posibles, imágenes o sonido. El hallazgo relevante fue que repetir sólo la
acción dejaba `$35` atascada en la fase 0/2; la transición depende del relevo
de estado y de los límites del ring. La prueba ahora falla si falta el impacto
de cualquiera de los doce luchadores. Otras dos rutas por acción precisan el
contacto y la salida: con rival derribado, `$11` cambia su subestado de 8 a 6
y el actor sale al estado 7 (1.131 pasos); `$14` lo cambia de 8 a 7 y sale al
estado 7 (812 pasos). Con el rival activo en `$61`, `$11` le asigna la reacción
`$77` antes de aterrizar en estado 4 (1.552 pasos), mientras `$14` aterriza sin
alcanzarlo y el rival permanece en `$61` (692 pasos). Cada ruta pasó con los
doce personajes; son condiciones iniciales construidas, no todos los ángulos,
distancias o estados del rival. Informes: `action11-downed-recovery.json`,
`action11-active-contact.json`, `action14-downed-recovery.json` y
`action14-active-miss.json` en la misma carpeta.

Para `$35` se añadieron seis rutas completas de fallo. Con rival activo en
`$61` y posición interior `$2C0`, los doce luchadores aterrizan en estado 4
sin alterar la acción del rival en cada uno de los cuatro modos; suman 5.426
pasos. Con rival derribado en el borde interior `$310`, los modos 2 y 3
aterrizan sin impacto para los doce; suman 1.514 pasos. La ROM y el port
coincidieron en RAM de trabajo, registros y descriptor en cada paso. Se
descartó una prueba anterior con `$340` porque ese X queda fuera del límite
del ring de este escenario. Los informes válidos son
`action35-active-mode0.json` a `action35-active-mode3.json` y
`action35-rope-mode2.json`/`action35-rope-mode3.json`. Las posiciones,
reacciones y posibles caídas posteriores todavía necesitan cobertura.

También se completó la secuencia de interludio Royal `$8E`: el relevo de
estado, el resolutor y la acción se intercalaron durante 653 pasos para cada
uno de los doce personajes, 7.836 comparaciones en total. En los doce casos
las fases cambiaron en los pasos 70, 140 y 141; al final el siguiente actor
recibió `$79` y se alzó la bandera `0x0100`. Se compararon RAM, registros,
plano de primer término, paleta y descriptor con la ROM en cada paso. Informe:
`action-interleaved/action8e.json`. Es un interludio construido y no un combate
Royal completo ni una medida de rendimiento.

El lanzamiento enlazado `$44` tiene ahora dos trayectorias completas para
los doce luchadores. Los doce aciertos alcanzan la caída del rival y salen
al estado 0 en 94 pasos (1.128 en total); la pose alternativa `$1865C` aparece
cuando el rival tiene ID 8. Para cada luchador se eligió con la ROM original
un estado inicial que falla la tirada. Los doce fallos pasaron de `$44` a `$82`
y luego a `$7B` en el paso 81 (972 pasos en total). La prueba reproduce el
relevo del motor; sin él, `$82` permanecía en un ciclo de espera y una
repetición aislada de `$1C03E` habría dado una conclusión equivocada.
Informes: `action-interleaved/action44-success.json` y `action44-miss.json`.

La ampliación por índice de lanzamiento cubre ahora los cinco valores de
`+$F2`: 60 trayectorias de acierto (94 pasos cada una) y 48 de fallo (81 pasos
cada una), siempre para los doce personajes. El índice 0 no admite fallo en
esta condición: la tabla ROM exige 16 y el resultado comparado de la tirada
ocupa cuatro bits (0..15). En los índices 1..4 se fuerza un estado válido del
rival que permite fallar y se busca la semilla consultando la ROM original;
el acierto usa la marca original de éxito garantizado del atacante. Los
informes añadidos son `action44-success-index1.json` a `index4.json` y
`action44-miss-index1.json` a `index3.json`; los dos informes anteriores
cubren acierto 0 y fallo 4. Cada fallo atraviesa `$82`→`$7B`. La recuperación
posterior del rival no forma parte de estas trayectorias.

La acción aérea `$13` tiene ahora dos secuencias con física y límites del ring
intercalados. Sin rival derribado, los doce llegaron a su caída en estado 4;
con el rival enlazado en subestado 8, los doce alcanzaron el impacto que lo
pasa al subestado 7. Cada variante sumó 593 pasos. Una tercera continúa desde
el golpe hasta que el atacante sale al estado 7 (893 pasos acumulados). La
recuperación posterior de la víctima y otras posiciones iniciales siguen
pendientes. Informes: `action-interleaved/action13-fall.json`,
`action13-impact.json` y `action13-recovery.json`.

El salto de varias fases `$12` pasó tres trayectorias de 1.113 pasos cada una
para los doce personajes: caída sin contacto, golpe al rival derribado (su
subestado cambia de 8 a 6) y contacto con rival activo en `$61` (su subestado
pasa a `$77`). Los rivales se situaron en la zona de aterrizaje con estados
válidos antes del salto. Otras dos continúan cada contacto hasta que el
atacante sale al estado 7: 1.413 pasos acumulados por ruta. La recuperación
posterior de la víctima sigue pendiente. Informes:
`action-interleaved/action12-fall.json`, `action12-impact.json`,
`action12-active.json`, `action12-downed-recovery.json` y
`action12-active-recovery.json`.

Se continuó cada impacto contra rival derribado hasta que ese rival vuelve
al estado 0, intercalando relevo de estado, resolución del ring, acción y
física en la ROM y en el port. Se cotejaron los 16 KiB de RAM de trabajo y
D0-D7 tras cada dueño. Pasaron los doce personajes para `$11`, `$12`, `$13`,
`$14` y los cuatro modos de `$35`; los ocho informes adicionales
`action11-victim-recovery.json`, `action12-victim-recovery.json`,
`action13-victim-recovery.json`, `action14-victim-recovery.json` y
`action35-victim-mode0.json` a `mode3.json` reúnen 9.384 pasos del rival.
La marca de ocupación del atacante se inicializa en reposo: dejarla aleatoria
en el escenario artificial bloqueaba indefinidamente la recuperación de
algunos personajes. No se extrapola esta ruta construida a todos los agarres,
ángulos ni combates; las imágenes de animación siguen sin compararse aquí.

Las acciones de salto `$0C` y `$10` pasaron cuatro trayectorias cada una:
golpe sobre los subestados 8 y 9 del rival derribado, contacto con el rival
activo en `$61` (reacción `$77`) y caída sin impacto. `$0D` pasó sus rutas
0, 1 y 2, que dejan al rival en los subestados `$1C`, 3 y `$20`, además de
un recorrido sin golpe sobre un rival ya derribado. La carrera `$0E` pasó
sus cuatro fases y tres salidas: golpe que entrega `$51` al rival, fallo y
reacción especial contra `$54`. Cada variante terminó para los doce
luchadores con física y límites del ring intercalados. El escenario de fallo
usa coordenadas alcanzables: situar el rival artificial fuera del ring
impedía completar la fase de persecución. Estos quince informes añaden
16.935 pasos del atacante; la continuidad de `$51` y `$77` se verifica en
los informes siguientes. Informes: `action0c-*.json`, `action0d-*.json`,
`action0e-*.json` y `action10-*.json`.

Siete informes posteriores siguen el rival de esas acciones: tras `$0C` y
`$10` contra los dos subestados de derribo llega al reposo; `$0E`→`$51`
también acaba en reposo; los contragolpes `$0C`/`$10`→`$77` enlazan de
vuelta a `$61`/`$37`. Se cotejaron otras 7.354 llamadas de acción del
atacante y 6.804 del rival, incluidas física y colisión, para los doce
luchadores por ruta. Los informes son `action0c-victim-ground8.json`,
`action0c-victim-ground9.json`, `action10-victim-ground8.json`,
`action10-victim-ground9.json`, `action0e-victim-hit.json`,
`action0c-counter-return.json` y `action10-counter-return.json`.

Las tres rutas con impacto de `$0D` continúan igualmente hasta que el rival
vuelve al estado 0: `$1C` (ruta 0), 3 (ruta 1) y `$20` (ruta 2). Se
verificaron los doce personajes por ruta, 2.817 pasos del atacante y 5.436
del rival adicionales. La ruta 0 conserva el bit de selección que el oráculo
necesita antes del salto; limpiarlo por completo la convertía falsamente en
ruta 2. Informes: `action0d-victim-route0.json` a `route2.json`.

`action-selector-inventory.json` toma los 143 punteros de la tabla de la ROM
original (SHA-1 `a726c46f350723ea73158bc78055d17d418b14e2`), los cruza con
las elecciones generadas para los doce personajes y enlaza los verificadores
existentes. Hay un verificador con nombre para los 143 IDs (72 en la familia,
76 individuales, con solapamiento). Esto acredita que cada ID tiene una
entrada de prueba, no que se hayan cerrado sus ramas. El inventario registra
por separado 67 informes de secuencia (82.193 pasos del atacante y 21.624
del rival). El inventario exige que todos los
informes correspondan a los hashes actuales de ROM, verificador y puente,
y que los pasos sumen sus resultados por personaje.

En el compositor Saturn, la tabla de 512 índices de tile se borraba entera
cada fotograma. Se empaquetó una época de siete bits en las mismas entradas,
con borrado únicamente al iniciar y al reciclar la época. Una prueba de 260
fotogramas con colisiones de índice y cambio de identidades midió 2.048 bytes
borrados frente a 266.240 del camino anterior y conservó los slots y comandos
esperados. No se le atribuye una mejora de FPS hasta medirla en emulador y
Saturn. No cambió la carga de audio ni se preparó BIN/CUE nueva.

La segunda tabla de 512 entradas, usada para reubicar tiles que ya están en
VRAM, tenía el mismo borrado por fotograma. Ahora recicla su marca de época
sin otra tabla. La prueba de 260 fotogramas cubrió colisiones de hash, un tile
residente duplicado y las dos vueltas de la época: 2.048 bytes borrados en vez
de 266.240. `test_saturn_render_bridge` volvió a pasar. Sumadas ambas tablas,
los borrados medidos bajan de 532.480 a 4.096 bytes en esas 260 llamadas,
sin que esa cifra equivalga a una medida de FPS, carga de CD o uso de VRAM.
El lector ROM volvió a pasar límites de página, rangos inválidos, fallos de
lectura, recargas e identidad de caché; una serie de 1.000 lecturas del mismo
word no generó más accesos al archivo. Esta prueba protege frente a lecturas
erróneas de ROM, pero no mide todavía el lector físico del MODE.

La investigación anterior de la ROM y los paquetes de animaciones denominados
«FINAL» identifican datos y rutinas originales; no certifican su integración
completa en el port. Los 143 selectores conectados no equivalen a 143 acciones
terminadas. Tampoco el cociente 88/143 mide el porcentaje de contenido acabado:
solo identifica el alcance de una clase de prueba de un fotograma. Falta un
inventario de ramas y secuencias para calcular un porcentaje de terminación
defendible.

## Revisión del árbitro y diagnóstico R04 (26-10-2026)

Esta revisión sustituye la solución provisional de `$20482` descrita más abajo.
Se eliminó la sustitución del enlace de agarre por el objetivo del luchador:
la causa anterior estaba en `$1F9D0`, cuya condición estaba invertida en el
port. El árbitro debe buscar otra inmovilización cuando su actor anterior no
está bloqueado. La comparación directa de 388 variantes con la ROM pasó,
incluyendo ambas ramas de esa condición; la prueba inicial falló antes del
cambio. Los finales posteriores no representan una mejora de FPS: al cambiar
el comportamiento del árbitro cambia la trayectoria del combate.

La partida Royal entonces alcanzó dos rutas antes omitidas:

- Frame 7.051, `$21358`: el conteo invoca la limpieza con A0=árbitro, no con el
  luchador de A1. El registro real del árbitro ocupa `$64` bytes y los accesos
  a `+$7E`/`+$B6` corresponden a la cola compartida de efectos. El port conserva
  esa disposición al llamar a la limpieza ya implementada. Pasaron la captura
  exacta del fallo, 85 variantes Royal y 169 variantes TAG, comparando toda la
  RAM de trabajo, cola de dibujo, FG0, D0-D7 y el siguiente paso de RNG.
- Frame 7.052, `$2005C`: se validaba el enlace antes de que venciera el contador.
  La ROM sólo lo utiliza en `$2006C`. Si el agarre Royal ya se liberó, el acceso
  por cero escribe en ROM `$109` y el bus ignora esa escritura. El port reproduce
  esa operación; conserva el bloqueo para escrituras inválidas fuera de ROM.
  Pasaron 14 variantes de espera/final con el mapa ROM de sólo lectura.

Con las dos correcciones, el mismo ejecutable instrumentado completó Royal
en el frame 7.148 y TAG en el 3.373, con AddressSanitizer y
UndefinedBehaviorSanitizer activos, sin diagnósticos. Son trayectorias del
motor en host; no certifican todas las parejas ni la fluidez de Saturn física.
Evidencia: `referee-root-cause/`, y capturas originales reproducibles en
`src/runtime/tests/r18/referee_royal_snapshot_frame7051.bin` y
`referee_royal_snapshot_frame7052.bin`.

El usuario solicitó una nueva BIN/CUE y notificó texto de diagnóstico cortado.
La fuente estaba ampliada al doble y 42 columnas ocupaban casi 512 píxeles,
aunque la salida visible es de 320. R04 usa glifos 5x7 a tamaño nativo y margen
de CRT. Las cinco páginas pasan la prueba de límites de 320x240 y se revisaron
sus imágenes completas en `fault-r04/all-pages.png`. Se corrigió además la
etiqueta TAG/Royal del diagnóstico para usar el bit real de modo `0x01`.
El dibujo del diagnóstico sólo se ejecuta al fallar o cambiar de página.

Entrega autorizada R04: `validation/releases/wrestlefest-runtime18-eval-r04-20261026/`
y ZIP homónimo. La compilación SH-2 enlazó con 33.260 bytes disponibles de
heap frente a 16.384 reservados. Pasaron 9.276 sectores EDC/ECC, 19 archivos
CDFS y coincidencia exacta con el ejecutable enlazado. BIN SHA-256:
`c35d955a3dd8002b4a8dc3973a5bca6d4a6db8454a7534d89a19d3fff6e04c35`.
La copia local y el contenido del ZIP se comprobaron por SHA-256. Esta imagen
sustituye los dos artefactos internos autorizados; la entrega anterior se
conserva en su propia carpeta. No se ejecutó esta imagen R04 en emulador ni
Saturn física antes de la entrega; las pruebas de combate citadas son de host.

## Resultados verificados

- Los 143 selectores de la tabla ROM `$12614` están enlazados en el port. Esto
  acredita rutas conectadas, no equivalencia exhaustiva de todas sus ramas.
- Las matrices diferenciales de 88 selectores pasaron 412.248 casos con los
  doce personajes frente a instrucciones de la ROM. La matriz inicial de 16
  selectores aportó 58.752; `$72`, `$06` y `$00` aportaron 4.608 cada uno;
  `$4E`, `$6A`, `$4F`, `$70`, `$7B`, `$7C`, `$6B`, `$69`, `$68` y `$08` aportaron 3.456 cada
  uno, `$0A`, `$0C`, `$0D`, `$0E`, `$10`, `$15`, `$16`, `$17`, `$1E`, `$26`, `$29`, `$37`, `$3D`, `$4A`, `$4B`, `$4C`, `$51`, `$57`, `$59`, `$5E`, `$61`, `$65`, `$71`, `$73`, `$74`, `$76`, `$77`, `$81`, `$84`, `$85`, `$8B` y `$8C` aportaron 3.456 cada uno, y `$24`, `$33`, `$48` y `$4D` aportaron 4.608 cada uno. Las variantes adicionales de `$69`, `$6B` y
  `$68` se verificó además con el bit Royal `0x01`, `$69` con el bit de escena
  `0x02`, y `$6B` al llegar a la salida del ring.
  `$4C` añadió 3.456 casos con créditos y otros 3.456 en la situación especial
  de rescate, comparando también la memoria de paletas del relevo TAG. La
  prueba de cobertura confirmó las visitas de la ROM a `$1F760` y `$214C0`.
  `$0A` añadió 648 casos de nueve ramas concretas de interrupción, con
  comprobación del estado de la víctima. Esa prueba detectó que la liberación
  de `$8048`/`$800E` desplazaba al actor equivocado: la ROM sigue el enlace
  desde el compañero que ejecuta `$10B9A`, y el port ahora hace lo mismo.
  Otros 72 casos de `$0A` verificaron la interrupción de Royal con crédito,
  incluidos los tres puestos, ambos sentidos y la limpieza de la pareja
  enlazada cuando el selector es `$53`. La comparación reprodujo la rama
  `$21358` de la ROM y corrigió un bloqueo real del port en ese punto.
  `$0C` añadió 288 casos de cuatro resultados de contacto: derribo, contra y
  fallo, con comprobaciones de la rama ejecutada.
  `$0D` añadió 288 casos de tres tipos de impacto y un fallo del salto, con
  comprobaciones del estado del luchador golpeado.
  `$0E` añadió 216 casos de choque, fallo y respuesta especial en su fase final.
  `$10` añadió 288 casos de dos derribos, una contraacción y un fallo, con
  comprobaciones del estado resultante.
  `$1E` añadió 48 casos focalizados en las dos liberaciones de objeto, con
  comprobaciones del estado del objeto y de su dueño.
  `$33` añadió 216 casos en ambas cuerdas y en el aterrizaje, comprobando el
  estado de la víctima. Las posiciones quedaron separadas para no confundir
  un impacto de cuerda con un aterrizaje.
  `$37` añadió 6.912 casos con enlaces nulos literales y marcados, además de
  su matriz ordinaria. Ambos modos reprodujeron la posición y el estado de la ROM.
  `$61` añadió 3.456 casos con crédito y 3.456 casos con la marca de salida
  hacia `$59`, comparando las banderas del compañero y el contador de
  recuperación. `$59` y `$77` recibieron matrices completas; `$4D` se comparó
  con la cola de efectos ocupada para aislar el movimiento de entrada TAG.
  Los efectos visuales 19 y 20 de esa entrada pasaron aparte 528 casos con los
  once huecos posibles de la cola, los doce personajes y ambas orientaciones,
  comparando RAM, vídeo y registros con la ROM.
  `$48` añadió 288 casos de agarre TAG con crédito; la prueba exige que la
  inmovilización efectiva ocurra en los puestos de actor 1 y 9. El escenario
  enlaza recíprocamente a ambos luchadores, condición necesaria para entrar
  en esa rama de la ROM.
  `$51` añadió 72 casos de salida TAG con crédito, verificando las marcas de
  relevo distintas en atacante y víctima según la acción previa de esta.
  `$57` añadió 72 casos de liberación TAG con crédito, comprobando la máscara
  exacta de limpieza del compañero.
  `$76` pasó además 264 casos de su efecto de aterrizaje 8 con los once huecos
  de la cola, doce personajes y ambas orientaciones, incluyendo RAM y vídeo.
  `$65`, `$73` y `$74` pasaron sus matrices completas con la cola de efectos
  ocupada y comprobación explícita de la transición terminal a `$1C`/`$1D`.
  La prueba separada de efectos temporales de `$35`, `$65`, `$73` y `$74`
  volvió a pasar 672 casos con RAM, vídeo y registros frente a la ROM.
  `$5E`, `$81`, `$84`, `$85` y `$87` pasaron matrices completas; las tres reversiones
  comprueban además el estado y el selector de la víctima cuando vence el
  contador.
  `$88` (camino a la cuerda) y `$8E` (interludio Royal) pasaron también sus
  matrices de 3.456 casos cada una, incluidas las dos orientaciones.
  `$11` (golpe aéreo) y `$14` (agarre aéreo) pasaron 4.608 casos cada uno,
  incluyendo su contacto con el luchador enlazado.
  `$35` (otro golpe aéreo) pasó 5.760 casos, incluida su fase aérea 3, y `$44`
  (acción de cuerda) pasó 3.456, con las doce identidades de luchador. La fase
  aérea de `$35` detectó una máscara errónea tras contacto válido: se borraba
  `0x04` del selector, pero la ROM borra `0x10`. Quedó corregido y la matriz
  completa volvió a pasar. El verificador también rechaza ahora las ejecuciones
  con cero casos; ese fallo de montaje apareció al aislar la fase 3. La matriz
  exige visitar `$1785E`, la instrucción de limpieza de la ROM, con los doce
  luchadores y volvió a pasar con esa exigencia.
  Las variantes adicionales con bytes de modo `0x01`, `0x02` y `0x03`
  añadieron 5.760 casos cada una: `$35` suma 23.040 casos en cuatro
  combinaciones de modo y escena. El bit de juego `0x01` corresponde a Royal;
  el bit de escena `0x02` fuerza la fase 3 de la ROM. En las cuatro matrices,
  cada uno de los doce luchadores alcanzó las instrucciones ROM de acierto,
  fallo y salida pertinentes a su escena, y el port reprodujo la RAM y los
  registros. Esta cobertura de un paso no equivale a una trayectoria aérea
  completa con física y colisiones intercaladas.
  `$55` y `$56` pasaron 3.456 casos cada una frente a la ROM.
  `$5A` y `$5B` pasaron también 3.456 casos cada una.
  `$7D` y `$7E` añadieron 3.456 casos cada una, además de sus secuencias
  consecutivas ya verificadas.
  `$80` y `$82` añadieron 3.456 casos cada una, además de sus relevos
  consecutivos ya verificados.
  `$86` y `$89` añadieron 3.456 casos cada una; sus secuencias de 72 y 120
  pasos respectivamente ya se habían cotejado.
  `$12` pasó 5.760 casos, incluidos los fotogramas 3 y 5 de trayectoria y
  contacto; `$13` pasó 3.456 casos. Sus trayectorias parten de posiciones
  coherentes dentro del ring para que el oráculo compare una acción válida.
  `$78` y `$7F` pasaron 3.456 casos cada una.
  `$5E` pasó además 288 casos con salud cero y comprobación de su temporizador.
  `$66` pasó una matriz base de 3.456 casos y otros 3.456 con relevo TAG
  acreditado, comparando también la memoria de paletas. Sus doce secuencias
  de 50 pasos coincidieron con la ROM.
  `$23` pasó 7.776 casos de sus nueve fases con los doce personajes, ambos
  estados del latch, tres puestos de actor, tres fotogramas de animación,
  dos temporizadores y ambas orientaciones. La comparación detectó y corrigió
  tres diferencias de registros: dirección relativa al iniciar trayectoria,
  resistencia de la víctima al iniciar el agarre y limpieza de dos registros
  tras un impacto fallido. Otros 72 casos verificaron el golpe aéreo conectado
  y 72 la inmovilización TAG con crédito; esta última reveló y corrigió los
  dos registros de desplazamiento del rival. Es cobertura de fases y de esas
  ramas, no de todas las variantes internas.
  Las matrices de las mismas nueve fases pasaron también 7.776 casos por cada
  uno de los bytes de modo `0x01`, `0x02` y `0x03`. Con la matriz `0x00`, `$23`
  suma 31.104 casos de un paso; `0x01` identifica Royal y `0x02` añade el bit
  de escena. Esto no sustituye el seguimiento de todas sus trayectorias con
  física y colisiones intercaladas.
  Otros 72 casos de `$23` pasaron el aviso de contacto al árbitro por ambos
  lados, comparando los cuatro indicadores globales con la ROM.
  Otros 72 casos pasaron la caída por contacto de tipo tres, con su subfase
  y estado secundario cotejados tras los dos avisos sonoros.
  La liberación de la inmovilización TAG pasó 72 casos por señal de la víctima,
  72 por señal del atacante y 72 de espera. La primera variante detectó y
  corrigió los registros de desplazamiento durante la separación.
  `$8B` y `$8C` pasaron sus resultados de inmovilización en los cuatro
  participantes, y `$71` pasó además 528 casos de sus efectos visibles 13 y
  14 frente a RAM, vídeo y registros de la ROM.
  En `$00` se verificaron tanto el
  movimiento inicial como sus fases ya iniciadas con posiciones y límites de
  ring coherentes. La matriz de `$70` detectó y permitió corregir la elección
  indebida del descriptor al finalizar la animación de entrada. La matriz de
  `$15` detectó y permitió corregir los registros D1/D2 que permanecían con
  valores antiguos después de un desplazamiento durante el agarre. La matriz
  de `$17` detectó y corrigió la pérdida del valor previo de contacto y los
  registros D1/D2 tras la separación de los luchadores.
- Se compararon asimismo secuencias sucesivas de `$00` (40 pasos), `$08`
  (32 pasos), `$0C` (45 pasos), `$0D` (60 pasos), `$0E` (48 pasos por cada una de
  sus cuatro fases), `$10` (48 pasos por cada una de sus tres trayectorias),
  `$15` (45 pasos), `$16` (45 pasos), `$17` (39 pasos), `$23`
  (80 pasos desde la fase cero con bytes de modo `0x00` y Royal `0x01`, y 74
  con `0x02` y `0x03`), `$24`
  (63 pasos), `$26` (45 pasos), `$29` (60 pasos), `$33` (60 pasos),
  `$37` (40 pasos), `$48` (40 pasos), `$4C` acreditado (50 pasos), `$57`
  (40 pasos), `$59` (40 pasos), `$66` (50 pasos), `$76` (40 pasos), `$77`
  (50 pasos), `$7D`, `$80`, `$81`, `$82`, `$84` y `$85` (80 pasos cada uno), `$7B`
  (4 pasos), `$7C` (11 pasos), `$7E` (104 pasos hasta la liberación) y `$89`
  (120 pasos), `$86` y `$87` (72 pasos cada uno), y `$55`, `$56`, `$5A` y `$5B`
  hasta su salida (49, 20, 32 y 36 pasos, respectivamente), `$78` (19 pasos)
  y `$7F` (108 pasos hasta el relevo) para cada uno de los doce personajes:
  31.860
  comparaciones de frame consecutivo
  adicionales frente a la ROM. Los relevos `$7B`→`$7C` y `$7C`→`$17`, y el
  estado final del lanzamiento `$24` y de la liberación `$17`, se verificaron explícitamente.
  En `$23`, las cuatro secuencias por personaje pasaron después de corregir
  una expectativa del verificador: en Royal `0x01`, el primer relevo de la
  subfase 0 ocurre en el paso 73, pero las llamadas aisladas posteriores
  continúan hasta otra subfase. RAM y registros ya coincidían con la ROM en
  cada paso. Ahora la prueba comprueba el primer relevo, y no confunde el
  último estado del ensayo aislado con el final de una partida real.
  La reversión `$81` alcanzó además su relevo real a `$7B` y `$15` al final de
  sus 80 pasos. Un primer fallo de esta prueba se debió al adaptador de
  verificación, que restauraba indebidamente los indicadores globales de
  `$23` también después de otras acciones; quedó corregido y la secuencia pasó.
  `$84` y `$85` alcanzaron también sus caídas finales y el selector `$06` de
  la víctima tras 80 pasos para los doce personajes.
  `$80` y `$82` completaron sus 80 pasos y relevaron a `$7B`/`$15` igual que
  `$81`, conservando las diferencias de temporizador de su descriptor.
  `$89` coincidió durante 120 pasos con la ROM, incluyendo el empujón único
  del compañero al acabar el primer ciclo de animación.
  `$7E` coincidió durante sus dos ciclos de agarre hasta soltar al compañero
  y pasar a `$7B`/`$15` en el paso 104 de cada personaje.
  `$7D` completó sus 80 pasos y transfirió el control a `$7B`/`$15` en los
  doce personajes.
  `$86` y `$87` completaron 72 pasos consecutivos cada uno frente a la ROM.
  Las cuatro salidas de lanzamiento `$55`/`$56`/`$5A`/`$5B` comprobaron además
  que atacante y víctima terminan en sus estados respectivos, no solo que la
  memoria coincide durante la espera.
  `$78` terminó en estado 7 y `$7F` completó dos ciclos de animación antes de
  devolver a la pareja a `$7B`/`$15`, en los doce personajes.
- El atlas de sprites emparejados analizó 21.696 composiciones (904 poses por
  doce parejas y dos orientaciones) sin bloqueos. El verificador del parser
  contra la ROM pasó 665 casos de sprites directos.
- La precarga TAG ahora contiene 7.822 tiles en formato R18D y reserva 96
  huecos de caché. Reúne los tiles vistos en las partidas completas controlada
  y sin entrada, y prioriza otros tiles según trazas históricas TAG/Royal.
- Partida TAG controlada: fin real en frame 14.409, 0 lecturas de `ROM68K.BIN`
  y 0 de `SPRITES.PAK` durante la lucha (antes: 4 y 4.696).
- Partida TAG sin entrada: fin real en frame 4.415, 0 lecturas de ROM y 0 de
  sprites durante la lucha (antes: 2 y 629).
- Las dos partidas anteriores también completaron con AddressSanitizer y
  UndefinedBehaviorSanitizer sin diagnóstico de memoria ni bloqueo, también
  tras las correcciones de `$70`, `$15` y `$17` (frames 14.409 y 4.415; 0
  lecturas de CD). Esta comprobación final se repitió después de modificar
  `$15` y `$17`. Se repitieron otra vez después de corregir `$0A`, con los
  mismos finales (14.409 y 4.415), cero lecturas de CD y ningún diagnóstico
  de memoria. Registro: `validation/local-20261026/asan-after-action0a.log`.
- Tras las correcciones de `$23`, ambas partidas TAG volvieron a terminar con
  AddressSanitizer y UndefinedBehaviorSanitizer activos: frames 14.409 y
  4.415, cero lecturas de ROM o sprites durante el combate y ningún
  diagnóstico de memoria. Registro:
  `validation/local-20261026/asan-after-raw23-final.log`. El archivo
  `wf_r17_raw23.c` pasó además la comprobación de sintaxis del compilador SH-2
  con las opciones de entorno independientes de la imagen.
  La comprobación se repitió tras la corrección final de liberación TAG:
  `validation/local-20261026/asan-after-raw23-release.log` registra los mismos
  dos finales, cero lecturas de CD en combate y ningún error de memoria.
- Tras la corrección de la máscara en `$35`, dos combates TAG volvieron a
  terminar en los frames 14.409 y 4.415 con AddressSanitizer y
  UndefinedBehaviorSanitizer activos, sin diagnósticos ni lecturas de ROM o
  sprites durante el combate. Registro:
  `validation/local-20261026/asan-after-raw35-mask.log`. El archivo corregido
  pasó también la comprobación de sintaxis SH-2.
- Una partida Royal completa del motor con perfil de entrada `pressure`
  avanzó 12.000 frames sin bloqueo ni diagnóstico de AddressSanitizer o
  UndefinedBehaviorSanitizer. Es una ventana de combate prolongada, no una
  victoria ni una prueba de velocidad en Saturn física. Evidencia:
  `validation/local-20261026/royal-pressure-asan/report.json` y `host.log`.
- Después de la limpieza Royal de `$0A`, otra partida Royal de 12.000 frames
  pasó con AddressSanitizer y UndefinedBehaviorSanitizer. La prueba específica
  de `$22`, que comparte la limpieza `$21358`, pasó 1.946 casos frente a la ROM
  original. Evidencia: `after-royal-and-referee-fix/royal-report.json` y
  `royal-host.log`.
- Un combate TAG instrumentado encontró un bloqueo reproducible en el frame
  5.338: el árbitro cerraba el resultado después de un KO con un enlace de
  agarre ya liberado. Una prueba aislada reprodujo primero el fallo de
  `$20482`. El cierre ahora usa el rival válido del luchador si falta aquel
  enlace y marca a los cuatro participantes. La prueba de regresión pasó y
  el combate completo terminó en el frame 5.462 sin diagnóstico de memoria.
  Evidencia: `after-royal-and-referee-fix/tag-report.json` y `tag-host.log`.
- `test-runtime17-f4c2` volvió a pasar. Su caso de selector desconocido usaba
  `$35`, ya implementado; ahora usa `$8F`, fuera de la tabla ROM, y comprueba
  que el despachador sigue bloqueándolo.
  Se repitió después de corregir `$35`; el registro es
  `validation/local-20261026/f4c2-after-raw35-mask.log`.
- Enlace SH-2 tras corregir `$70`: 33.804 bytes disponibles para heap frente a 16.384 reservados.
  El disco interno pasó 9.276 sectores con EDC/ECC exactos y coincidencia de
  todos los archivos ISO/CDFS. BIN SHA-256:
  `a38f62e63e665f69f472e2060ddea4986efdc7d61bc6085d6bf8ac798a17c5d4`.
- La compilación Saturn posterior a las correcciones de `$15` y `$17` pasó:
  33.708 bytes disponibles para heap; 9.276 sectores con EDC/ECC y cargas
  CDFS exactos, BIN idéntico al ejecutable enlazado. SHA-256 interno:
  `946c2452a5e9536d1a090adc829d5eabebc0b0728fa3aad44e439d6f2743d6e3`.
- La compilación Saturn posterior a `$0A` pasó con 33.676 bytes disponibles
  para heap. El informe `validation/local-20261026/disc-validation-after-action0a.json`
  confirma 9.276 sectores EDC/ECC exactos, todas las cargas CDFS exactas y
  el BIN idéntico al ejecutable enlazado. SHA-256 interno:
  `9a23c9f8a995e44a10eca70d2d2acf155d5bfa02179311ffcdfc73fbcc3469d9`.
- La compilación separada de instrumentación de frames enlazó y su disco
  interno pasó la validación exacta de 9.228 sectores. Mednafen ejecutó su
  combate controlado hasta el frame 120: del 30 al 120, 90 frames consumieron
  102 VBlanks (1,133 por frame; unos 53 fps simulados a 60 Hz), sin lecturas
  de CD ni fallo fatal. Es una ventana corta, no prueba sostenida de hardware.
- Tras la autorización expresa para sustituir los artefactos internos, el
  ejecutable Saturn enlazó con 33.132 bytes de heap disponibles frente a
  16.384 reservados. La BIN/CUE interna pasó la comprobación exacta de 9.276
  sectores, EDC/ECC, 19 archivos CDFS y coincidencia de `A.BIN` con el
  ejecutable enlazado. La pareja copiada al espacio de trabajo tiene SHA-256
  BIN `6b77e2c35c835a776b60445992b673cfad3adbdb2e671d383c4a3b45d900b115`
  y CUE `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`.
  Informes: `after-royal-and-referee-fix/disc-validation.json` y
  `hwram-budget.json`. Todavía no hay prueba de esta imagen en Saturn física.

## Límites y trabajo pendiente

- Las cuatro fases aisladas de Royal `$8E` pasaron antes 653 pasos en total.
  La prueba nueva de 7.836 pasos con relevo de estado intercalado verifica ya
  el flujo entre fases y el paso terminal a `$79` en los doce personajes. Sigue
  siendo una escena construida, no una partida Royal completa, una verificación
  de arte gráfico ni una prueba de FPS.

- La residencia cero se ha probado en esas dos rutas TAG. Otras parejas,
  modos, entradas y trayectorias pueden pedir tiles fuera de la precarga.
- El disco interno aún no demuestra fluidez en la Saturn física ni una tasa
  sostenida de 60 fps. Faltan ventanas largas, otros modos y prueba física.
- La imagen interna posterior a `$23`, `$35`, la limpieza Royal de `$0A` y el
  cierre TAG del árbitro ya enlazó y pasó la validación exacta del disco. El
  usuario autorizó sustituir los dos archivos internos; esta imagen no se ha
  presentado aún como una nueva entrega de evaluación.
- El arranque en Mednafen de esta VM reconoció 4.096 bytes del ejecutable
  exacto y no registró fallo fatal durante sus primeros 80 ciclos de vídeo.
  Esa emulación avanzó lentamente y la captura quedó negra; no valida el
  frontend, un combate ni su velocidad.
- Faltan matrices de variantes y pruebas de secuencia para los selectores que
  hoy sólo están conectados o tienen pruebas parciales, además de escenas
  completas, audio y situaciones posteriores al combate.
- Las pruebas aisladas anteriores de `$11`, `$14` y `$35` ya fueron ampliadas
  con física, límites del ring y relevo de estado intercalados; sus resultados
  nuevos figuran al comienzo de este documento. Siguen pendientes más
  posiciones, rivales, colisiones y partidas completas.
- Hay 143 selectores enlazados; 88 están en la matriz para los doce personajes
  y otros 55 tienen una prueba diferencial con nombre propio. Hay 78
  verificadores individuales y 72 en el verificador de familias. La unión cubre
  los 143 identificadores. Ese inventario de nombres no cuenta ramas internas
  ni secuencias completas: aún quedan variantes sin cotejo directo.
- La petición posterior del usuario autorizó una BIN/CUE de evaluación. Está
  en `validation/releases/wrestlefest-runtime18-eval-20261026/`; no equivale
  a una versión de contenido completo.

## Reproducción de la evidencia

Las trazas `validation/runtime18-match-tile-use.csv` y
`validation/runtime18-noinput-tile-use.csv` alimentan
`validation/local-20261026/plan_match_hot_tiles.py`. El informe de capacidad
está en `match-hot-selection.json`. La prueba exacta de tiles es
`make -C src/runtime test-runtime18-hot-sprites`; las dos partidas se ejecutan
con `test-runtime18-complete-match` y `test-runtime18-noinput-complete-match`.
La compilación interna usa `validation/build-runtime18.sh` y su informe
`validation/results/runtime18/disc-validation.json` en la VM.
