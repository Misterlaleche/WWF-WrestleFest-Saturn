# WWF WrestleFest para Sega Saturn

[English](README.md) · **Español**

Port nativo de **WWF WrestleFest**, de Technos, para Sega Saturn. El objetivo es conservar los gráficos, las reglas, la respuesta del mando, los tiempos y el sonido del arcade en la consola. El proyecto sigue en desarrollo; la equivalencia con el original continúa siendo el criterio de aceptación.

**Fuentes actuales: P1-03B BOOTFIX CANDIDATE, del 22 de noviembre de 2026.** El repositorio se renovó el **25 de noviembre de 2026** a partir del paquete de fuentes más reciente recuperado y verificado para esta actualización. La corrección de arranque aún necesita confirmación en Saturn física. Esta actualización del repositorio no certifica un juego terminado ni una nueva versión probada en hardware.

[Compilación](docs/BUILDING.es.md) · [Estado y pruebas](docs/STATUS.md) · [Procedencia de las fuentes](docs/PROVENANCE.md) · [Historial](docs/HISTORY.md) · [Avisos de terceros](docs/THIRD-PARTY.md)

## Qué contiene

El repositorio reúne ahora un único árbol de fuentes integrado: núcleo nativo de juego, código de plataforma Saturn, Jo Engine adaptado, herramientas de recursos, pruebas de regresión y evidencias compactas de desarrollo. Las antiguas versiones independientes de escenas, TAG, Royal y audio siguen accesibles en el [historial de Git](docs/HISTORY.md).

| Componente | Implementación actual |
| --- | --- |
| Destino | Sega Saturn NTSC, **expansión de RAM de 4 MB** y Terraonion MODE como configuración principal de pruebas físicas |
| SDK | **Jo Engine 2026.3**, conservando las adaptaciones del proyecto |
| Modo de vídeo | **320 × 240** |
| Lógica de juego | Reconstrucción nativa en C del comportamiento del 68000 del arcade, manteniendo sus estados y semántica de registros cuando corresponde |
| Gráficos | Sprites y envío de comandos mediante VDP1; fondos, ring y capas de escena mediante VDP2 |
| Reparto de CPU | SH-2 principal para juego y preparación de fotogramas; trabajo del segundo SH-2 mediante las colas de comandos y descompresión de Jo |
| Audio | Reproducción PCM en SCSP: 18 músicas leídas de archivos de datos, más efectos y voces decodificados |
| Resultado de compilación | `game.bin` y `game.cue`, además del ELF, ejecutable raw e ISO intermedios |

La compilación actual utiliza Jo Engine. El trabajo anterior con Yaul forma parte del historial y de algunas referencias conservadas; no es el SDK enlazado por el `makefile` actual. Esta revisión necesita la expansión de 4 MB, aunque parte del trabajo anterior tuviera como objetivo una Saturn sin ampliación.

## Últimos avances incluidos

P1-03B conserva el trabajo anterior P0-01–P0-06 de juego y diagnóstico, GFX1 y las actualizaciones P1. Los informes fechados de [`docs/`](docs/) explican el alcance y las evidencias de cada bloque.

- **Diagnóstico de arranque y corrección candidata.** La prueba física de la versión anterior se detenía después de leer por completo `SPRITES.WFP`, con el owner `0018CD10`. P1-03B devuelve la rutina Adler-32 de SH-2 a la compilación habitual con `-O2` y añade fases detalladas al diagnóstico de recursos. La causa sospechada está documentada; el arranque correcto de esta candidata sigue sin confirmarse.
- **Preparación musical.** Las 18 músicas se regeneraron desde la implementación de sonido de referencia, aplicando conversión limitada en banda de 176,4 kHz a 44,1 kHz. Esto corrige el aliasing introducido por la decimación simple anterior.
- **Fundido musical.** El comando `0x1F` aplica ahora un fundido de volumen de cinco segundos en SCSP y después detiene la voz. Es una aproximación implementada; la equivalencia exacta con la envolvente del driver arcade sigue abierta.
- **Presupuesto de memoria.** Determinado código frío se optimiza por tamaño, conservando la política necesaria en las rutas activas. El mapa recuperado de P1-03B registra **8.208 bytes** antes de la reserva `0x060DC000`, frente a un mínimo de 8.192. Es el margen documentado de aquella compilación, no una medición nueva del rendimiento de la consola.
- **Evidencia de progresión TAG.** La regresión recuperada llega al segundo combate mediante pulsaciones ordinarias del controlador de pruebas. Este resultado no cierra la aceptación del recorrido completo de TAG ni de Royal.

Consulta el [informe BOOTFIX](docs/p1-03b-bootfix-20261122.md), el [informe de audio P1-03](docs/p1-03-audio-bandlimit-and-fade-20261121.md) y la [corrección del diagnóstico de progresión TAG](docs/p1-01-tag-progression-correction-20261121.md).

## Por qué requiere más que trasladar gráficos

El comportamiento del arcade debe mantenerse al pasar de una placa basada en 68000 a los procesadores SH-2 y las memorias separadas de gráficos y sonido de Saturn. El orden de los actores, las colisiones, la lectura del mando, el estado aleatorio, las transiciones y las órdenes de sonido deben coincidir con el original. La ROM original y las trazas auditadas son la referencia de comportamiento; una aproximación cómoda no demuestra equivalencia.

Además, todo debe caber en los presupuestos de tiempo y memoria de Saturn. Los sprites se empaquetan y almacenan en caché, las tablas de comandos tienen que seguir siendo válidas hasta que VDP1 las consuma y la música debe seguir recibiendo datos mientras avanzan el juego y la descompresión. Las pruebas de host, las comparaciones diferenciales y Mednafen ayudan a detectar errores. Las pruebas físicas siguen siendo necesarias para validar el arranque, el lector, la cadencia sostenida, la continuidad del audio y la imagen final.

## Compilar tu propia copia

Empieza por la [guía en español](docs/BUILDING.es.md) o la [guía en inglés](docs/BUILDING.md). El entorno documentado utiliza GNU/Linux x86-64, GNU Make, Python 3 y el GCC SH-2 8.2 fijado por el paquete de Jo Engine 2026.3.

Hay que aportar dos entradas externas:

1. El archivo exacto del SDK Jo Engine que acepta [`tools/setup_compiler.py`](tools/setup_compiler.py).
2. Un disco P1-03/P1-03B compatible o recursos preparados que satisfagan [`asset-lock.json`](asset-lock.json).

El manifiesto actual describe **34 archivos que suman 632.644.966 bytes**. Aquí no se distribuyen ROM, recursos gráficos o sonoros, BIOS, imágenes de disco ni ejecutables del compilador. Disponer de cualquier `wwfwfest.zip` no convierte por sí solo este repositorio en una cadena completa de generación de recursos mediante un único comando: parte de los originales de generación es externa. El procedimiento documentado recupera los recursos preparados y verificados de un disco compatible.

Se incluye el directorio adaptado [`sdk/Compiler/COMMON/`](sdk/Compiler/COMMON/). Faltaba en los ZIP de fuentes recientes y se ha restaurado desde la configuración heredada y verificada de R03. No lo sustituyas, ni sustituyas [`sdk/jo_engine/`](sdk/jo_engine/), por las versiones sin modificar del SDK.

## Controles

Para la compilación con el frontend original (`WF_FRONTEND=1`):

| Mando de Saturn | Acción |
| --- | --- |
| Cruceta | Movimiento y selección en los menús |
| A | Entrada A original / puñetazo |
| B | Entrada B original / patada |
| C | Introducir un crédito; pulsar y soltar |
| START | Empezar o incorporarse cuando lo permitan el estado del juego y los créditos |

El adaptador también contiene rutas multijugador y sus pruebas. Su presencia no certifica todas las combinaciones en hardware. El selector de diagnóstico compilado sin `WF_FRONTEND=1` es una entrada diferente; para jugar con el frontend usa las opciones documentadas.

## Trabajo pendiente

- Confirmar que P1-03B supera la carga de recursos y alcanza el título en Saturn real.
- Cerrar la aceptación completa de TAG y Royal, con todas sus interacciones, transiciones y estados finales. Hay código de récords, iniciales, créditos y guardado, pero su validación física completa sigue pendiente.
- Alcanzar la cadencia sostenida del arcade, especialmente con la mayor carga de Royal. Los porcentajes antiguos del emulador son mediciones históricas, no garantías de esta candidata ni de la consola real.
- Completar la comparación de imagen, animaciones, prioridades y tiempos en todo el juego.
- Validar el audio revisado en hardware, eliminar las interrupciones restantes y resolver los bucles y ciclos de vida musicales exactos. Las pistas largas siguen siendo **capturas finitas de 600 segundos**.

El [documento de estado](docs/STATUS.md) distingue las evidencias recuperadas, las comprobaciones de esta renovación y las pruebas físicas pendientes. Una prueba aprobada en el ordenador no equivale a una prueba aprobada en Saturn.

## Organización del repositorio

| Ruta | Contenido |
| --- | --- |
| [`core/`](core/) | Lógica y estados nativos, tablas y contratos de compatibilidad con la referencia |
| [`port/`](port/) | Arranque Saturn y adaptadores de gráficos, recursos, audio, entrada y guardado |
| [`sdk/jo_engine/`](sdk/jo_engine/) | Fuentes adaptadas de Jo Engine |
| [`sdk/Compiler/COMMON/`](sdk/Compiler/COMMON/) | Reglas de compilación, linker y plantilla de arranque Jo adaptados |
| [`tools/`](tools/) | Recuperación y generación de recursos, análisis de capturas y empaquetado |
| [`tests/`](tests/) | Regresiones de host, implementaciones de referencia congeladas y muestras de RAM |
| [`docs/`](docs/) | Compilación, documentación técnica, estado, procedencia e historial |
| [`evidence/`](evidence/) | Informes y registros compactos identificados por revisión |

## Colaborar y comunicar pruebas físicas

Son útiles las correcciones pequeñas y reproducibles, las mediciones con un método claro, las comparaciones apoyadas en la ROM y los informes precisos de hardware. Incluye el commit o hash del disco, región de la consola, cartucho RAM, MODE o método CD-R/cargador, los pasos seguidos y la pantalla de diagnóstico completa si aparece. Consulta [CONTRIBUTING.md](CONTRIBUTING.md).

Los cambios de comportamiento deben apoyarse en evidencias. Conserva las soluciones del compilador, las reservas de memoria y los avisos de terceros. No sustituyas un comportamiento sin resolver por una regla inventada y lo presentes como idéntico al arcade.

## Créditos y derechos

Este es un proyecto de desarrollo independiente, sin afiliación ni respaldo de los titulares del juego original o de Sega. Los nombres, personajes, gráficos y sonidos originales pertenecen a sus respectivos titulares. El repositorio no concede licencia sobre esos materiales.

Jo Engine, zlib, la referencia Z80 y Nuked-OPM conservan sus propios avisos y condiciones. Consulta los [avisos de terceros](docs/THIRD-PARTY.md). No se aplica una nueva licencia general a todo el código importado ni a los materiales del juego original.
