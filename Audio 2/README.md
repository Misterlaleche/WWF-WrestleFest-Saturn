# Audio 2

Instantánea del trabajo de audio nativo SCSP, publicada el 1 de noviembre de 2026.
Relacionado con [issue #2](https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/issues/2).

**Estado: desarrollo incompleto. No es una entrega de audio terminada ni una imagen para Saturn.**
R04 se conserva intacto como referencia de software. Los directorios existentes del repositorio no se sustituyen.

## Qué se conserva

- `base/`: driver SCSP/68000, secuenciador, DSP, PCM/SFX, herramientas y pruebas del árbol de audio.
- `experimentos/native18k-standalone/`: generador consolidado, fuentes legibles y comprobaciones con identidades fijadas.
- `experimentos/z80-inline-tabla/`: fuente exacta del último prototipo medido, intérprete generado y resultados HOST/SH2.
- `experimentos/pcm-servicio/`: preparación anticipada de PCM3 y servicio intercalado, todavía aislados.
- `experimentos/loader-abi4/`: cargador con cuatro segmentos y validaciones previas a la escritura.
- `integracion/master-congelado/`: frontera de integración de audio y propiedad de memoria del master medido. Es una referencia parcial, no el port gráfico completo.
- `validacion/` y `evidencia/`: observadores, pruebas negativas, catálogo, informes y límites de las mediciones.
- `FUENTES-SHA256.json`: inventario de bytes y procedencia; `verificar_paquete.py` lo comprueba sin ROM ni compilador.

## Resultado que impide entregarlo como final

El último ensayo de 9600 cuadros del emulador registra **361 descartes FM**, 1664 avisos de retraso y `game_frame=457`.
El gate de audio falla. Que el módulo siga en RUNNING y marque error 0 no acredita reproducción completa.
Estos recuentos describen esta ejecución; no comparan cargas gráficas equivalentes entre variantes.

Las mejoras del PCM tienen pruebas separadas en simulación 68000. No constituyen prueba conjunta de música,
efectos y gráficos en Saturn. Ver `ESTADO.json` y `evidencia/pcm-servicio/all18-resumen.json`.
El último informe aislado conserva seis avisos de retraso (dos en cada comando 0x07, 0x0E y 0x12), aunque
no registra underruns. Esta variante tampoco se declara terminada.

Antes de crear BIN/CUE faltan: eliminar los descartes, integrar el PCM nuevo, comprobar las 18 músicas y los
98 comandos con mezclas/transiciones/fade y medir el impacto frente al mismo juego sin audio.
Después seguirá siendo necesaria la prueba física del usuario.

## Uso por el Codex gráfico

Leer `INTEGRACION.md` antes de importar. Conservar su runtime gráfico actual y R04. Importar únicamente
cambios de audio revisados, con propiedad explícita de memoria. No copiar el master congelado sobre el port.
No mezclar el módulo ABI3 de las medidas con el cargador ABI4 sin recompilar el par completo.

## Reproducción y recursos externos

Este paquete conserva código y evidencia, sin ROM, BIOS, PCM, paquetes de sonidos, ELF con ROM incrustada ni BIN/CUE/ISO.
Los recursos originales se generan localmente desde las ROM del usuario. Los compiladores/SDK y el ELF master
de referencia no se distribuyen aquí; sus identidades están registradas en los informes.
Consultar `COMPILACION.md`: el generador standalone es la base reproducible consolidada; algunos scripts
históricos conservan rutas del laboratorio y no son comandos portables listos para ejecutar tras un clone.

Los informes históricos contienen resultados de prototipos anteriores. `ESTADO.json` y este README prevalecen
para el estado de esta publicación. Las licencias de terceros se conservan junto a sus fuentes.
