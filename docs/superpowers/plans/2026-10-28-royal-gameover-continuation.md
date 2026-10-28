# Continuación de Royal tras GAME OVER

- El ejecutable R05 termina la pantalla `$1AFC` en `$1B1E` y permanece en un bucle. La ROM reúne los participantes activos en `$0092`, llama `$65C0` y salta a `$6FC`.
- Añadir una prueba diferencial de `$1B1E..$1B41`, con las 16 combinaciones de participantes y valores aleatorios, RAM y registros completos, solicitud y frontera de retorno. Primero debe fallar sobre el estado actual.
- Implementar esa continuación en el controlador post-partida existente y conectarla al frontend tras la pantalla Royal, con las mismas reglas de cancelación y guardado de TAG.
- Ejecutar el oráculo, regresiones post-partida, compilación SH-2 y una prueba de flujo cuando el entorno lo permita. No afirmar que funciona en Saturn física sin observarla ni preparar un disco de entrega sin solicitud.
