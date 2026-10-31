# ENCARGO MAESTRO PARA CODEX
## WWF WrestleFest Arcade (1991) → Sega Saturn — Port completo ROM-EXACT

### 1. MISIÓN

Actúa como ingeniero principal experto en programación de Sega Saturn, SH-2, VDP1, VDP2, SCU, DMA, SMPC, CDFS, Yaul/libyaul y arquitectura arcade Motorola 68000.

Tu misión es construir **una única versión completa y jugable de WWF WrestleFest de Technōs (1991) para Sega Saturn NTSC**, utilizando los códigos fuente, investigaciones ROM-EXACT y optimizaciones obtenidas hasta el 30 de octubre de 2026.

No quiero otra demostración técnica independiente. Quiero el videojuego completo, desde el arranque hasta los finales, con sus modos TAG TEAM y ROYAL RUMBLE, frontend, transiciones, entradas al ring, presentaciones, progresión, continuaciones, records y demás elementos originales.

**La ROM original `wwfwfest.zip` manda absolutamente.** No inventes movimientos, mecánicas, animaciones, IA, reglas ni temporizaciones de gameplay.

El objetivo principal es lograr que Saturn ejecute el juego con una fluidez prácticamente indistinguible del arcade, tanto con su RAM estándar como con cartucho de expansión de 4 MB.

### 2. FUENTES Y REPOSITORIO

Repositorio privado:

https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private

Examina detalladamente:

- `Intro/`: WP0 R04, intro antidroga y título animado. Aprobada en Saturn real.
- **`Entrada Ring/`: WP01, entrada de los luchadores al ring, ya implementada, probada y aprobada en Saturn real con velocidad de desplazamiento arcade. Es una implementación válida que debes conservar e integrar.**
- `Escena Presentadora/`: WP02 R02, presentación de los cuatro luchadores, presentadora y secuencia original. Aprobada.
- `Combate real en Saturn/`: WP1979, combate TAG nativo con Hogan, Warrior, DiBiase y Mr. Perfect.
- `Tag Mode/`: investigaciones y checkpoints WP2015, incluida la recuperación de fuentes R3 y documentación de rendimiento.
- `modo Royal/`: WP1980 HW1, código fuente del combate Royal y configuraciones estándar/4 MB.
- `src/`, `tests/`, `docs/`, `HANDOFF.md`, `PROJECT-STATE.md` y el resto de contratos técnicos.

Consulta también los paquetes ROM-EXACT y workpacks del proyecto, incluyendo los relativos a rankings, records, créditos, resultados, continuaciones, cámaras, animaciones, gameplay, interfaz y frontend.

**No te limites a los informes. Examina las fuentes reales y reconstruye cada implementación antes de integrarla.**

La jerarquía de autoridad es:

1. ROM original.
2. Desensamblado y evidencias ROM-EXACT auditadas.
3. Especificaciones de hardware Saturn y documentación Yaul.
4. Código fuente y pruebas anteriores.
5. Nuevas adaptaciones necesarias para Saturn.

Los módulos aprobados deben conservar su comportamiento y fidelidad. Sólo podrás modificar sus adaptadores de integración cuando exista una necesidad técnica demostrable, verificando que no se introducen regresiones.

### 3. RENDIMIENTO YA DEMOSTRADO

Las investigaciones previas han acreditado resultados significativos.

**TAG WP2015 R6:**

- Aproximadamente 59,10 actualizaciones de combate por segundo de media en Mednafen.
- Peor ventana de 60 actualizaciones: aproximadamente 55,22/s.
- Se detectaron y optimizaron picos asociados a descompresión, cachés y transferencias de sprites.
- Las fuentes completas de R6 se perdieron tras la interrupción del entorno; no están recuperadas en GitHub. Se conservan fuentes anteriores, pruebas y documentación.

No afirmes que R6 está disponible sin verificarla. Recupera y vuelve a implementar las optimizaciones documentadas que falten.

**ROYAL WP1980 HW1:**

- Cartucho 4 MB: 58,83 actualizaciones/s de media.
- Sin cartucho: 50,59 actualizaciones/s de media.
- Peor ventana de 99 actualizaciones con 4 MB: 55,35/s.
- La versión con 4 MB demuestra que la precarga y la administración de memoria pueden mejorar sensiblemente el rendimiento.

**INTRO, ENTRADA AL RING Y PRESENTADORA:**

Estas escenas cuentan con implementaciones independientes ya aceptadas en Saturn real. La entrada al ring, en particular, consiguió representar el desplazamiento de los luchadores a velocidad arcade. Debes recuperar sus técnicas de renderizado, sincronización y gestión de recursos y aprovecharlas donde sean aplicables al juego completo.

La referencia de la lógica arcade ronda las **57,44485 actualizaciones/s**.

No confundas esa frecuencia con los aproximadamente 59,826 refrescos físicos por segundo de Saturn NTSC.

**OBJETIVO:** velocidad de lógica arcade, animaciones originales completas, movimientos sin ralentizaciones perceptibles, sin parpadeos y sin frames de animación eliminados.

No basta con superar la media: también hay que reducir los peores picos de tiempo.

### 4. ARQUITECTURA DEL JUEGO COMPLETO

Construye un runtime maestro compartido que integre:

- Planificador principal SH-2 y lógica arcade.
- Renderizado de luchadores y sprites mediante VDP1.
- Fondos, HUD y capas mediante VDP2 según las necesidades técnicas.
- Gestor de animaciones y atlas de sprites.
- Caché de memoria interna y expandida.
- Transferencias optimizadas y DMA cuando corresponda.
- Entrada de mandos SMPC.
- Máquina de estados de frontend, gameplay y resultados.
- Sistema de lectura y carga del CD.
- Interfaz de audio preparada, pero desactivada durante esta fase.

No concatenes las BIN/CUE anteriores ni conviertas las demostraciones en vídeos precalculados.

Integra sus fuentes y algoritmos en un único ejecutable nativo.

No dupliques bucles principales, temporizadores, gestores de VRAM, inicializaciones de pantalla ni propiedades de estados.

### 5. CARTUCHO DE EXPANSIÓN DE 4 MB

Quiero **una sola BIN/CUE universal**, compatible con Saturn NTSC estándar y Saturn NTSC con cartucho RAM de 4 MB.

Durante el arranque, el programa debe identificar y comprobar la expansión conforme a los procedimientos técnicos compatibles con Sega Saturn.

**Con cartucho válido de 4 MB:**

Activa automáticamente una configuración de memoria ampliada para precargar y conservar más sprites, animaciones, atlas, tablas y datos reutilizables.

Aprovecha las técnicas demostradas en Royal WP1980 y las investigaciones TAG, reduciendo transferencias desde CD y descompresiones durante el combate.

**Sin cartucho de 4 MB:**

Arranca normalmente con la RAM interna, utilizando una configuración de caché adecuada y precarga optimizada.

No elimines movimientos, personajes, animaciones, reglas ni características del arcade.

No confundas un cartucho de 1 MB con uno de 4 MB.

La memoria ampliada no aumenta directamente la potencia de los SH-2 ni la VRAM. Debe utilizarse inteligentemente como almacén de recursos, manteniendo las operaciones críticas en memoria rápida.

Ambas configuraciones deben ejecutar la **misma lógica de gameplay**. Sólo cambia la estrategia de almacenamiento y carga.

El objetivo de rendimiento arcade es válido para ambas, no exclusivamente para la versión ampliada.

### 6. LIBERTAD PARA IMPLEMENTAR TIEMPOS Y PANTALLAS DE CARGA

**IMPORTANTE: TIENES LIBERTAD TOTAL PARA ESTABLECER LOS TIEMPOS DE CARGA QUE CONSIDERES NECESARIOS ENTRE COMBATES, ESCENAS Y TRANSICIONES.**

No te impongo que el juego tenga una sola carga inicial ni que todas las transiciones sean instantáneas.

Puedes utilizar cargas específicas:

- Después de la selección de luchadores.
- Antes de las entradas al ring.
- Antes del combate.
- Al finalizar los combates.
- Antes de las escenas posteriores al combate.
- Entre enfrentamientos sucesivos.
- Durante cambios de escenario o fase de competición.
- Antes de rankings, resultados o pantallas finales.
- En cualquier otra transición en la que sea técnicamente necesario.

Puedes mostrar una pantalla de carga discreta y funcional cuando la ROM no tenga un equivalente, identificándola como una adaptación técnica Saturn y sin modificar escenas originales innecesariamente.

No existe un límite arbitrario de segundos. Decide según la cantidad de recursos que necesites precargar y el rendimiento que se obtenga.

**La prioridad absoluta es que, una vez comenzado el combate, la acción transcurra continuamente y a la velocidad arcade.**

Prefiero una carga previa de varios segundos a un combate con tirones, pausas del CD, sprites incompletos o movimientos lentos.

Aprovecha esos periodos para:

- Precargar animaciones de todos los luchadores que participarán.
- Preparar atlas y texturas.
- Descomprimir recursos cuando sea conveniente.
- Organizar los bancos de memoria.
- Preparar fondos, HUD y paletas.
- Llenar las cachés del cartucho de 4 MB cuando esté presente.
- Verificar que el conjunto necesario es accesible antes de iniciar el combate.

Las cargas deben estar controladas, terminar correctamente y comprobar errores. No encubras bloqueos reales con una pantalla de carga permanente.

**No introduzcas esperas artificiales durante una escena cuyo timing ya está demostrado.** Una vez iniciada la entrada al ring, presentación, combate o animación arcade, su reproducción debe respetar los tiempos originales.

### 7. FRONTEND Y ESCENAS COMPLETAS

Completa el juego original, incluyendo:

- Pantalla antidroga.
- Introducción y título animado.
- Attract/demo.
- Créditos y comienzo de partida.
- Selección TAG/ROYAL.
- Selección de luchadores.
- Información y presentación de personajes.
- Entradas al ring.
- Presentadora y anuncios de los participantes.
- Inicio del combate.
- HUD, cámara, árbitro y elementos del ring.
- Secuencias de victoria y derrota.
- Escenas posteriores al combate.
- Progresión entre enfrentamientos.
- Pantallas de continuación y sus cuentas atrás.
- GAME OVER.
- Records, rankings y registro de iniciales.
- Secuencias finales y créditos del juego.
- Retorno al frontend y ciclo attract.

Todo conforme a lo existente en la ROM.

#### Escena Entrada Ring — obligación específica

La carpeta `Entrada Ring/` de GitHub contiene una implementación Saturn independiente de la entrada al ring que ya fue aprobada en hardware real.

**No la ignores ni vuelvas a diseñarla desde cero.**

Inspecciona su código, animaciones, sincronización, control de VDP1/VDP2 y desplazamiento.

Integra las rutas de entrada correspondientes a las modalidades y luchadores originales. Las pruebas anteriores con dos luchadores son una referencia técnica, no un límite de contenido para el juego final.

Debe conservar la velocidad arcade y enlazar correctamente con la presentadora y el inicio del combate.

Si su integración exige cargar recursos previamente, utiliza los tiempos de carga autorizados en la sección anterior.

Durante esta fase el sonido de la escena permanecerá desactivado, igual que en el resto del juego.

### 8. GAMEPLAY COMPLETO ROM-EXACT

No limites el juego a los cuatro luchadores de las primeras pruebas.

Implementa todos los personajes, acciones, reglas y comportamientos presentes en el arcade.

Audita los sistemas de:

- Movimiento y orientación.
- Animaciones y cambios de estado.
- Golpes, agarres y derribos.
- Llaves y movimientos especiales.
- Finishers.
- Contras y recuperaciones.
- Detección de colisiones.
- Daño y resistencia.
- Cuerdas y combate fuera del ring.
- Pins, cuentas y arbitraje.
- Relevos TAG.
- Comportamiento de compañeros y rivales.
- IA de todos los luchadores.
- Eliminaciones y reglas Royal.
- Victoria, derrota y progresión.
- Continues y condiciones finales.

Si encuentras rutinas incompletas, `TODO`, stubs, movimientos ausentes o estados que producen bloqueos, **investiga la implementación original en la ROM y complétala**.

No improvises su comportamiento.

Utiliza pruebas diferenciales de los estados internos, RNG, daño, animaciones, contactos y temporizadores.

No declares el juego completo si existen rutas obligatorias pendientes. Documenta como OPEN cualquier comportamiento que no pueda demostrarse.

### 9. OPTIMIZACIÓN DE RENDIMIENTO

La máxima prioridad técnica es alcanzar una cadencia de combate comparable a la arcade.

Queda prohibido aumentar artificialmente el rendimiento mediante:

- Supresión de fotogramas originales de animación.
- Reducción del número de luchadores.
- Eliminación de acciones.
- Simplificación de IA.
- Cambios en la velocidad de desplazamiento.
- Sustitución del combate real por secuencias pregrabadas.
- Reducción injustificada de fidelidad visual.

Audita los costes de SH-2, renderizado VDP1, VDP2, preparación de sprites, descompresión, transferencias, accesos al CD, memoria y sincronización de los buffers.

Prioriza la eliminación de picos, especialmente durante cambios de pose, agarres, movimientos especiales, entradas simultáneas y situaciones con varios luchadores visibles.

Reutiliza las optimizaciones demostradas en TAG y Royal cuando sean aplicables, pero verifica nuevamente cada una en el runtime integrado.

Las pantallas de carga deberán absorber el trabajo que pueda adelantarse fuera del combate.

### 10. AUDIO DESACTIVADO EN ESTA FASE

**NO IMPLEMENTES NI ACTIVES EL SONIDO TODAVÍA.**

No reproduzcas música, efectos, voces ni CDDA en esta beta.

Conserva los puntos de llamada, eventos y contratos ROM-EXACT para una integración posterior del audio nativo SCSP.

La ausencia de audio no debe alterar la lógica, las transiciones ni los temporizadores del juego.

Identifica las mediciones como rendimiento sin audio. No afirmes que el rendimiento futuro con sonido ya está demostrado.

### 11. PRUEBAS OBLIGATORIAS EN MEDNAFEN

Utiliza Mednafen 1.32.1 y ejecuta las BIN/CUE reales que producirás.

Prueba tanto sin cartucho como con expansión de 4 MB.

Debes comprobar el recorrido completo del juego, incluyendo intro, selección, entrada al ring, presentadora, TAG, Royal, progresión, continuaciones, rankings, records y finales.

Incluye pruebas con mando que ejecuten movimientos y situaciones imprevistas.

Mide:

- Actualizaciones lógicas por segundo.
- Refrescos de pantalla consumidos.
- Media de rendimiento.
- Peores ventanas de 1 y 5 segundos.
- Peores ventanas de 60 y 99 actualizaciones.
- Picos de preparación y subida gráfica.
- Lecturas del CD durante el combate.
- Pausas, bloqueos y errores de memoria.
- Parpadeos, sprites incompletos y fallos de sincronización.

Distingue el tiempo de carga del tiempo efectivo de combate. No ocultes ralentizaciones excluyendo del análisis ventanas exigentes del gameplay.

Compara los estados de juego con la ROM original y las referencias existentes.

Registra vídeos reales de Mednafen con sus tiempos originales, sin interpolación ni aceleración artificial.

Cuando detectes un error o una rutina ausente, detén la promoción de esa compilación, corrige el problema y repite la prueba.

### 12. TRABAJO AUTÓNOMO Y GITHUB

Crea una rama de integración independiente:

`feature/wrestlefest-full-game-rom-exact`

No modifiques las carpetas originales aprobadas ni los proyectos anteriores.

Trabaja sobre un directorio nuevo de producción integrado.

Después de cada hito funcional:

1. Guarda todas las fuentes, herramientas y pruebas.
2. Publica un commit en GitHub.
3. Verifica que el commit está accesible.
4. Guarda los SHA-256 de los archivos y del ejecutable.
5. Registra la configuración de compilación.
6. Conserva BIN/CUE, vídeos y recursos externos en almacenamiento persistente, cuando corresponda.
7. Actualiza `ESTADO.md` con lo realizado, lo pendiente y el siguiente trabajo exacto.

**No vuelvas a permitir que una versión avanzada exista únicamente en una carpeta temporal.**

No borres ni sustituyas los checkpoints que ya hayan sido verificados.

Si se interrumpe la sesión, debe quedar una versión recuperable desde GitHub y los recursos externos documentados. No prometas ejecución en segundo plano cuando la sesión no esté activa.

### 13. ORDEN DE EJECUCIÓN

Organiza el desarrollo en estas fases:

**A. Auditoría y recuperación:** identificar las mejores fuentes disponibles de cada escena y motor, sin promover material incompleto como versión definitiva.

**B. Runtime maestro:** unificar inicialización Saturn, SH-2, entrada, VDP1/VDP2, memoria, CD y detección automática de cartucho.

**C. Frontend integrado:** desde la intro y selección hasta la entrada al ring y presentadora, conservando las implementaciones aceptadas.

**D. Combate TAG y ROYAL:** integrar ambos motores, completar movimientos y reglas faltantes, configurar precarga normal y ampliada.

**E. Escenas y progresión:** resultados, postcombate, rankings, records, continuaciones, finales y retorno al frontend.

**F. Rendimiento:** auditar y corregir los picos de todos los recorridos con Mednafen.

**G. Beta completa:** compilar, verificar, documentar y entregar el disco universal.

Puedes solapar fases si las dependencias técnicas lo permiten, pero no declares una cerrada sin evidencia.

### 14. ENTREGA SOLICITADA

La entrega final será una **BETA COMPLETA SIN AUDIO** que incluya:

- Una única BIN/CUE universal para Sega Saturn NTSC.
- Detección automática del cartucho RAM de 4 MB.
- Funcionamiento íntegro sin cartucho.
- Frontend completo.
- Entrada al ring y escena de presentadora integradas.
- TAG y Royal con todos los personajes y mecánicas originales.
- Escenas de resultados, continuaciones, records, rankings y finales.
- Tiempos de carga establecidos libremente entre escenas y combates.
- Combate continuo con rendimiento lo más próximo posible al arcade.
- Código fuente íntegro, reproducible y conservado en GitHub.
- Vídeos de Mednafen.
- Pruebas y métricas de rendimiento de ambas configuraciones.
- Instrucciones para comprobar el disco en Sega Saturn física con MODE.

Si alguna función todavía no está terminada, no entregues una beta etiquetada falsamente como completa. Conserva y entrega el último checkpoint funcional, indicando los bloqueos exactos.

### 15. INSTRUCCIÓN FINAL

Tienes autonomía para investigar, programar, compilar, probar, optimizar y guardar checkpoints, sin solicitar confirmaciones rutinarias.

No emplees turnos en informes innecesarios. Prioriza trabajo ejecutable y evidencias verificables.

**DOS REGLAS IRRENUNCIABLES:**

**PRIMERA:** puedes dedicar a las cargas entre escenas y combates todo el tiempo técnicamente necesario para preparar los recursos. No sacrifiques el rendimiento del combate por reducir unos segundos la espera.

**SEGUNDA:** el rendimiento, los movimientos, las animaciones, las reglas y la fluidez del combate deben acercarse al máximo al arcade original. No aceptes simplificaciones del gameplay como solución a un cuello de botella.

La ROM manda. Saturn debe ejecutar el juego de forma nativa.

**OBJETIVO FINAL: WWF WRESTLEFEST COMPLETO PARA SEGA SATURN, CON UN ÚNICO DISCO, COMPATIBLE CON Y SIN CARTUCHO DE 4 MB, Y COMBATES A VELOCIDAD PRÁCTICAMENTE ARCADE.**

Comienza ahora auditando las fuentes de GitHub y los paquetes originales, crea el primer checkpoint de integración reproducible y continúa hasta alcanzar los criterios de aceptación.