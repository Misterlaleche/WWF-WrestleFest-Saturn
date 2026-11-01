# Integración de Audio 2

No integrar como audio final. La música nativa aún pierde notas bajo combate.

1. Preservar R04 y el runtime gráfico en su rama actual.
2. Tratar `base/` como código de audio recuperado y `experimentos/` como variantes separadas, nunca como un único build aprobado.
3. Mantener la contención de errores: un fallo de audio no bloquea el gameplay; conservar esperas acotadas, STOP y latidos.
4. Revisar mapa ELF y asignación antes de cargar. El master ABI3 medido reserva 14 KiB TLSF privado, 18 KiB de ventana hot y 8 KiB de pila dentro de los 40 KiB del SDK.
5. El hot del último prototipo ocupa 13992 B más 3220 B de estado; quedan 1220 B en su ventana de 18432 B. No apropiarse de otra RAM sin contrato.
6. ABI4 cambia el cargador, su cabecera y la posición enlazada de HWRAM. Recompilar master/módulo como un par y comprobar todas las direcciones.
7. El experimento PCM nuevo es independiente del firmware b882 usado al medir el último Z80. Regenerar layout de observador 68000 desde su compilador; no reutilizar offsets de otra estructura.
8. Medir el contador real de underruns de 32 bits, no sólo el buzón. Cualquier descarte, underrun o audio desactivado bloquea la aceptación.
9. No ampliar colas, tolerancias o lead para ocultar retrasos. No convertir capturas finitas en bucles de producción.
10. Ejecutar la cobertura completa y el control gráfico equivalente con audio/SFX/sin audio antes de una imagen. La suite conservada es infraestructura y plan, no evidencia de ejecución de todo el catálogo.

No se modifica ningún productor de gameplay sin evidencia ROM. El audio consume los comandos del dueño de escena/gameplay.
Los 18×600 s de comparación HOST verifican semántica frente al secuenciador original; no demuestran que Saturn lo ejecute en tiempo real.
