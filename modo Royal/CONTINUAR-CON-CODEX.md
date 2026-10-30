# Continuación con Codex

## Estado de partida

El usuario pidió conservar en GitHub esta implementación bajo el nombre **modo Royal** y continuar buscando velocidad arcade. Este snapshot es **WP1980 HW1**, ya entregado antes de la publicación. No atribuirle mejoras de velocidad desarrolladas después ni sustituir sus mediciones por las de otra imagen.

Base activa: `wp1980/work/hwfix01/`. Base anterior para RED: `wp1980/native/`, disponible mediante `--scope all`. Conservar ambas y abrir una copia aislada para el siguiente candidato. Registrar cada cambio y sus hashes; no reutilizar el nombre HW1 para una imagen diferente.

La publicación añade exclusivamente esta carpeta al repositorio de partida `ea1e193d9a72c534bdead5206e9b8449757437b2`. No cambia el estado canónico de las otras escenas, audio o gameplay. No promover una instrucción histórica del snapshot por encima de `README.md`, `PROJECT-STATE.md`, `HANDOFF.md` y las autoridades originales del proyecto.

## Primeros pasos reproducibles

1. Leer los tres documentos de esta carpeta y los contratos de la raíz indicados en README.
2. Obtener el ZIP exacto enlazado, verificarlo y restaurar `build`; para probes y evidencia amplia, restaurar `all`.
3. Copiar `wp1980/` a una carpeta sin espacios. El build se hace desde `work/hwfix01`, con el SDK suministrado, en directorios de salida nuevos.
4. Mantener un resultado de referencia de base y 4 MB, con identidad del ejecutable y disco, opciones del emulador y hashes de recursos.
5. Repetir pruebas específicas que afecten al cambio y los controles de memoria/disco; el `make test` heredado está bloqueado por `tests/test_core.c` ausente y debe seguir constando como bloqueo.

## Rendimiento: referencia honesta

| HW1 | 4 MB | Base |
| --- | ---: | ---: |
| Actualizaciones/campos | 8.474 / 8.618 | 8.474 / 10.022 |
| Actualizaciones/s medias | 58,826476 | 50,585369 |
| Peor ventana observada de 99 | 55,353142/s | 41,709762/s |
| Peor ventana observada de 59 | 54,303712/s | 37,954207/s |

El objetivo estricto **no está cumplido**. Una media favorable no demuestra velocidad mínima constante. Las mediciones son de Mednafen standalone 1.32.1, NTSC y caché full; no equivalen a la tasa física de Saturn ni a una medición de la placa arcade. Fijar el umbral de comparación en la evidencia arcade aprobada y conservar su procedencia, sin redondearlo a 60 por conveniencia.

Los logs guardan las ventanas con peor resultado: 4 MB, actualizaciones 4.373→4.472 para 99; base, 4.365→4.464. La ventana base de 59 está en 5.780→5.839. Son puntos útiles para perfilar; no basta con optimizar solo esos puntos, ni interpolar las observaciones que faltan.

## Trabajo siguiente

Medir primero qué consume el tiempo en una variante aislada: propietarios del runtime, preparación de sprites, consultas de ROM/atlas, transferencias VDP1, sincronizaciones, commits VDP2 y servicio de efectos. Separar el coste de instrumentación del binario de aceptación. Los contadores actuales `wp1980_draws`, `wp1980_vblanks`, `wp1980_max_gap`, `wp1980_sprite_block_decodes` y `wp1980_sprite_block_hits` ofrecen puntos de observación; no todos explican por sí solos el cuello de botella.

La variante 4 MB ya elimina descompresión del atlas pero conserva caídas observadas. No concluir que el cartucho resuelve todo el presupuesto. La variante base necesita conservar su ejecución funcional y sus límites de memoria; no financiar una mejora ocultando datos fuera de las regiones declaradas.

Preferir reducir trabajo repetido comprobablemente redundante, mejorar residencia o transporte conservando bytes y orden observable, o reorganizar preparación de datos dentro de los contratos. No cambiar RNG, tiempos de la ROM, velocidades, daño, colisión, turnos de CPU o cadencia de animación para obtener una cifra mayor. No descartar frames del juego, saltar errores, precalcular estados finales ni convertir una demo en una reproducción de vídeo.

Cada candidato necesita una comparación con **la misma condición inicial y entradas**. Como mínimo: estado final completo de nueve actores, RNG, frame, resultado, errores y CD; cuando el cambio afecte una fase, comparar también checkpoints intermedios relevantes. Mantener los probes de resultado con lectura denegada y caché reemplazada cuando se toque residencia o layout. El cierre anterior fija siete páginas por una razón reproducida; no volver a enfriarlas sin demostrar un contrato equivalente.

## Límites que siguen abiertos

- HW1 en consola: el usuario probó las imágenes anteriores, no esta revisión corregida. La etiqueta sigue siendo hardware pendiente.
- Audio: efectos activados y música desactivada. La corrección del anillo es verificable, pero no certifica que explique o resuelva todo el problema audible físico. Una cola vacía no prueba fidelidad ni sincronía.
- Integración: el entry directo, ROYINIT, entradas automáticas y loop final son un banco de pruebas. Conectar selección, RNG, setup y siguiente estado por sus propietarios originales, compartiendo infraestructura.
- Cobertura: una demostración y los probes del resultado no prueban todas las mecánicas, semillas, entradas manuales, multijugador o PAL.
- Fallos: conservar `WF_BLOCKED` y el diagnóstico cuando no exista continuación probada. El snapshot del último CD puede preceder al fallo; no asumir causalidad solo por estar visible.

## Qué debe acompañar la próxima entrega

Fuente y diff identificados, recursos/SDK precisos, build nuevo, controles de memoria y disco, pruebas relevantes y sus límites, captura real sin alteración temporal y medidas de ventanas además de media. Identificar el estado de las pruebas físicas por imagen exacta. Un resultado de host o emulador nunca cambia por sí solo la etiqueta a HW-PASS.

Los binarios, ROM, packs, PCM, capturas y volcados siguen fuera de Git. Conservar hashes y un mecanismo de restauración verificable. La revisión de fuentes, pruebas y documentación puede hacerse desde Git; para ejecutar con los recursos exactos se necesita el paquete privado del usuario.
