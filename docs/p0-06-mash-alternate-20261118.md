# WrestleFest Saturn — P0-06: MASH, control alternativo y validación del personaje

**Fecha:** 18/11/2026. **Estado:** candidato acumulativo de fuentes P0-01 a P0-06. No es una BIN/CUE, no sustituye a FIX2 y no acredita el port completo en Saturn.

## Base comprobada

Se ha abierto el paquete exacto `WrestleFest_P0_05_Relaciones_Fuentes_y_Pruebas_20261117.zip`, SHA-256 `fc987e8f2aa0c489e670b38eb1dff49b5bd47c341dafc503ca0d2ba7d9fde727`. Su CRC y sus **1.365 identidades** se verificaron sin diferencias. Base Git: `d05231268374e328f0f9abe631996a0ee6bf829b`. Trabajo en una copia local aislada, rama `p0-06-mash-alternate`, sin publicación remota.

La ROM se reconstruyó de los chips pares `31e14-0.ic18` e impares `31e13-0.ic19` del `wwfwfest.zip` proporcionado. SHA-1 `a726c46f350723ea73158bc78055d17d418b14e2`; SHA-256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`. Los tests rechazan otra ROM. Sus bytes no se modifican ni se incluyen en este paquete.

Antes de modificar producción se ejecutó la matriz directa anterior de P0-05: **4.447/4.447**.

## 1. Finalización del contador MASH: `$EBC4 / $EBF6`

**Defecto reproducido:** al alcanzar exactamente `$4000` en el contador `+$AA`, el port rechazaba una referencia de objetivo cuya dirección física era cero. También rechazaba referencias a actores válidos con etiqueta en el byte alto. Esto detenía la selección antes de ejecutar las continuaciones originales.

La ROM hace una lectura por el bus de 24 bits. En la dirección física cero, el byte `$34` vale `$FF`: su bit 4 está activo. Por tanto, se omiten las dos ramas especiales del estado 4, pero **se continúa con la tabla de finalizaciones del estado 5**. Omitir toda la rutina al ver cero sería incorrecto.

Ahora se conserva esa distinción: una referencia cero no crea un actor y no da lugar a una escritura sobre él. Los actores reales siguen la ruta anterior. Las referencias no nulas que no corresponden a una dirección admitida continúan devolviendo `WF_BLOCKED` con dependencia `$EBF6`.

Se mantienen sin cambios el umbral exacto, la prioridad de las acciones `$4A/$51`, el borrado del contador únicamente al consumir la acción y estas transiciones de la ROM:

| Entrada | Salida |
|---|---|
| `$4A`, `$51`, `$64` | `$4B` |
| `$5D`, `$5E`, `$5F` | `$67`, `$56`, `$5C` |
| `$60`, `$61`, `$62`, `$63` | `$5B`, `$59`, `$5A`, `$55` |
| `$52`, `$53` | `$58`, `$57` |

Referencia en el candidato: `core/src/wf_r17_human_raw23.c:142–148`. El resto de la rutina y su tabla ya existían.

## 2. Clasificador alternativo `$E496`

Esta ruta se toma con `FLAGS32.bit0` y acepta el estado bajo 1 con `AF=1`. Selecciona la categoría `$13` y llama a `$F178`.

**Defecto reproducido:** se detenía con un objetivo de dirección física cero. Además, resolver un puntero solo por su dirección física no basta: la escritura del enlace debe conservar los **32 bits originales**.

La corrección respeta el orden de `$F178`: primero escribe el enlace propio, después comprueba el enlace recíproco y solo lo escribe si estaba vacío. Esto también se prueba con el propio actor como objetivo. Para la dirección cero, ROM[`$26..$29`] vale `$FFFFFFFF`, por lo que no se produce una escritura recíproca. Los filtros posteriores usan la vista de lectura ROM existente; no se asigna un rival ficticio.

Las condiciones de entrada, la categoría, las tablas y las rutas de no aceptación se conservan. Referencia: `core/src/wf_r17_human_raw23.c:445–456`.

## 3. Fallo adicional: truncado del identificador de personaje

Al comprobar las fronteras de `$E496` apareció un defecto independiente: el índice de personaje se leía como WORD, pero se convertía a BYTE **antes** de validar el límite. Así, por ejemplo, `$0100` se convertía en `$00` y se aceptaba como otro personaje.

Después de corregir las dos continuaciones, seis casos de esta guarda seguían fallando. Se mantuvo esa prueba en rojo y se corrigió el tipo de la variable y su lectura. Ahora se valida la palabra completa antes de acceder a la tabla. Es una protección frente a estados inválidos; no se afirma que el usuario haya sufrido una corrupción con ese valor.

Referencias: `core/src/wf_r17_human_raw23.c:677,728–730`. Los prefijos de escritura del enlace que ya se habían ejecutado se conservan al comunicar el error.

## 4. Alcance mínimo del cambio

Solo cambia **un archivo de producción**: `core/src/wf_r17_human_raw23.c`, con 18 líneas añadidas y ocho retiradas respecto a P0-05. Se conservan los finales de línea CRLF originales. Los otros **438 de 439 archivos versionados** de `core/`, `port/` y `sdk/` mantienen sus bytes.

No se cambian gráficos, audio, tablas de acciones, probabilidades, reloj, dificultad ni el resto de las correcciones P0-01–05. No se incorpora un emulador al juego Saturn. La protección sonora y las correcciones del árbitro anteriores permanecen en esta línea de fuentes.

## 5. Pruebas ejecutadas de nuevo

| Control | Resultado |
|---|---|
| Matriz directa, GCC, AddressSanitizer y UndefinedBehaviorSanitizer | **3.435/3.435** |
| Misma matriz, Clang, ambos sanitizadores | **3.435/3.435**, salida idéntica a GCC |
| API directa sin solicitar traza | **3.435/3.435** |
| Fase real `$F18A`, con el núcleo de producción instrumentado | **3.435/3.435** |
| Mutaciones deliberadas compiladas y ejecutadas | **12/12 detectadas** |
| Compilación y enlace host con GCC | 117 unidades del núcleo; 127 objetos; sondas TAG, Royal y frontend enlazadas |

Se comparan los **16 KiB de RAM y los ocho registros D completos**, no solo el valor de retorno. Se exige que la referencia haya ejecutado las direcciones correspondientes a cada caso. Las finalizaciones MASH incluyen además resultados literales deducidos de los bytes de la ROM, independientes del código C.

La matriz directa contiene **3.390 comparaciones con instrucciones ROM y 45 controles de rechazo nativo**. La integrada contiene **3.384 comparaciones ROM y 51 controles de rechazo**. Los controles de rechazo no se cuentan como acciones legales del arcade.

Con el archivo de producción P0-05 reinstalado en las mismas pruebas integradas, **2.727 casos fallan**: 2.712 continuaciones ordinarias se detienen y 15 casos de índice inválido no alcanzan la guarda esperada. Se conserva esa ejecución negativa. Son estados sintéticos controlados, no miles de partidas ni una reconstrucción de Hogan/Warrior contra Legion of Doom.

SHA-256 común de la salida directa GCC/Clang: `c919f43865630770edd218b33067cebf8e3e334672ffb5a38ccc0302f13fafca`.

Las doce mutaciones alteran, por separado: máscara de dirección MASH, continuación con cero, bit 4 de la lectura cero, umbral, borrado del contador, conservación de la etiqueta del enlace, escritura del enlace, condición de reciprocidad, categoría, condición AF, rechazo de direcciones inválidas y truncado del índice. Ningún error de compilación se cuenta como mutación detectada.

### Regresión de los bloques anteriores

Los nueve comandos de regresión seleccionados terminaron con código cero: P0-01 (12 grupos), P0-02 directo (777 casos) y fase/paridad, P0-03 directo (5.589) y fase (229), P0-04 directo (3.411) y fase (1.092), P0-05 directo (4.447) y fase (4.191). Los informes, comandos y registros de estas ejecuciones nuevas se incluyen.

## 6. Suite global: sigue sin superar la aceptación

Se ejecutó `tests/run_initial_load_tests.py --assets cd`. Resultado: **11 grupos con salida cero y ocho fallidos**, código global 1, sin deriva de los archivos vigilados durante la ejecución.

Los ocho grupos fallidos son `raw-metadata`, `raw-packaging`, `music-api`, `music-generation`, `music-order`, `audio-service`, `audio-decoder` y `combat-audio`. La carpeta `cd/` solo dispone de la ROM de programa verificada; faltan, entre otros, `SPRITES.WFP`, las pistas `MU*.WAV` y `SFX.DAT`. Algunos tests abortan al abrir los archivos ausentes. No se han sustituido por datos ficticios para conseguir un PASS.

Los dos controles negativos de metadatos devuelven cero, pero con recursos ausentes no certifican aisladamente las alteraciones que pretendían probar. Los 11 grupos no se presentan como una aprobación de la imagen de disco.

## 7. Límites y problemas del proceso conservados

La referencia sigue siendo el **intérprete limitado heredado de pruebas**, sin cambios de instrucciones en este bloque. No es un contraste independiente con un motor 68000 completo. La instalación de una dependencia externa falló por resolución de red; también fallaron los intentos de descarga. No se declara esa puerta superada.

No se comparan todos los registros de dirección, el SR/X completo, excepciones, interrupciones ni ciclos del 68000. No se ha medido rendimiento, memoria libre o estabilidad de partidas completas en Saturn.

La primera matriz integrada incluía seis entradas deliberadamente inválidas con `AF=$FF`: provocaban un índice 63 fuera de la tabla original de estado 1. Se corrigió **la clasificación del test**, no la rutina: siguen presentes como controles de la guarda `$F25A`, comprobando también D1. El informe del primer intento se conserva; no se presenta como una ejecución pasada.

La combinación inicial de compilación completa y prueba integrada agotó el límite de ejecución del comando. La compilación sí terminó; la prueba se ejecutó después por separado hasta su resultado completo. Un primer recorrido de mutaciones también quedó interrumpido. Las cifras de aceptación corresponden exclusivamente al segundo recorrido completo y a las ejecuciones finales.

Se revisaron en esta sesión el diff, el orden de escrituras, las fronteras de tabla, los rechazos, las referencias ROM y las pruebas de mutación. **No hubo un revisor independiente.**

Se volvió a intentar localizar FIX2 y el SDK completo en los archivos accesibles. Las búsquedas no devolvieron resultados y la materialización del archivo Jo Engine previamente identificado no autorizó el acceso a sus bytes. Esto no demuestra que no existan en la biblioteca del usuario; significa que no se han recuperado en esta sesión.

## 8. Reproducción y recuperación

Desde `source/`, con Python 3 y GCC/Clang, proporcionar la ROM exacta en `source_assets/ROM68K.BIN`. El test heredado del árbitro puede reconstruirla desde el ZIP del usuario:

```sh
python tests/test_referee_p0_02.py --rom-zip /ruta/wwfwfest.zip --sanitize --output build/rom-check
python tests/test_human_p0_06.py --output build/p006-direct
CC=clang python tests/test_human_p0_06.py --output build/p006-clang
python tests/test_human_p0_06.py --no-trace --output build/p006-no-trace
WF_HOST_CC=gcc CC="$PWD/tests/p0_05/cc_sanitize.sh" \
  python tools/build_runtime_tests.py --build-dir build/p006-native
python tests/test_human_p0_06.py --build-dir build/p006-native --output build/p006-phase
python tests/test_human_p0_06_mutants.py --output build/p006-mutants
```

Para repetir el control negativo, usar `--source ../evidence/human-p005.c`. El resultado esperado de ese control es FAIL. El paquete incluye el parche incremental sobre P0-05, el acumulativo sobre la instantánea R03 y un bundle Git con historial. El nombre exacto de la rama y el commit se indican en `MANIFEST.json`.

La siguiente prioridad es consolidar estas fuentes con **las fuentes y recursos exactos de FIX2**, recuperar el compilador SH-2, comprobar el enlace y su margen de memoria, y ejecutar las pruebas en emulador Saturn y consola. Estos dos cierres de rutas no prueban que ya no queden otras dependencias de gameplay abiertas. No hay nueva imagen para grabar en CD-R.
