# Integración E02 sobre las correcciones C

E02 reduce el trabajo de ordenación cuando todos los sprites comparten clave,
evita validar dos veces una publicación ya comprobada y lee las muestras PCM
alineadas como palabras. El pack de escenas conserva sus siete contenidos
exactos y ocupa menos memoria, dejando 145 bloques de caché en el frontend.
Se mantienen las correcciones C de BIOS, SMPC, attract/START y diagnóstico.

Los hashes de fuentes y los recibos están en `evidence/e02-20261113`. La imagen
manual se construye con `make WF_FRONTEND=1`, sin `WF_AUTORUN`, `WF_CAPTURE`
ni `WF_PROFILE`. Cuatro controladores fríos usan `-Os`; las reservas de pila,
música y JoWork no se reducen. Los assets deben corresponder al lock D03/E01
actual; el lock B12 se conserva para reconstruir los discos anteriores.

## Medición y alcance

| Prueba | C de referencia | E02 |
| --- | ---: | ---: |
| Ticks del combate Royal de referencia | 14.409 | 14.409 |
| Campos, límites WFT1 del combate | 15.217 | 15.148 |
| Campos sobre calendario nativo de 15.075 | 142 | 73 |
| Campos observados entre ticks comunes 8.006–9.993 | 2.160 | 2.137 |
| Cadencia nominal en esa ventana | 95,804 % | 96,835 % |

Cada variante tiene una captura completa de derrota en Mednafen stock,
arranque en frío, sin estados cargados ni escrituras externas al juego.
Los 2.412 bytes de actores finales, RNG y dueño del resultado son idénticos;
ambas guardan el récord y vuelven al título visible y activo. No aparecen
fallos de juego, SMPC, acceso nulo ni errores/underruns de audio. Las ventanas
intermedias son observaciones asíncronas con incertidumbre de campos; no se
interpolan tiempos ni se utiliza la velocidad del ordenador como la de Saturn.

Las rutas nativas Royal/TAG conservan 182 artefactos de estado, comandos y
renderizado. Pasan el sorter real con 5.712 casos y controles negativos,
la publicación de estado con 6.158 casos, las interrupciones en ambos puntos
de publicación y las pruebas existentes del SDK con DMA diferido. La lectura
PCM conserva 12.061.722 muestras en 32.830 casos, además de comandos y arranques.

Esto acepta la mejora de ese recorrido, no la velocidad arcade de todo el
juego. La prueba de victoria C y los controles manuales C validan los fixes
compartidos, pero no sustituyen una captura de todas las rutas del binario E02.
El fundido de música y la retención compacta de escenas son experimentos
separados: no forman parte de esta promoción.

## Comprobaciones reproducibles de la promoción

```sh
python tests/test_sdk_uniform_sort.py --output build/e02-sort
python tests/run_jo_publish_state.py \
  --reference tests/fixtures/sdk/wf_jo_texture_cache_before_prevalidated.h \
  --output build/e02-publish-state
python tests/run_jo_publish_prevalidated.py --output build/e02-publish-seams
```

Los recibos conservan los hashes de fuentes y del build medido. El estado de
la Saturn física, la causa de los seis minutos de carga y las dos anomalías
gráficas descritas por el usuario permanecen abiertos.
