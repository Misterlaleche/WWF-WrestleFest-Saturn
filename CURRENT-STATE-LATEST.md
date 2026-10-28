# Estado actual: acciones, frontend y audio, 28 de octubre de 2026

## Respuesta de colisión `$24AA8` y aparición HOST (28-10-2026)

La respuesta `$24AA8` dejaba de ejecutar la limpieza y los efectos del agarre
y bloqueaba cuando el subtipo seguía la alternativa del arcade. Se implantaron
ambas rutas, incluyendo restauración del estado atacante, limpieza de señales,
liberación y verificación de los punteros que se consumen. Antes del cambio,
una entrada controlada quedaba bloqueada en el port mientras la ROM continuaba.
Después pasaron 4.096 variantes frente a la ROM, dos casos alternativos con
vínculo nulo o inválido que la ROM no utiliza, y una guarda para un puntero
realmente consumido. Informe `validation/local-20261028/collision-24aa8-rom.json`.

La ruta de limpieza apareció durante una partida Royal HOST instrumentada de
carrera, frame 1.550, participante 8 contra 0; el recorrido continuó hasta
14.000 frames. Su estado anterior a la colisión se volvió a ejecutar contra la
ROM en una comparación aislada y coincidieron RAM proyectada y D0–D7. Al
repetir esa misma captura controlada con la rutina de R05 se detectaron ocho
diferencias en RAM: es una repetición aislada, no una partida R05 capturada.
Evidencia `collision-24aa8-natural-capture/royal-rush/host.log`,
`collision-24aa8-natural-capture/natural-r05-red.log` y
`collision-24aa8-natural-rom.json`. No es emulación Saturn, prueba física,
certificación del combate completo ni identificación del bloqueo de la foto.
Una partida TAG HOST avanzó también 14.000 frames con estas fuentes; no mide FPS.

Las seis comprobaciones de acciones/fase se actualizaron en `revalidate-24aa8`;
el inventario sigue en `0/143` selectores certificados. El ELF SH-2 combinado
compiló sin símbolos pendientes y pasó las comprobaciones de pilas y memoria:
18.700 bytes para heap frente a 16.384 reservados. SHA-256:
`9223d8128661d0a9a82efc8a9bb9bc361dabeb5f54d9f57e9eaab75e28878534`.
Fuentes y presupuesto están en `validation/local-20261028/collision-24aa8-build/`.
Sin BIN/CUE nueva ni prueba en emulador Saturn o consola.

## Respuesta de agarre `$24924` con orientación igual (28-10-2026)

La ruta igual de `$24924` devolvía éxito después de modificar dos estados,
pero omitía borrar marcadores de agarre, la notificación TAG, la limpieza del
luchador enlazado y la liberación del vínculo. Se restituyeron los efectos de
la ROM y una comprobación previa de los punteros; debería evitar una respuesta
incompleta en esos contactos. La normalización intermedia del subtipo se omite
porque su resultado es sobrescrito antes de ser consumido y preserva D0/D1.
La comparación con la ROM completa primero detectó cinco diferencias de RAM;
tras la corrección pasaron 3.072 variantes controladas de TAG/Royal, créditos,
subtipos y vínculos, más una guarda de puntero inválido sin escrituras. Informe
`validation/local-20261028/collision-24924-equal-rom.json`. La ruta opuesta
pasó también 4.167 casos (`collision-24924-mismatch-after-equal.json`).

Una partida automática TAG HOST de Hogan/Boss Man avanzó 14.000 fotogramas.
Cuatro recorridos instrumentados de TAG y Royal (agarre y carrera) no
alcanzaron la nueva ruta; no se acredita aparición natural ni Saturn física.
Evidencia `collision-24924-equal-tag/report.json` y
`collision-24924-natural-probe/report.json`. Los resultados son independientes
de la fluidez real. Se regeneraron las seis pruebas de acciones/fase en
`revalidate-24924-equal`; el inventario permanece en `0/143`. La compilación
SH-2 de esta etapa se detuvo para incorporar `$24AA8`; la compilación combinada
se registra en la sección anterior. No se ha preparado BIN/CUE.

## Respuesta de agarre `$24868` con luchadores opuestos (28-10-2026)

Antes, cuando los luchadores miraban en direcciones opuestas, la ruta de
colisión `$24868` podía detener el combate con temporizador bajo y, en otros
casos, continuaba sin completar la limpieza del agarre. Se implementaron la
respuesta enlazada y su alternativa de inversión, incluyendo estados,
marcadores, notificación TAG, limpieza y liberación. Esto debe permitir que
las situaciones cubiertas continúen, pero no añade un movimiento identificado
ni prueba que sea el bloqueo de la fotografía.

Una comparación diferencial de la llamada completa `$2435A` pasa 8.192
variantes controladas con ambos temporizadores, TAG/Royal, créditos, vínculos
y alias entre participantes; compara RAM y D0-D7 con la ROM e incluye dos
comprobaciones de bloqueo seguro para punteros inválidos. Los casos alcanzan
los puntos `$24878`, `$248A2`, `$248AE`, `$248CE`, `$248DA`, `$248E6` y
`$248F0`. El caso sin corrección primero se detuvo en el port y continuó en la
ROM. La prueba anterior de orientación igual sigue pasando sus ocho casos;
las otras respuestas verificadas se comprobaron nuevamente. Informe
`validation/local-20261028/collision-24868-opposite-rom.json`.

Con el código actual, una partida HOST automática TAG de Hogan/Boss Man avanzó
14.000 frames y una Royal llegó a GAME OVER en el frame 9.322. No se ha probado
que estos recorridos hayan entrado en la nueva rama ni que reproduzcan el caso
físico, y no miden FPS. El inventario se actualizó con las seis pruebas de
acción/fase en `revalidate-24868-opposite`: `0/143` selectores certificados.
El ejecutable SH-2 de esta etapa compiló sin símbolos pendientes y pasó las
comprobaciones de pilas y presupuesto: 18.956 bytes para heap frente a los
16.384 reservados. SHA-256 del ELF interno:
`571c085895ed7cb4e19f3144eb33bea9712a4ea1e1a69d4109b5ff9368fca3a3`.
Fuentes y presupuesto están en `validation/local-20261028/collision-24868-build/`.
No se ha generado BIN/CUE ni probado el cambio en emulador Saturn o consola.

## Bloqueo TAG fotografiado en Saturn real (28-10-2026)

El usuario probó la última imagen R05 con Hogan y Big Boss Man y fotografió
las cinco páginas. La pantalla R04 registra `OWNER 024062`, `DEP 024408`,
`FRAME 0000029C`, TAG y `SLOT FF`. La segunda página dice que no se ha
identificado participante; la tercera conserva estados y posiciones de los
actores. El mando aparece sin botones mantenidos en la instantánea, lo que no
permite recuperar la entrada inmediatamente anterior. Los contadores de disco
son acumulados y estas fotos no demuestran que la causa sea una lectura de CD.

Se cotejó el código archivado de R05: `$024408` identifica la primera rutina
del par, porque `blocked_callback()` prioriza siempre esa dirección. La rutina
primera de ese selector solo acepta o rechaza; no devuelve bloqueo, de modo
que el bloqueo se produjo en la segunda respuesta del par. Las fotos no
incluyen ni perfiles ni par de participantes ni la segunda rutina: no se puede
identificar la rama exacta ni afirmar que las correcciones posteriores ya
resuelven este caso concreto.

Se añadió la captura de la pareja de colisión que ya se registraba en memoria,
mostrándola en la página 2 del diagnóstico R05 con slots, perfiles, selectores,
ambas rutinas, vínculo, subtipo, temporizador, orientación y flags. Solo se
copia al fallar, con validación para no atribuir al bloqueo una captura antigua;
el trazado de juego no añade trabajo por frame. Una prueba HOST verifica
captura, guardas, texto y límites del área visible. No puede recuperar los
datos omitidos de una imagen ya tomada, ni sustituye una prueba en Saturn.

Durante la revisión se encontró además un fallo concreto en `$24A90`, una de
las posibles respuestas de colisión, aunque no se conoce la respuesta de la
foto. La ROM usa el vínculo `+$76`; el port utilizaba el de agarre `+$26`.
Antes del cambio, una prueba controlada con `+$26` vacío y `+$76` válido
bloqueaba en el port mientras la ROM proseguía. Se cambió la resolución del
vínculo y se cotejaron 30 variantes controladas con la ROM completa, más un
caso de puntero inválido que no modifica memoria. La prueba HOST del detector
completo también pasa. Informe `validation/local-20261028/collision-24a90-rom.json`.
Una partida HOST con Hogan y Boss Man, rivales seleccionados por la prueba y
entradas automatizadas de agarre, avanzó 14.000 frames sin bloquearse; no
reproduce el incidente físico ni terminó el combate. El informe se encuentra en
`validation/local-20261028/physical-fault-link-fix-tag/report.json`. No demuestra
que `$24A90` fuera la causa de la foto ni una reparación en Saturn física.
Se actualizaron las seis pruebas de acción y fase a las huellas actuales en
`revalidate-physical-fault-link-fix`; el inventario se ha comprobado y la barra
continúa en `0/143`. El ejecutable SH-2 final compiló sin símbolos pendientes
y pasó la comprobación de pilas y presupuesto: 19.212 bytes para heap frente
a 16.384 reservados. Quedan 2.828 bytes sobre esa reserva; incorporar música
requiere revisar de nuevo el presupuesto. SHA-256 del ELF interno:
`329be623646a745fcc4a884ebb71b2d4f90f6a21a0c636d4eed723f7606496fe`.
El mapa, hashes de fuentes y presupuesto están en
`validation/local-20261028/physical-fault-diagnostic-build/`. No se ha
preparado una imagen BIN/CUE; no hay prueba en emulador Saturn ni en consola.

## Interrupciones de agarre: dos respuestas de colisión (28-10-2026)

En R05, dos respuestas de colisión aceptaban algunos contactos sin ejecutar
la secuencia completa del arcade. En `$24A42` faltaban la limpieza de agarres,
la notificación y la liberación del vínculo; en `$24A06` faltaban la respuesta
del luchador enlazado y la limpieza. Se restituyeron esas operaciones y se
comprueban los punteros necesarios antes de modificar los actores. En juego,
estos contactos deberían continuar con las reacciones y liberaciones correctas;
no se presenta esto como un movimiento nuevo ni como una reparación general.

La comparación con la ROM completa pasó 1.024 variantes controladas de
`$24A42` y 3.072 de `$24A06`, incluidas situaciones TAG/Royal y vínculos
compartidos; ambos tests comprueban también el rechazo de un puntero inválido
sin alteración parcial. Otras regresiones pasaron 49, 289, 1.881 y 4.167 casos.
Los datos son controlados a partir de capturas HOST: no demuestran que las dos
ramas nuevas hayan aparecido de forma natural en una partida ni que se hayan
probado en Saturn. Informes `collision-24a42-rom.json`, `collision-24a06-rom.json`,
`collision-24a88-rom.json`, `collision-249be-rom.json`,
`collision-24984-expanded-final.json` y `collision-24924-expanded-final.json`
en `validation/local-20261028/`.

Con el código actual, una partida de prueba HOST de Royal alcanzó GAME OVER
en el frame 9.322; una TAG de agarres avanzó 14.000 frames sin finalizar.
Esto no mide velocidad ni acredita la estabilidad de todas las situaciones.
Los tests de regresión HOST 08, 13 y 16 pasan. No se ha creado una BIN/CUE.
El inventario mantiene `0/143` selectores íntegramente certificados.

Se regeneraron las seis pruebas de acción y fase que registran todos los hashes
de fuentes en `revalidate-expanded-collision`. Los informes históricos que
registran solo el archivo de acciones conservan su hash original: el inventario
verifica comparando bytes con la fuente archivada de R05 que el único cambio
en ese archivo es la nueva función pública de limpieza, no utilizada por esos
tests. Esto preserva evidencia limitada, no certifica la integración actual.
El ejecutable interno SH-2 compiló y enlazó sin símbolos pendientes; la
comprobación de pilas pasó y el presupuesto deja 20.300 bytes de heap frente a
16.384 reservados. Su SHA-256 es
`2ff224793623544a3b36afb754e93c89f9f6132937485d2f9ec55c3e906bb717`.
Los archivos de enlace y las diez fuentes modificadas frente a R05 están
identificados en `validation/local-20261028/expanded-collision-build/`.
La compilación no equivale a una prueba en emulador o en Saturn real.

## Controles y colisiones Royal: cambios posteriores a R05

Se detectó un error concreto en `$F2D6`: el preprocesador tenía una copia
incorrecta de tres entradas de la tabla de dirección. Con el mapeo actual,
arriba, arriba-derecha y arriba-izquierda tomaban ángulos equivocados. Ahora
ambas rutas usan la tabla compartida ya contrastada con la ROM y conservan
el registro D1 que calcula el original. Las 16 entradas coinciden en caminar
y en agarre móvil: 32 pruebas de RAM y D0–D7. Esto no demuestra por sí solo
que todas las direcciones u orientación estén corregidas en Saturn física.

Al ampliar la comparación `$F18A`, se corrigió también el tratamiento de D1/D2
en carrera y una lectura: `$F3DE` consulta el byte ROM `$00A3` (`$FF` en la
ROM verificada), no `$1C00A3` en RAM. La matriz cubre 384 variantes, 32 de
ellas con la llamada RNG original; 12 capturas HOST de otras fases reutilizadas como
entradas controladas coinciden en RAM y D0–D7. Evidencias
`validation/local-20261028/f18a-direction-rom.json`, `f18a-run-state-rom.json`
y `f18a-facing-audit.json`.

La nueva trayectoria Royal de agarres expuso dos bloqueos de colisión en el
frame 3834. Se implementaron las ramas originales de orientación opuesta
`$24A88` y la respuesta enlazada de temporizador bajo `$249BE/$249E6`, con
49 y 289 comparaciones frente a la ROM, respectivamente. La segunda cubre
variantes de TAG/Royal y conserva la llamada de audio cuando corresponde.
Con ambas correcciones, la misma partida HOST terminó en frame 6332; otra
partida TAG avanzó 14.000 frames y cuatro perfiles adicionales terminaron o
avanzaron hasta 14.000. No se ha probado el cambio en Saturn ni medido FPS.
Informes: `collision-24a88-rom.json`, `collision-249be-rom.json`,
`royal-collision-final/report.json` y `tag-collision-final/report.json` en
`validation/local-20261028/`. Las demás ramas de colisión siguen necesitando
validación y algunas conservan bloqueos explícitos.

El ELF posterior a todos estos cambios enlazó sin símbolos pendientes. Las
pilas pasaron su comprobación y quedaron 21.452 bytes para heap frente a los
16.384 reservados. SHA-256 del ELF:
`6fcee9d2a3400fa0dbef39ec2ace523a2bf2b10e879bfe9a3fe3d4a739116805`.
Fuentes, mapa y presupuesto se guardaron en `control-collision-build/`.
El enlace no equivale a probar una imagen en el emulador ni en Saturn real.

Se repitieron los seis oráculos de acción/fase cuyas huellas de fuentes
quedaron invalidadas por estos cambios; el inventario vuelve a comparar hashes
estrictos contra `revalidate-collision-final`. No se cuenta esto como un
selector certificado nuevo. La barra sigue en `0/143`.

## Royal: continuación después de GAME OVER

La R05 quedaba en un bucle tras ejecutar `$1AFC..$1B1D`. El código actual
continúa en `$1B1E`: reúne hasta cuatro participantes activos en `$1C0092`,
llama al ranking `$65C0`, vuelve a `$6FC` e intenta guardar los récords en
memoria interna. La rutina no borra los huecos que la ROM conserva. Los
valores D/A, la RAM y el punto de llamada a ranking coinciden con la ROM en
128 variantes que cubren las 16 máscaras de participantes; se verificó la
frontera de retorno. Evidencia `validation/local-20261028/royal-gameover-collect-rom.json`.

Una partida Royal HOST sin forzar el resultado terminó en frame 2872, mostró
el tramo de GAME OVER, llamó al ranking y volvió al título conservando las
tablas en RAM. Evidencia `validation/local-20261028/royal-natural-post/report.json`
y `host.log`. El entorno usa un adaptador de prueba y no verifica entrada manual
de iniciales, grabación BUP, VDP1 Saturn ni hardware físico. El tramo GAME OVER
anterior pasa 28 comparaciones con la ROM; la regresión TAG pasa 337 casos.
El ejecutable SH-2 de esa etapa anterior enlazó sin símbolos pendientes; las pilas pasaron la
comprobación y quedaron 21.900 bytes para heap frente a 16.384 reservados.
El ELF interno tiene SHA-256 `368552ce98ad58ca29179fa8950bd7bc1091d5f927f254ced45c73ffe57fa254`;
el presupuesto, el mapa y los hashes de los cuatro archivos superpuestos a la
compilación R05 están en `validation/local-20261028/royal-continuation-build/`.
No se ha generado una nueva BIN/CUE. El objetivo de animaciones y la barra
estricta `0/143` siguen abiertos.

Tras los cambios posteriores en el núcleo de combate se volvieron a ejecutar
los seis informes de acciones y el inventario usa de nuevo hashes estrictos,
como se detalla en la sección de controles y colisiones anterior.

**Prioridad vigente:** el usuario aplaza la música nativa y pide completar y
cotejar el 100 % de selectores, acciones y animaciones con la ROM arcade,
cerrando los bloqueos del camino. La barra estricta sigue en `0/143`
selectores certificados.
Una captura natural histórica Royal del personaje 8 en `$07` pasó 38 pasos
frente a la ROM y a la traza de poses; completa la evidencia de ocurrencia
natural de los dos personajes de las tablas auditadas, pero no demuestra todas
las entradas indirectas ni la escena completa o VDP1. Informe y límites en
`validation/local-20261028/action07-royal-natural-rom.json`.
Una caché de 64 palabras ROM añadió 384 bytes (total 7.300) y redujo las
llamadas al lector de metadatos de 45.402 a 20.376 en 10.787 dibujos de una
traza TAG, con salida idéntica. Pasaron 2.356 comparaciones gráficas directas
con la ROM y la prueba de caché con sanitizadores, incluida invalidación tras
cambio de fuente. Es una medida de trabajo interno, no de FPS ni de lecturas
de CD; véase `validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`.
Las animaciones normal y alternativa del selector `$00` coincidieron con la
ROM hasta volver al estado 0 para los doce luchadores: 39 y 33 pasos por
personaje. Nueve posiciones de entrada, con ambos bordes del ring y de la
escena exterior, también coincidieron: diez informes, 120 trayectorias y
4.608 pasos de acción. El inventario integra ahora 77 informes de secuencia
vigentes. Faltan otras combinaciones de contacto y la escena completa antes
de certificar ese selector; la barra permanece en 0/143.
En dos combates HOST, 19 secuencias naturales completas de `$00` siguieron
los 39 pasos y poses del oráculo de ROM; otras nueve quedaron parciales o
interrumpidas. Una fase completa `$F4C2` capturada durante TAG en el frame
1261 coincidió con la ROM en RAM, primer plano y D0–D7. Los detalles están
en `validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`.
La matriz actual de `$00` coteja 96.384 estados con la ROM; cubre ambos
resultados de 51 condiciones de acción/contacto y seis del despacho y avance
de animación. Una alteración deliberada en una copia temporal fue detectada.
Dos secuencias de la fase completa de luchadores pasaron 39 pasos normales y
33 alternativos; la segunda usa una marca modificada en una captura HOST.
Se localizó además la alternativa en una traza TAG histórica y su captura se
cotejó con la ROM y el código actual hasta la salida, para el personaje real
y once variantes controladas. La partida histórica bloqueó más tarde y no
demuestra una partida completa con el código actual ni la salida final VDP1.
Para `$07`, la continuación histórica del personaje 7 coincide con la ROM en
42 pasos; once variantes controladas terminan en 38 pasos cada una. La ROM
solo contiene las tres poses de esa acción para los personajes 7 y 8, que
son también los únicos seleccionados por las tablas humana y CPU auditadas.
Faltan asignaciones indirectas, escena actual y VDP1 antes de certificarlo.
Una prueba nueva de fase completa `$F4C2` detectó que faltaba la llamada
`$F8D8` en la escena exterior: omitía la colocación del marcador fijo y el
cambio de lista de dibujo del luchador. Se implementó; dos fases TAG y una
Royal coinciden ahora con la ROM en RAM, primer plano, cola de dibujo y
registros D0–D7. Pasaron también las tres ramas de posición en sus cuatro
límites y un combate TAG HOST completo. Evidencia en
`validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`.

La ampliación a diez fases completas reveló un segundo error: los estados
principales `$0B/$0C` avanzaban de la pose alternativa `$FE` a la pose 3,
cuando la ROM reinicia el descriptor de tipo 2 en la pose 0. Se corrigió;
las diez fases TAG/Royal coinciden ahora en RAM, primer plano, cola y
registros, y un combate Royal HOST avanzó 3500 frames. La barra de
selectores permanece en `0/143`.

Cuatro continuaciones controladas de la fase completa `$F4C2` pasaron
768 fases y 722.848 instrucciones ROM sin diferencias en RAM, primer plano
ni D0–D7. La ruta Royal incluyó el relevo del agarre `$0C` al actor enlazado
tras 204 fases. No son fotogramas completos del arcade; el detalle está en
`validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`.

Las secuencias intercaladas de `$11`, `$14`
y `$35` ya pasaron para doce personajes (831, 692 y 6.290 pasos respectivamente;
la última cifra suma los cuatro valores de modo); `$35` llegó al impacto con
los doce en cada modo. El interludio Royal `$8E` atravesó sus cuatro fases y
el relevo final en los doce personajes, 7.836 pasos cotejados con la ROM.
El lanzamiento `$44` cubre los cinco índices de acierto y los cuatro índices
que pueden fallar, con relevo `$82`→`$7B`, para los doce luchadores. `$11` y
`$14` cubren también el impacto contra rival derribado hasta
la salida del atacante; frente a un rival activo `$61`, `$11` provoca `$77`
y `$14` falla el contacto en las posiciones ensayadas. `$35` cubre ahora
impacto contra rival derribado y fallo contra rival activo en los cuatro
modos, más un fallo cerca de la cuerda en los modos 2/3. `$12` y `$13` ya
continúan sus impactos hasta la salida del atacante al estado 7. En las
rutas de impacto de `$11`, `$12`, `$13`, `$14` y los cuatro modos de `$35`,
el rival llega al reposo tras su caída para los doce luchadores. `$0C` y
`$10` atraviesan ahora sus rutas de golpe al rival derribado (subestados 8 y
9), contragolpe `$61` y fallo; `$0D` cubre sus rutas 0, 1, 2 y sin golpe;
`$0E` atraviesa las cuatro fases de carrera y sus salidas de golpe, fallo y
reacción especial `$54`. El rival se recupera hasta reposo tras los golpes
de `$0C`/`$10` y `$0E`→`$51`; los contragolpes `$0C`/`$10`→`$77` entregan
`$61`/`$37`. Las tres reacciones de `$0D` también llegan al reposo.
Cada variante pasó para los doce luchadores. Hay 67 informes
de secuencia (82.193 pasos del atacante y 21.624 del rival)
asociados a la versión actual del verificador.
El inventario de la ROM registra los 143 selectores,
su verificador y las elecciones de los doce luchadores; todavía no certifica
las ramas internas. Una auditoría directa cotejó los 180 descriptores de
animación, 519 pares de pose y duración y los 199 punteros de selección con
la ROM sin discrepancias; aún falta verificar la ejecución y los sprites.
Los 169 descriptores literales del ejecutor de acciones también coinciden
con la ROM; 35 de ellos no estaban en la tabla generada anterior.
La selección dinámica de animación de `$79` pasó 7.611 estados cotejados y
37 pasos desde una captura real. La continuación con resolución, acción y
física intercaladas pasó otros 822 pasos para los doce personajes; sus 54
pares de personaje y pose visual tienen cotejo gráfico directo con la ROM.
Se verificaron
también 8.552 estados de `$1E`/`$1F` con los dos descriptores de liberación
de objeto y 58 pasos de continuación desde dos capturas TAG reales (29 por
selector). Las dos capturas y sus variantes controladas de los doce personajes
pasaron además 696 pasos con resolución, acción y física intercaladas;
sus 120 pares visuales, 60 por ruta, se cotejaron directamente con la ROM.
Esta prueba reveló y permitió corregir la condición de contacto central
del resolutor `$2851E`: la ROM solo marca contacto 4 si Y supera `$140`.
Pasaron 632 casos directos de ayudantes compartidos y la prueba local
`$F4C2` tras el arreglo. En `$6B`, 5.766 estados, incluidas seis capturas originales del
modo demostración, cubrieron los cuatro descriptores de caminar y 2.880
llegadas al reposo, sin diferencias con la ROM. Tres capturas continuaron
hasta el reposo con física intercalada en 36, 36 y 40 pasos.
Una captura real de `$2F` completó 162 pasos intercalados hasta el reposo
para el personaje 1; la variante del personaje 5 completó 125 y recorrió
el descriptor alternativo. Ambos son los ID incluidos para `$2F` en la matriz
de elección, y sus 14 pares visuales coinciden con el constructor ROM.
Las variantes controladas de los doce ID sumaron 1.537 pasos de acción,
resolución y física coincidentes. En los diez ID que no figuran en esa
matriz, 26 pares de pose carecen de definición gráfica en la propia ROM;
queda pendiente descartar su selección dinámica. El cotejo de las 252
filas de la matriz humana y 144 tablas normales de elección CPU encontró
`$2F` solo para los personajes 1 y 5 en la matriz humana y nunca en
esas tablas CPU.
La captura real de `$2E` llegó al reposo en 34 pasos con resolución,
acción y física intercaladas; las doce variantes controladas sumaron 408
pasos coincidentes. Solo los personajes 1, 5 y 9 tienen esta acción en la
matriz humana y en las 144 tablas normales CPU cotejadas; sus seis pares
visuales se verificaron contra la ROM. Las otras variantes forzadas
produjeron nueve pares de pose ausentes en la ROM. La prueba aislada
`$2E` pasó 3.874 casos y ahora termina limpiamente en Windows.
La captura real de `$2D` llegó al reposo tras 23 pasos intercalados;
las doce variantes controladas sumaron 276. Sus diez pares visuales
válidos para los personajes 0, 5, 6, 10 y 11 pasaron el cotejo gráfico.
La matriz humana de 252 filas y 144 tablas normales CPU restringen esa
acción a esos cinco personajes; las variantes forzadas de los demás
produjeron siete pares de pose sin gráfico en la ROM. La prueba aislada
de velocidades y contacto con cuerdas pasó 4.521 casos.
Se cotejaron
1.178 pares distintos personaje/pose de selectores y alternativas
posibles frente al constructor gráfico ROM, en ambos sentidos
(2.356 casos), con cero
bloqueos. La única pose ausente en el descriptor inicial, `$2F`/set 5, tiene
descriptor alternativo válido; sus 5.548 casos de rama volvieron a pasar.
La salida del ring `$68` pasó 8.071 cotejos con la ROM para las cuatro fases,
incluidas siete capturas reales del modo demostración. Cubrió sus cinco
descriptores y los relevos a `$69`, `$6A` y `$7A`. Desde una de esas capturas,
la ruta `$68`→`$69` atravesó 445 pasos de resolución, acción y física sin
diferencias, emitió los dos comandos de transferencia y llegó al reposo.
Al variar de forma controlada el personaje de esa captura para los doce sets,
la ruta TAG hasta reposo pasó otros 5.481 pasos. La bifurcación Royal hasta
`$7A` pasó 1.176 pasos; el rescate TAG hasta `$6A`, 180. La continuación de
ese rescate hasta `$6B` y reposo exterior pasó 1.511 pasos al reproducir el
cambio de modo de escena documentado por una segunda captura natural.
La retirada Royal `$7A` se continuó por separado desde una captura auténtica
del fotograma 2276: subestados 0→1→2→3→4→5 y borrado del actor en 233 pasos.
Las doce variantes de personaje sumaron 2.807 pasos, y otras doce con paleta
asignada otros 2.807; RAM, primer plano, registros, descriptores y liberación
de paleta coincidieron con la ROM. Las diez poses visuales usadas por esa
ruta se construyeron para los doce personajes en ambos sentidos: 240 cotejos
gráficos sin diferencias; con caché, las lecturas ROM pasaron de 5.220 a 690
en host. El rescate TAG se encadenó desde la acción
`$68` al gestor real `$F98C` y después a `$6A`→`$6B`→reposo. El cambio de
escena pasó 30 cotejos con la ROM y cinco guardias; en el relevo encadenado
coincidieron los 16 KiB de RAM y primer plano, y la continuación pasó 132
pasos para el personaje de origen y 1.601 para las doce variantes. Los demás
actores no corren durante los pasos aislados de acción. El callback real de liberación
de paleta pasó aparte 3.840 cotejos con el `$2B58` de la ROM. Ninguno de los
143 selectores cumple todavía
todos los criterios de certificación: `░░░░░░░░░░ 0/143 (0 %)`.
Las 26 poses de los seis descriptores `$68`/`$69`/`$6B` se construyeron para
los doce luchadores y ambos sentidos: 624 comparaciones gráficas con la ROM,
sin diferencias.
Las 904 poses gráficas enlazadas se compararon además directamente con la
ROM para los doce compañeros y ambos sentidos, en cuatro posiciones que
incluyen bordes y desbordamiento: 86.784 ejecuciones sin diferencias ni bloqueos.
Una caché adicional de 96 bytes para metadatos de set bajó las lecturas ROM
en caliente de 30.036 a 23.673 en esas 2.278 construcciones; eliminar una
segunda lectura de cabecera las redujo a 21.976, sin diferencias frente a la ROM.
Una caché directa de offsets de pose, de 192 bytes adicionales, las redujo a
18.860; en 21.696 construcciones enlazadas pasaron de 309.434 a 267.306.
Las tres mejoras compilaron para SH-2; aún no se enlazó
el juego ni se midió la velocidad en Saturn real.
Tras ampliar el puente de prueba para la retirada Royal, las 67 pruebas
intercaladas previas de acciones se repitieron y pasaron. El inventario
actual indexa aparte 93 informes de ring/escena para `$68`, `$69`, `$6A`,
`$6B` y `$7A`; esas cifras cuentan evidencias, no selectores terminados.
El compositor y la retención de tiles residentes eliminaron
sendos borrados de 1 KiB cada fotograma sin reservar otras tablas. Son avances parciales,
no una certificación del 100 %. Evidencia y límites:
`validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`.

**Resultado de Saturn real:** R02 queda en negro más de un minuto después de
Technos; los altavoces producen ruidos más tarde y no aparece diagnóstico.
No debe considerarse jugable. Para aislar la causa se ha preparado
`entregas/WrestleFest-Saturn-diagnostico-audio-20261026/`: conserva R02
salvo el firmware de música, sustituido por el de efectos. En Beetle Saturn
alcanza el título y la demo. El usuario confirmó que volvió a jugar con
normalidad en una Saturn NTSC real con MODE. Esta comparación apunta al
firmware de música como origen del bloqueo de R02, pero no identifica aún su
instrucción o transferencia causante. La música queda aplazada por petición
del usuario mientras se completan y cotejan acciones y animaciones con la ROM.
Los problemas anteriores de frontend y rendimiento aún requieren validación
separada.

La revisión R02 entregada para Saturn/MODE está en
`entregas/WrestleFest-Saturn-frontend-r02-20261026/`. Incluye audio SCSP
nativo, separación de las pilas SH-2, limpieza de escenas, y muestra la
pantalla antidroga antes de cargar el audio completo. En Mednafen la primera
placa aparece hacia VBlank 400 frente a 840 de R01. La imagen MODE1/2352
de 9588 sectores pasó EDC/ECC y cotejo CDFS exacto.

La prueba prolongada descubrió dos dependencias que detenían ciclos de la
demo: callback de caída `$1BBC6` y escena visual `$8490`. Ambas están
implantadas desde la ROM. La prueba focalizada de `$1BBC6` pasa y la última
emulación llegó a VBlank 56.982, atravesó `$8490` y siguió con
`fatal_owner=0`. La ausencia de bloqueo ilimitado y la fluidez real todavía
no están acreditadas.

La revisión física de los residuos entre antidroga, copyright, Technos y
selección de modo es el paso pendiente para cerrar el punto 2 indicado por
el usuario. El ritmo arcade de los retratos, la selección de modo/roster,
la entrada TAG y el logo de Demolition siguen pendientes. Ver
`validation/FRONTEND-GATING-20261026.md` para evidencias y límites. La nota
del 25 de octubre siguiente queda como historial.

# Estado anterior: fluidez, demostración, cuatro rutas de mando y Royal, 25 de octubre de 2026

La nueva intervención Royal, acción `$49`, reproduce 41 estados de la ROM
original con comparación de 16 KiB de RAM, 16 KiB de FG, registros D0-D7 y
descriptor de animación. Incluye inicio, espera de contacto, intervención,
liberación por distintas condiciones y estado ya completado. Su prueba
focalizada de la captura del frame 3.727 también pasa. El nuevo atlas Royal
reúne 7.407 imágenes, incluidas las 557 que faltaban en la ruta medida hasta
ese frame; deja 226 huecos para imágenes aún no observadas. El decodificador
del paquete pasa las pruebas normales y con AddressSanitizer/UBSan. En la
imagen stage9x, Royal continúa hasta el fotograma 4.500 sin bloqueo. En la
ruta medida, de 3.000 a 3.600 fotogramas la compilación anterior necesitó
1.170 ciclos de vídeo y 56 lecturas nuevas de disco; stage9x necesitó 720
ciclos y una lectura. Las trayectorias pueden divergir entre compilaciones,
de modo que es una comparación de ejecuciones, no de escenas idénticas.
Las 215 imágenes nuevas vistas después del antiguo bloqueo siguen usando la
caché de reserva; no se atribuye una tasa de 60 FPS sostenida ni una prueba
en Saturn física.

La prueba Royal ampliada llegó al fotograma 5.762 y señaló la acción `$42`
del luchador retenido. Se añadió la reacción y su liberación final: 30 estados
comparados con la ROM coinciden en RAM, FG, D0-D7 y descriptor. La prueba
focalizada que reproducía el bloqueo pasó tras el cambio. Queda por ejecutar
una nueva imagen completa para comprobar la continuidad desde ese punto.
Stage9y cruzó ese punto y llegó al fotograma 6.373. Allí la acción `$23`
alcanzó `$215B6`, la rutina original de pareja que ya se usaba en otras
acciones pero aún no estaba conectada en esta ruta. Su prueba focalizada
falló antes de conectar la rutina y ahora pasa. Queda la repetición larga
con una imagen nueva para verificar esta tercera transición.

La imagen de diagnóstico stage9v arranca el frontend completo y llega a TAG.
Mantiene 96 huecos para sprites nuevos y 48.172 bytes libres de HWRAM tras
añadir 433 imágenes observadas del combate. En una ruta TAG de 4.600 frames,
el emulador registró 363 lecturas de disco y 12.311 esperas de vídeo frente a
432 y 13.109 en stage9s. Los luchadores se separan después del frame 1.200,
por lo que esta comparación tardía indica mejora de la ruta, no un promedio
de escenas idénticas. No se acreditan 60 FPS sostenidos ni prueba Saturn/MODE.

La entrada por los dos conectores Saturn está conectada a los puertos arcade
1 y 2. Con crédito y START del segundo mando durante la selección original,
el emulador inicia combate con actor0 ligado a `$140020` y actor1 a `$140022`.
El ruteo de multitap hacia jugadores 3 y 4 tiene prueba lógica, aún no prueba
física. Se conserva INSERT COIN en pantalla según la petición del usuario.

La siguiente demostración automática se inició tras terminar la primera sin
bloqueo en stage9u. Se añadieron las ramas originales de CPU `$1CFFA` y AE20
programas 1/2, con pruebas focalizadas. Stage9v ganó TAG naturalmente en
frame 10.205 con IDs humanos 0/7, mostró la escena de Legion of Doom/Hogan y
continuó hasta frame 300 del siguiente combate, world 5/progreso 1 y rivales
IDs 3/6. Los dos handoffs F8C y el resultado humano `$4001/$4000` son lecturas
reales del emulador sin imponer estado. Royal destapó en frame 3.726 la defensa
CPU `$1F5DA`; la continuación original selector `$49`→`$1F45A` pasa la prueba
focalizada y 2.230 comparaciones con ROM de RAM/FG/D0–D7/RNG. Está pendiente
la repetición larga con la nueva compilación.
La inversión
física del mando reportada en stage9b requiere repetir la prueba en Saturn con
una imagen futura; la ruta emulada actual mueve derecha a la derecha y arriba
hacia arriba. Los residuos de las pantallas de arranque, capas de cuerdas y
fluidez de la presentadora también esperan comprobación física. No entregar
otro BIN/CUE hasta cerrar los problemas pendientes y seguir implementando
movimientos y situaciones.

# Estado anterior: más movimientos y ciclo TAG, 24 de octubre de 2026

Objetivo: frontend desde anti-drogas, selección, entradas/presentación,
combate libre, victoria TAG, escena original y siguiente ring. El usuario
pide más movimientos antes de otra entrega BIN/CUE; la entrega del23Oct
permanece intacta. Destino: Saturn americana NTSC-U60Hz con Terraonion MODE.

Checkpoint stage9b: victoria humana HOST10902 con sólo botones normales,
policy4,8,4,8,4,0,1, winners3/result4001/4000. Original11B6→B608/BDA6/AE20→
AC0 tras455IRQs, progreso1→F8C world5, roster0/7/1/2, seed6DF2B11D.
Segundo combate1000frames con MAP5/cámara5, seed4ED28511, HP74/30/128/130.
Repetido tras completar todos los nuevos owners, sin tocar HP/RNG/resultados.
48 CORE de la copia final congelada PASS.
ASan/UBSan/leaks stage9b PASS: todos los TUs recompilados, WIN10902/post/segundo1000 y fixtures explícitas4/9.
Beetle stage9b principal4/8/8 (human-tag4,8,4,8,4,0,1, 212.094s): Combate nativo completado LOSS en3566, resultado/transporte terminal válido; guarda fase0019E2/dependencia0065C0, último frame de combate3566. No se acredita victoria/segundo ring nativo completo.
Beetle stage9b segunda3/4/24 (human-tag3,4,2,24,4,0,1, 576.906s): Combate nativo completado LOSS en9327, resultado/transporte terminal válido; guarda fase0019E2/dependencia0065C0, último frame de combate9327. No se acredita victoria/segundo ring nativo completo.
Beetle stage9b tercera3/8/24 (human-tag3,8,4,24,4,0,1, 530.984s): siguiente combate observado frame300/world5; victoria humana, progreso1, identidades0/7 y handoffs0→5 verificados.

Nuevos movimientos comparados con ROM intacta, sin helpers interceptados:
- Raw0B: salto/contacto,2327+61 y repetición independiente2327+61 PASS;
  resuelve el guard nativo stage9/8798. Captura9actores/D8 exacta.
- Raw20: animación/desplazamiento con NOP inicial,2422+1 y revisión726
  observaciones directas PASS; BPL usa N, no signed-overflow inventado.
- Raw54: agarre/liberación,4310+73 y revisión2520+27 PASS.
- Raw62: acción enlazada,4101+23/360quanta y revisión3475+271 PASS.
- Víctima1BD0E: reacción del agarre54 añadida tras guard HOST6733;
  matriz3397+63 PASS. Revisión independiente se conserva por separado.
Acciones A061EBCDEB68D4702BCAE734AAE390D05D0C96D1E7AA99E7766D058FB4807FEE;
dispatcher9BB5615B720983839803C862951E6BEB19202711FE19244B4EA01C9BF7E59A1D.
Las demás guardas siguen explícitas hasta portar y probar sus owners.

El mando físico514E usa la fuente+8A de cada actor, que TAG intercambia;
adaptadores nativo/HOST4105controles cada uno y revisión ROM2661+9 PASS.
El controlador de pruebas emulado ahora coincide con el cuerpo C en48576
vectores, incluido B ante rivales caídos. Un fallo independiente descubrió
que strategy4 alternabaA/B también ante caídos; RED conservado y corregido.
Esa variante defectuosa nunca se ejecutó en Saturn. Reutilización ASAN
descartada tras revisión:9b recompila todo desde el archivo de fuentes.

Presentadora verificada en el vídeo nativo stage9:80s señala a Hogan que
saluda,94s a Warrior levantando brazos,96/103s abandona el ring antes de
luchar. Audio silencioso, sin promesa de voz. Primera victoria original:
LoD/Hogan/tablero; AE20(3) oculta objeto1282. El presentador AE20(0)
interviene antes de los retos por título, progreso4/9. Fixtures4/9 llegan
F8C con driverAE20/B2E4 real, pero no son victorias naturales de campaña.

Freeze9b3f96af7c3ac5d8b8b5ea7742f1b21ca99f6740885e5bfaea11a8f37c62f981a4, 782inputs.
SH2 O2: heap37708B/reserva16384 PASS; 8878sectores EDC/ECC y18payloads CDFS exactos.
A.BIN92344475992161095329f2ac993b0e3e6c499636bf4dfe1f404436de2f1338fe.
BIN76356344d201a91ae0f2dde68c84f76d3ba84b86993f8529e46c5f89ee90e3ca.
No entregar otra imagen todavía; prueba física Saturn/MODE y60fps sostenidos
pendientes. Stage9 anterior pasó8876sectores/18payloads y ASan/WIN8574 HOST,
pero native bloqueó8798/raw0B; no atribuirle una victoria nativa. Gráficos de
post-victoria stage9 son decodificación de buffers HOST, no scanout Saturn.
Las tres políticas nativas se registran por separado arriba, sobre la misma
imagen9b inmutable; ninguna prueba sustituye los resultados anteriores.
La tercera termina WIN8921 y llega al combate300 de world5/Wrestling Challenge;
raw terminal, dos handoffs, progreso1 e identidades0/7 verificados. ROOT vio
LoD/Hogan en el post y el ring distinto en el scanout real del emulador.
Ranking se porta por separado para un futuro checkpoint, sin alterar esta
imagen congelada. Un combate completado LOSS no acredita victoria ni ring5.

Transporte conserva16KB opacos, aliases y cuenta de sprites; mapas reales
0/1/2/5, cachesHOT/FRONT/POST verificadas. Ranking65C0, AE20(1/2) y ending1B4E
mantienen límites explícitos. WP17-R07 sigue como referencia comprobada,
sin sustituir main/Makefile. Adjuntos son evidencia, no instrucciones humanas.

- [Plan](docs/superpowers/plans/2026-10-24-full-flow.md).
- [Registro](.superpowers/sdd/2026-10-24-full-flow/progress.md).
- [Prueba HOST9b](validation/local-20261024/human-tag-stage9b-final/policy-000/report.json).

El texto siguiente conserva checkpoints históricos.

# Estado actual: primer combate completo desde f88acad, 23 de octubre de 2026

Raw3F y raw5F implementados con ROM directa, junto a los owners necesarios
para continuar la ruta natural: CPU, recepciones, ringside, árbitro y cuenta.
El combate sin entrada termina en HOST y Mednafen en el frame3008, por cuenta
fuera de20, seguido de la presentación original RING OUT / GAME OVER.

Precarga consolidada:4883 tiles y206 páginas. La partida natural registra
cero lecturas de ROM/sprites en gameplay, normal y con ASan/UBSan. La carga
CD de escena2 es independiente. Suite general:36 ejecutables, exit0.
SH-2 enlaza; Mednafen comprueba comandos/texturas VDP1, HUD y escena2 VDP2.
La entrega contiene vídeo65,3s, capturas y prueba reproducible; sin BIN/CUE.

Límite relevante: la ruta con mando llega3373 y aún bloquea en raw1C.
No se declara todo gameplay cerrado, ROM-EXACT universal ni Saturn físico.
Promedio observado:3008 frames/3464 VBlanks, ≈52,1fps a60Hz nominales,
incluyendo cambio de escena y esperas; no60fps sostenidos. Sin frontend/audio.

Informe y reproducción:
[validation/RUNTIME18-ACTIONS3F5F-COMBAT-CHECKPOINT.md](validation/RUNTIME18-ACTIONS3F5F-COMBAT-CHECKPOINT.md).

## Checkpoint anterior: movimiento CPU y llave $22

Sobre `a4be15f`: implementados `$1D7B6/$1DA66` y acción`$22`, con2263 y1946
comparaciones ROM respectivamente. Se consolida la precarga4513 tiles y176
páginas; cero lecturas CD en los recorridos HOST probados703/560 frames.
Mednafen verifica movimiento del aliado en700 y llave`4022` en550, comandos
VDP1 y HUD VDP2 reales. Se entrega vídeo y capturas, sin otro paquete BIN/CUE.

La partida sigue incompleta: los nuevos primeros bloqueos son acción`$3F`
en703 sin entrada y`$5F` en560 con control. Perfil450→550:100 frames/111
VBlanks, aproximadamente54fps en ese tramo; no60fps sostenidos, ROM-EXACT
universal ni validación Saturn físico. No se retoma el frontend.

Informe y reproducción:
[validation/RUNTIME18-MOVEMENT-ACTION22-CHECKPOINT.md](validation/RUNTIME18-MOVEMENT-ACTION22-CHECKPOINT.md).

## Investigación anterior: HUD, aliados y stamina

Sobre `95328a2`: corregido el origen de relleno de las barras, preservando
ambos retratos; comprobado en capturas reales de Mednafen. Se corrigen
también mantenimiento de daño/tier, distancias CPU que no se refrescaban,
registros de seguimiento y poses retenidas. La distancia falsa cero podía
provocar puñetazos lejanos; el caso ROM654 ahora pasa a movimiento.

La partida corregida aún está incompleta: sin entrada bloquea en669
`$1D7B6`, y la secuencia controlada en460 `$12614`. Los finales4391/14409
del checkpoint anterior no se atribuyen al nuevo árbol. No se ocultan esos
bloqueos. Entrega de esta investigación: capturas, sin otro paquete BIN/CUE.

Informe y reproducción:
[validation/RUNTIME18-HUD-ALLIES-INVESTIGATION.md](validation/RUNTIME18-HUD-ALLIES-INVESTIGATION.md).

## Checkpoint anterior: sprites nativos y escrituras del ring

Continúa desde `64d93b1`: caché de 32 cuerpos, comandos VDP1 sin clears
redundantes, recorte de filas transparentes y máscaras rápidas para SH-2.
El frame 120 conserva sus píxeles y reduce 1840 → 1474 filas VDP1.
CPU −6,9 %; la cadencia inicial solo mejora 99 → 98 VBlanks/90 frames.
El tramo avanzado conserva 198/180. Partida completa: 4887 VBlanks frente
a 4919, con 4391 frames. No son 60 FPS sostenidos.

Se conecta el parche original `$298EC` a VDP2 con 96 casos ROM. La apertura
de cuerdas todavía no se ha activado jugando en Mednafen; la comparación
se identifica como software. Se libera HWRAM manteniendo 4096 escrituras
en una zona separada de LWRAM.

Informe, límites, reproducción y evidencia:
[validation/RUNTIME18-SPRITE-WORK-CHECKPOINT.md](validation/RUNTIME18-SPRITE-WORK-CHECKPOINT.md).
El checkpoint incluye BIN/CUE directo, sin frontend ni audio. Saturn físico
con CD-R queda pendiente. Persisten rutas de acciones bloqueadas; no se
sustituyeron por éxito provisional.

## Registro histórico de RUNTIME-17

El texto siguiente describe exclusivamente el cierre RUNTIME-17.

# RUNTIME-17 — checkpoint verificado R02, 18 de octubre de 2026

Cerrados los pendientes de este checkpoint: owners nativos $2017A, $6E34 y $9132, integración $18C4, composición de registros de un frame activo y compilación/enlace SH-2. No se ha iniciado RUNTIME-18. Este cierre no declara equivalencia ROM-EXACT universal ni aptitud para hardware.

## Procedencia y cambios

Base GitHub exacta b85df2d23e4c64ecf24b5d2d41c4dc7b9000189e; recuperación anterior a35ffa429037986f55f09254106e0400c04427fe. Rama local recovery/runtime17-participants-20261018. El commit final consta en CHECKPOINT.txt del paquete. No hay cambios publicados en GitHub. Se conserva la historia recuperada con su límite shallow.

La ROM autoridad mantiene SHA1 a726c46f350723ea73158bc78055d17d418b14e2. TAG $6E34 y Royal $9132 se implementan en C nativo; los 22 helpers compartidos tienen generación reproducible y auditoría de fuente contra la ROM. No se enlaza un emulador con el runtime. Los emuladores MC68000 se usan exclusivamente como oráculos HOST independientes.

$18C4 usa los owners nativos por defecto, lee entradas actuales y detiene el recorrido después del primer pago. Las operaciones de vídeo se preparan antes del commit; un fallo conserva el estado de gameplay. Los callbacks de transporte no sustituyen decisiones de gameplay. Se corrigieron omisiones de anchura y preservación de registros en input, cámara, iteradores, árbitro, raw $7C, $F560 y $7A00. El ACK sincroniza los alias del contador de frames y el selector A/B.

## Evidencia y alcance

- 48 tests HOST con Clang, 48 con ASan/UBSan y 48 con GCC Linux; análisis GCC -fanalyzer de las fuentes.
- TAG: 28 casos contra ROM original, RAM/actores, D0–D7, FG0/paletas y orden de transferencias por canal.
- Royal: 45 casos contra ROM original en Musashi, los 22 helpers alcanzados, RAM/FG0, D0–D7/A0–A6 y orden de escrituras.
- Frame activo: ejecución real de $F9A a $10E8; D0–D7 y RNG coinciden en los 29 puntos de salida y tras ACK. Actor visible con acción $7C, movimiento $0200→$01E0, árbitro, objetos fijos y TAG nativo. Se verifica el selector de la siguiente rama B, no un segundo frame completo.
- Contrato de registros del render $D1FC: 144 comparaciones ROM para records simples/compactados sin enlaces y 18 rechazos de casos fuera de ese contrato. El adaptador del test lee records reales; no reemplaza registros por tuples esperados. No es un renderizador Saturn completo.
- Cuatro regresiones ROM adicionales verifican midpoint del árbitro y pago/refill, incluidos éxito y fallo.
- 14 auditorías estáticas de ROM y reproducción byte a byte de la fuente generada.
- Enlace limpio SH-2 GCC 13.2.0 con cero símbolos sin resolver. El SDK conserva avisos de segmentos RWX. El ELF es un harness de enlace, no un combate arrancable.

Los resultados completos y hashes están en evidence/ y SHA256SUMS del paquete. validation/README.md permite repetir las pruebas con la ROM y el SDK originales.

## Lo que no acredita este cierre

No se han probado igualdad de framebuffer, todos los records de sprites, todas las acciones/ramas de gameplay, timing, ni un combate completo en Saturn. Los cambios de escena y otras dependencias externas conservan su bloqueo explícito cuando falta una implementación; no se sustituyen por éxito provisional. Los dobles de callbacks que quedan en tests históricos solo prueban contratos aislados.

No hay BIN/CUE ni disco jugable. Pruebas de emulador Saturn y Saturn real con CD-R: NO EJECUTADAS. No usar el harness como imagen para grabar. RUNTIME-18 permanece sin comenzar hasta una nueva instrucción.

La documentación histórica describe estados anteriores. R02 documenta el cierre RUNTIME-17; para el estado actual prevalece la cabecera RUNTIME-18 de este archivo.
# Evaluación R05 entregada el 28-10-2026

- Imagen solicitada para Saturn real: `entregas/WrestleFest-Saturn-evaluacion-R05-20261028.zip`; incluye BIN, CUE, instrucciones y hashes.
- Comparada con la imagen de aislamiento SFX que probó el usuario: el único archivo interno con contenido distinto es `A.BIN`; el firmware de efectos coincide. Comprobación de sectores y archivos en `validation/local-20261028/eval-r05-build/comparison-isolation.json`.
- Compilación SH-2, arranque hasta título y recorrido a combates TAG y Royal en Beetle Saturn. En Royal se observaron periodos negros transitorios antes de continuar; sin prueba en consola real ni medición real de FPS. Los controles no quedaron certificados por la prueba automatizada.
- El léeme distingue correcciones y optimizaciones del ejecutable frente al trabajo de comprobación de acciones; no afirma nuevas acciones respecto a la imagen anterior ni resolución completa del frontend. Siguen 0/143 selectores certificados integralmente.

