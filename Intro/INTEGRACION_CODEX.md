# Integrar la intro R04 desde fuentes

## Punto de partida

Usar `source/` de esta carpeta y los recursos restaurados según `COMPILAR.md`. Los 34 archivos de texto copiados del paquete original deben conservar los hashes de `audit/build/delivered_source_hashes.json`. Ese manifiesto histórico contiene también los recursos externos y `cd/A.BIN`, hasta completar los 42 archivos originales.

La publicación no cambia el programa probado. Los únicos añadidos son documentación, manifiestos y el restaurador del paquete. La ROM original prevalece sobre cualquier implementación o comentario.

## Alcance real

Esta demostración termina al finalizar la secuencia del título. El secuenciador aplica estados exportados del arcade; no implementa por sí mismo créditos, selección de modo, roster, attract posterior ni transiciones generales del juego. No inferir esos comportamientos a partir del final de este bloque.

Los datos gráficos conservan mapas de celdas, paletas, scroll y sprites que Saturn representa mediante VDP1/VDP2. El PCM del título ya está mezclado y usa Rice sin pérdida. No etiquetar este bloque como motor de síntesis YM2151/OKI en tiempo real, ni confundir su revisión R04 con la del audio WP24 del repositorio.

## Propiedad de recursos que debe reconciliarse

| Área | Condición que conserva la demostración |
|---|---|
| Temporización | Lectura coherente FRCH/FRCL mediante `wp0_timer_count_get()`, guardando/restaurando SR y enmascarando las interrupciones enmascarables en ese acceso. Una NMI no queda enmascarada. |
| Presentación | Preparación, envío y terminación de dibujo coordinados por `wp0_framebuffer`; no desactivar las guardias 401/402/403 para ocultar sobrecostes. |
| Arranque/reinicio | Inicialización de ambos framebuffers y preparación del estado inicial con la imagen oculta. |
| Caché de secuencia | Seis paquetes activos grandes conservados en 106.072 bytes del `stage` ya reservado; preparar tras la carga de audio y conservar hasta finalizar/reiniciar. |
| Audio | SCSP slot 0, PCM16 mono 44,1 kHz, búfer circular de 64 KiB y palabra adicional de interpolación. El SH-2 atiende la reposición. |
| Inicialización SCSP | `wp0_scsp_initialize()` apaga el 68000 de sonido, reinicializa slots y desactiva escrituras DSP heredadas. No ejecutarlo sin reconciliar el propietario de sonido del juego. |
| Memoria | La demo utiliza varias regiones de RAM/VRAM para datos comprimidos. Su mapa es del bloque aislado; no asumir que esas regiones siguen libres en el juego integrado. |

La cadencia usa el racional 37609/39168 para conservar todos los estados arcade al presentarlos en campos Saturn NTSC. No sustituirla por un avance arbitrario de un estado por VBlank ni describir esa cuantización como identidad temporal inferior a un campo.

El audio de esta demo empieza en el evento 02 asociado a la fuente 606 y se detiene al terminar la escena. El fichero de 37 segundos incluye cola de seguridad; no convertirlo en un bucle musical general.

El transporte WP24 existente tiene otros propietarios de slots, buffers y lógica de música. Integrar ambos requiere una decisión explícita sobre el servicio de sonido común. Las constantes de un bloque no se deben copiar al otro sin comprobar su representación y evidencia.

## Secuencia de integración

1. Reconstruir primero la demo independiente con el mismo A.BIN de referencia.
2. Leer los contratos generales del repositorio y los documentos ROM de frontend correspondientes.
3. Definir entrada, actualización y salida de escena dentro del controlador canónico, respetando los propietarios existentes de entrada, VBlank, VDP1/VDP2 y sonido.
4. Asignar memoria y recursos antes de añadir las cargas y callbacks. Evitar inicializaciones duplicadas que borren el estado de otra escena.
5. Conservar el contenido y el orden de los datos originales; no regenerarlos con cambios visuales o temporales inventados.
6. Volver a comprobar arranque, secuencia completa, final y reinicio en Saturn real, incluyendo las transiciones añadidas por el juego.

Integrar módulos y datos; nunca concatenar imágenes BIN/CUE ni reemplazar el bucle principal del port por el `main.c` completo de esta demostración.

## Evidencia

`VALIDACION_R04.txt` es el informe histórico de la entrega. `PUBLICACION_VERIFICADA.json` documenta las comprobaciones de esta copia para GitHub. La confirmación posterior del usuario «Intro ok.» acredita únicamente el funcionamiento observado de la intro independiente; no acredita carga conjunta con gameplay ni medidas instrumentales de FPS.
