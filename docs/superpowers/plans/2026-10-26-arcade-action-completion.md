# Completar acciones y animaciones frente a la ROM arcade

## Criterio de cierre

El selector conectado al despachador no cuenta como acción terminada. Cada
selector `$00..$8E` necesita: sus ramas alcanzables identificadas en la ROM,
comparación de RAM/FG/paleta/registros en estados representativos, secuencias
con las transiciones del motor que correspondan, y una ruta de combate o escena
que atraviese sus enlaces relevantes sin bloqueo. Las animaciones también
requieren comprobar los descriptores y cuadros realmente usados por cada
luchador. Audio nativo queda aplazado por petición del usuario.

## Ejecución

1. Inventariar los 143 selectores y clasificar la evidencia existente en
   conexión, comparación aislada, cobertura de ramas, secuencia y ruta completa.
   Registrar los huecos sin convertir el número de pruebas en un porcentaje.
2. Empezar por las rutas de movimiento/contacto `$11`, `$14` y `$35`, cuyo
   cotejo de trayectoria exige intercalar movimiento y colisión; extender el
   oráculo con estados del motor reales y resolver toda divergencia observada.
3. Repetir el ciclo para las otras ramas y secuencias ausentes, incluidos los
   cambios Royal/TAG, parejas y estados de cada personaje. Conservar capturas
   de los bloqueos y convertirlas en regresiones antes de corregirlas.
4. En cada ruta medir trabajo gráfico y de CD: tiles reconstruidos, transferencias
   VDP1, RAM residente, fallos de caché y lecturas de disco. Optimizar solo el
   trabajo repetido que las mediciones identifiquen; comprobar que los bytes
   visuales y el comportamiento siguen coincidiendo con la ROM.
5. Pasar pruebas diferenciales, rutas largas con sanitizadores, compilación
   SH-2 y validación de disco. La Saturn real queda como validación final de
   fluidez y escenas. Las siguientes imágenes de prueba deben conservar la
   configuración sin música nativa hasta que se investigue por separado.

## Estado inicial

- 143 selectores enlazados. Matrices y pruebas focalizadas cubren sus IDs,
  pero no todas sus ramas ni secuencias.
- El disco de aislamiento sin música volvió a ser jugable en Saturn NTSC/MODE,
  según prueba del usuario; el R02 con música se queda negro tras Technos.
- Pendientes prioritarios ya documentados: trayectoria `$11`/`$14`/`$35`,
  flujo Royal `$8E`, variantes de parejas/modos y sprites de escenas frías.

## Progreso del 26 de octubre

- El inventario reproducible de los 143 IDs, punteros de la ROM, elecciones
  por personaje y rutas de prueba está en
  `validation/local-20261026/action-selector-inventory.json`. Continúa
  pendiente la clasificación exhaustiva de ramas internas y secuencias
  antiguas antes de afirmar una cobertura total.
- `$11`, `$14` y `$35` ya tienen secuencias con relevo de estado, resolución
  del ring y física intercalados; los seis informes para doce personajes están
  en `validation/local-20261026/action-interleaved/`.
- El interludio Royal `$8E` ya tiene una secuencia continua de 653 pasos por
  personaje con los cuatro subestados y el relevo final `$79`; el séptimo
  informe está en la misma carpeta.
- `$44` pasó los cinco índices de acierto y los cuatro índices que pueden
  fallar hasta `$82`→`$7B` para los doce.
  `$11` y `$14` pasaron además impactos contra rival derribado y la salida
  final, junto con una ruta contra rival activo por acción. `$12` y `$13`
  pasaron caída, impacto y salida del atacante; `$12` también con rival activo
  `$61`. `$35` añadió seis rutas de fallo con rival activo y borde interior,
  para los doce luchadores. Ocho informes siguen al rival derribado por
  `$11`/`$12`/`$13`/`$14` y los cuatro modos de `$35` hasta reposo; son
  9.384 pasos del rival. Los saltos `$0C`, `$0D`, `$10` y la carrera `$0E`
  añadieron quince trayectorias de contacto y fallo. Otras siete siguen la
  recuperación del rival o el relevo `$77`→`$61`/`$37`. Tres más siguen las
  reacciones de `$0D` hasta reposo. Hay 67 informes de secuencia, 82.193
  pasos del atacante y 21.624 del rival. El inventario
  comprueba hashes
  y sumas. Otras posiciones,
  contactos y animaciones visuales todavía requieren más secuencias.
- Dos índices gráficos de 1 KiB dejan de borrarse cada fotograma; las pruebas
  de 260 fotogramas conservaron orden de sprites y bajaron los borrados de
  532.480 a 4.096 bytes acumulados. Falta medir FPS en Saturn.

## Magnitud pendiente: censo provisional

El resumen histórico documenta secuencias consecutivas para 40 IDs distintos;
los informes intercalados nuevos añaden siete IDs sin solapamiento. Por tanto,
96 de los 143 IDs no aparecen en esos dos listados de secuencias. Este número
no mide acciones incompletas: algunas son instantáneas y puede haber evidencia
en verificadores antiguos que todavía no se haya indexado. Los 47 IDs citados
tampoco quedan cerrados por tener una sola secuencia. Antes de calcular un
porcentaje o una cantidad exacta de ramas restantes, hay que indexar por ID
las bifurcaciones ROM, las trayectorias ya verificadas y los cuadros gráficos
utilizados por los doce personajes.
