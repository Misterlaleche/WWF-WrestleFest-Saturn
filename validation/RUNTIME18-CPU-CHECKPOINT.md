# RUNTIME-18 — combate directo consolidado, 2026-10-23

Continuación de `7c690d5`, rama `runtime18/direct-combat`. El commit entregado
figura en CHECKPOINT.txt del paquete. No hay cambios publicados en GitHub.

## Resultado y alcance

La partida controlada completa termina por tiempo en el frame **14.409**, en la
frontera original `$1178/$1186 → $11B6`, con resultados de participantes
`8001,8000,8000,8000`. El observador sólo reconoce flags producidos por gameplay
después del ACK; no crea un ganador ni convierte el presupuesto de frames en
éxito. HOST y el replay SH-2 en Mednafen alcanzan ese resultado. Las evidencias
identifican el binario concreto. No se ha acreditado una victoria por cuenta de
tres en la integración completa, ni todas las entradas, parejas o semillas.

La secuencia reproducible usa la semilla de reset cero, RIGHT en frames 60–179,
A en 180–183 y ninguna entrada después. El replay de validación modifica sólo
esas entradas y la semilla inicial: no toca actores, stamina, CPU ni resultados.
El disco normal lee el mando y conserva el ring al finalizar. El informe verde
con el frame codificado se compila exclusivamente en el replay de validación.

La secuencia sin entrada llega al frame 2.496 y se detiene explícitamente en
`$19274`, acción `$59` del actor 0, selector `$0059`. La traza está en
noinput-frontier.log del paquete; no equivale a otra partida completa.
Ningún propietario no implementado devuelve éxito provisional. El texto TIME
UP se escribe en la RAM de vídeo arcade, pero todavía no se publica en VDP;
la pausa y el resultado están probados, esa presentación no.

## Autoridad ROM y cambios

ROM 68000: SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`;
SHA256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.
Musashi ejecuta sus instrucciones originales en HOST como oráculo independiente;
no se enlaza un emulador 68000 al runtime Saturn. Las escrituras a direcciones
de ROM se ignoran en las pruebas que alcanzan esos accesos, conservando las
lecturas originales. No se parchean instrucciones para sustituir gameplay.
En los casos que alcanzan sonido, `$2052` se corta por el contrato del puerto
silencioso, conservando registros; esa frontera no decide acciones ni resultados.

| Ruta contrastada | Casos principales |
|---|---:|
| CPU `$1D206`, incluyendo compañero que coincide con objetivo | 1.024 |
| CPU esquina `$1D31E/$EECE` | 768 |
| Acción `$71/$19F1E`, efectos y colas reales | 1.152 |
| CPU `$1DA7E/$1DE66` | 2.048 |
| CPU defensor `$1C7EC` y entrada `$1C7E2` | 3.584 |
| CPU asistencia `$1F5B2` | 1.296 |
| Acción `$4C`, TAG y rechazo Royal | 2.304 |
| Fin de relevo `$37/$61/$59` | 2.304 |
| Interrupción `$0A` y víctima `$77` | 1.920 |
| Interrupción en salto `$10/$11192` | 1.920 |
| Salida de ring `$68/$69/$6A/$6B` | 9.216 |
| Proyección `$26/$85` | 1.600 |
| Final por tiempo | 128 + 384 rechazos transaccionales |

También se mantienen las matrices de decisiones CPU anteriores, knockdown,
movimiento, entrada TAG, víctimas, escape de pin, paletas y árbitro. Las pruebas
comparan actores, globals, RNG y D0–D7; según la ruta incluyen RAM completa,
FG0, efectos y colas de render. El agregado ROM ejecuta los verificadores del
Makefile; su log conserva los recuentos y resultados completos.

Se corrige `$115D2` para enlaces con los bits altos marcados y dirección física
cero, leyendo los bytes ROM reales. El dispatcher conserva por separado la
semántica de null literal y el centinela `$7F000`. Se corrigen las paletas del
árbitro y el incremento del byte alto de `$169A` durante la cuenta.

El final por tiempo traduce `$263D8..$2641E`: flags, temporizador, mensaje y
nueve actores. La espera `$21E6` recibe D0.W=64 y espera **65** salidas de VBlank,
como el DBRA original. Si falta un callback requerido, permanece bloqueado.
ASan/UBSan detectaron además aritmética de punteros incorrecta en el acceso al
vídeo; se corrigieron los cinco accesos antes de repetir la partida completa.

## Precarga y presentación

HOTSPR.BIN contiene **2.944 tiles**, cabecera de 8.192 bytes y 376.832 bytes de
píxeles originales: 385.024 bytes en disco. SHA256:
`d17ab8e077803b9d491a77c02876c9d64c9bf4609bc5838fc50dc6328fd0d603`.
Se fijan 2.944 de los 3.072 huecos de 128 bytes en LWRAM. Los otros 128 siguen
leyendo el tile original cuando se necesita, sin reemplazos gráficos.

El test de partida completa exige también que la traza de lecturas de
SPRITES.PAK quede vacía. Esta propiedad corresponde a la secuencia indicada,
no a todo el roster. Las fronteras de cabecera 2.044/2.045 y los tamaños
1.204/2.195/4.092 se verifican con el constructor y cargador reales, junto a
bytes, rechazos y fallback. Se conserva la DMA agrupada y el scroll VDP2.
No hay nueva medición de FPS de partida completa; la cifra histórica de
3,23 VBlanks/frame corresponde sólo a frames 31–120 del checkpoint anterior.

## Reproducción

Desde la raíz del árbol incluido, con los cuatro assets locales en
`src/runtime/audit/runtime18-cd/`:

```sh
make -C src/runtime -j4 test test-runtime18-direct \
  test-runtime18-complete-match test-runtime18-hot-sprites \
  test-runtime18-rom-cache test-runtime18-ring-scanline \
  test-runtime18-ring-scroll-cache test-runtime18-yaul-dma
make -C src/runtime test-runtime18-complete-match-asan
python3 -m venv /tmp/wf-rom-oracle
/tmp/wf-rom-oracle/bin/pip install -r validation/requirements-rom.txt
make -C src/runtime -j3 test-runtime18-rom-validation \
  ROM_PYTHON=/tmp/wf-rom-oracle/bin/python
/tmp/wf-rom-oracle/bin/python validation/run-rom.py \
  src/runtime/audit/runtime18-cd/ROM68K.BIN /path/wwfwfest.zip \
  --out validation/results/runtime18/rom
DOCKER=/path/to/docker sh validation/build-runtime18.sh /path/to/sh2eb-elf
make -C src/runtime test-runtime18-cdfs
```

La compilación SH-2 usa GCC 13.2.0 y la imagen Yaul fijada por digest, sin
símbolos sin resolver. El verificador del disco comprueba cada sector raw,
EDC/ECC, IP, ISO9660 y los payloads contra fuente, incluido A.BIN. Eso demuestra
estructura y enlace; la ejecución se acredita por separado con Mednafen.

Para repetir la partida automática en SH-2:

```sh
R18_REPLAY_TEST=1 DOCKER=/path/to/docker \
  sh validation/build-runtime18.sh /path/to/sh2eb-elf
mednafen -ss.bios_na_eu /path/mpr-17933.bin -ss.region_default na \
  -ss.cart none validation/results/runtime18/replay/wrestlefest-runtime18.cue
```

El build normal no define ese modo. Para volver a producirlo, ejecutar el
constructor sin R18_REPLAY_TEST. Mantener juntos BIN y CUE y abrir/grabar CUE.
La BIOS, SDK, Docker y dependencias de oráculo son externas. Los metadatos de
fecha ISO pueden variar al reconstruir; los hashes identifican los bytes
entregados y probados. `validation/package-runtime18.py` empaqueta el commit,
assets y evidencia con SHA256SUMS y verifica el CRC del ZIP.

**Saturn físico con CD-R: no ejecutado. Sin declaración ROM-EXACT, sin
certificación para hardware y sin equivalencia de framebuffer completa.**
