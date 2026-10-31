# Royal reconstruido sin audio — medidas nuevas

Mednafen1.32.1, NTSC, CPU cache full, velocidad normal, frameskip0.
Cada variante completa8474 actualizaciones y llega a presentación terminada
y GAME OVER. RNG y los9 registros completos de actores coinciden con la
referencia HW1. Los dos perfiles tienen cero fatales y cero incremento de
transferencias CD completadas durante combate. Esto no descarta por sí solo
intentos fallidos de lectura; los probes host de denegación cubren resultados.

Son dos ejecutables de referencia independientes, WP1980_AUDIO=0. No acreditan
el juego completo integrado ni detección universal de cartucho. Estas medias
miden la capacidad del bucle anterior, que todavía no aplica el planificador
arcade compartido de esta integración. Una media por encima de57,44485 no
demuestra por sí misma velocidad de lógica fiel al arcade.

| Medida de actualizaciones/s | RAM base | Cartucho4MiB |
| --- | ---: | ---: |
| Media combate completo |50,5904|58,8948|
| Peor ventana observada de60 actualizaciones |38,1869|55,2241|
| Peor ventana observada de99 actualizaciones |41,7098|55,8753|
| Peor ventana observada de≈1s |36,8928|54,8406|
| Peor ventana observada de≈5s |44,8696|57,4331|
| Campos físicos consumidos en combate |10021|8608|

Las ventanas temporales usan60/300 campos:duraciones reales≈1,002906/5,014532s,
registradas explícitamente. No se retiman a1/5s. Las muestras de memoria son
asíncronas:se excluyen extremos ausentes o ambiguos, sin interpolación. Por eso
se informa el peor caso observado y la cobertura, sin asegurar que sea el peor
absoluto de todos los instantes. La ventana99 del analizador nuevo coincide
con el analizador original independiente en ambos perfiles.

MOV y MP4 contienen exactamente los mismos PTS para cada fotograma. Base:
12203 fotogramas;4MiB:11275. El vídeo incluye arranque,cargas y resultado;
el cálculo de combate usa exclusivamente contadores de la fase de combate.
La tasa de campos se calibra a partir de los PTS grabados,≈59,82612Hz, y no
del tiempo de ejecución del ordenador anfitrión.

## Lo que aún falla respecto al objetivo

La Saturn base sigue por debajo de la capacidad necesaria;4MiB también tiene
picos bajo el objetivo en ventanas cortas. No se han resuelto esos cuellos de
botella ni medido por separado todos los costes de preparación/subida gráfica.
Queda OPEN instrumentar y optimizar cachés,descompresión y transferencias VDP1,
conservar cada pose y volver a medir bajo el runtime maestro con cadencia arcade.
Las cargas previas siguen autorizadas, pero no justifican eliminar animaciones.

Los datos y el procedimiento reproducible están en evidence/royal-*-*.json y
tools/analyze_windows.py; las capturas completas permanecen externas a Git.
