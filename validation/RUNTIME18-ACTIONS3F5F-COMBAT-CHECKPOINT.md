# Continuación del combate desde f88acad

Base: `f88acadb8c3c95a3e4a1fe10eaf6e3e3960dbf1a`.
ROM68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
Sin frontend, audio ni entrega de imágenes de disco.

El recorrido **sin entrada completa un combate en HOST y Mednafen**, en el
frame3008, por cuenta fuera de20 y con la presentación original RING OUT /
GAME OVER. No se fuerza el resultado, la stamina ni el final. El recorrido
adicional con mando llega3373 y aún bloquea en raw1C; no se declara completo.

## Implementación y evidencia directa

Cada owner nuevo parte de una observación natural, archivada en
`src/runtime/tests/r18/*snapshot_f88acad.bin`, con procedencia y hash en su
manifest. Los casos derivados se distinguen expresamente de esas capturas.
Son observaciones HOST del modelo C: el exportador conserva los campos
representados y deja a cero el resto; no son volcados de la placa arcade.
Los oráculos ejecutan los bytes originales con machine68k; no sustituyen
rutinas de gameplay por retornos favorables. Los adaptadores HOST hacen los
efectos de vídeo y texto realmente requeridos. La entrada vblank de la placa
es un dispositivo modelado en el oráculo de transición.

| Owner | Comparaciones ROM que pasan |
|---|---:|
| raw3F, `$1836E` y efecto18 | 1060 |
| raw5F, `$19536` | 591 |
| main3, `$11D8E` | 1283 |
| CPU `$1C458` | 1027 |
| CPU `$1C9B2` | 1317 |
| CPU espera B6.bit4 | 1034 |
| raw03, `$12B16` | 1248 |
| state4 `$1B706` | 1330 |
| state4 `$1BB4C` | 1538 |
| raw18, `$14F08` | 1380 |
| state4 `$1BDE8` | 685 |
| raw6C, `$19C48` | 185 |
| raw38, `$17B12` | 840 |
| F98C core hasta `$26E66`, con bloqueo explícito | 265 |
| F98C completo y transporte de escenas0/2 | 29, más5 errores de recursos |
| Árbitro RINGSIDE `$1FD5E`, frontera `$1F934` | 100 |
| CPU ringside `$1C6B6` | 299 |
| CPU `$1E9A6/$1F084`, bancos1/9 | 2241 |
| CPU persecución exterior `$1CA2E` | 517 |
| state4 `$1BF1C` | 1922 |
| state4 `$1BA92` | 1957 |
| state4 `$1B288`, aliases06/07 | 1725 |
| raw2F y continuación raw81 | 5548 |
| state4 `$1BFA6` | 1685 |

Las comparaciones de acciones incluyen RAM `$1C0000..$1C1FFF`, FG0 y
D0–D7; cada script declara sus regiones exactas. No incluyen A0–A7/CCR ni
demuestran por sí solas ejecución Saturn. Las continuaciones aisladas de
animación son llamadas al owner; no se presentan como frames completos.
El oráculo raw2F prueba resolver en escenas0,1,2,3,5,6; no acredita escena4.

La transición ya conecta reset de colas y caches, mapas originales, paletas,
recursos VDP2 y actualización de cámara dentro del runtime. Conserva el orden
`$26E66 → $2A06 → $C1EA`; la vuelta al ring añade `$206FE/$1004C`.
Las escenas distintas de0/2 siguen produciendo un bloqueo explícito.

## Reproducir el recurso de ringside

El archivo nuevo no contiene gráficos dibujados de nuevo: lo genera el mismo
compilador de recursos desde la ROM original. Se conserva el archivo de
ring0 del checkpoint base y se añade el de ringside como `R18MAP2.BIN`.

```sh
python3 src/runtime/tools/build_r18_graphics.py --rom /ruta/wwfwfest.zip \
  --out /tmp/wf-ringside --mode 2 --palette-mode 2
cp /tmp/wf-ringside/R18RING.BIN src/runtime/audit/runtime18-cd/R18MAP2.BIN
```

95978 bytes; SHA256:
`979b622c418307f90b1ce98524db7bade230adf399bd13e6fc3fe4adc699acf0`.
El buffer `$00201000..$00220000` lo contiene sin invadir la caché ROM.

## Recorridos y precarga

Sin entrada: todos los puertos permanecenFF, RNG inicial0. En el recorrido
HOST equivalente se ejecuta raw3F desde703/slot1 y raw5F desde2085/slot0.
Se alcanza la condición original `$11B6` en3008: resultado8001, cuenta0014,
timer0023. La presentación `$11CE..$122B` espera exactamente131 VBlanks.

Con mando: RIGHT en frames60..179 y A en180..183, después ninguna entrada.
El siguiente bloqueo es3373/slot1, raw1C, en el dispatcher `$12614`.
Esto limita las entradas jugables que se pueden declarar verificadas; no
se presenta el port completo como terminado.

La política HOT usa la unión de demandas reales del recorrido completo y
la ruta controlada hasta3373, retirando hints de sprites históricos ajenos
a esos recorridos corregidos. Son4883 tiles originales,625024 bytes de
payload y237 slots fríos disponibles. El archivo conserva su cabecera16KB
y el fallback exacto para otros IDs. Se precargan206 páginas ROM,210944 bytes,
dentro de los212992 reservados; el trace también conserva páginas calientes
históricas cargadas durante el arranque.

Con la política anterior, el recorrido natural corregido hacía27 lecturas
ROM y4175 lecturas de sprites. La política final termina el mismo combate
con **ROM=0 y SPRITES=0 lecturas durante gameplay**. La carga de escena2
desde CD es independiente de esos contadores: no se afirma cero CD total.
Los hashes y procedencia están en `residency-consolidation.json`.

## Pruebas finales

`make -j4 test`:36 ejecutables, exit0. La prueba histórica de controller16
se actualiza porque `$1E9CC` ya está implementado: ahora comprueba bank9 y
el bloqueo auténtico `$1F802` cuando el fixture no suministra un target.
El oráculo nuevo valida por separado2241 contextos, incluidos216 del banco1.

El combate natural pasa normal y con ASan/UBSan, ambos3008 y cero lecturas
ROM/sprites. ASan detectó una formación intermedia de puntero fuera del array
en transición; se corrige usando `background+(dest-0x80000)`. El oráculo29/29
más5 guards y la prueba completa con sanitizadores vuelven a pasar.

Pasan también CDFS sobre el disco final, caché ROM, HOTSPR y sus ocho límites,
ring nativo, recorte y separación de stamina/HUD con sanitizadores. El planner
conserva1540 casos de equivalencia, todos65536 IDs y fallos de capacidad.
Los nuevos owners cuentan además con revisión y reejecución independientes.
No se atribuye un PASS al target de combate completo con mando.

## SH-2 y Mednafen

Entorno Yaul fijado por digest de contenedor en `validation/build-runtime18.sh`,
SDK extraído del checkpoint base y GCC13.2.0; hashes de sus componentes
en `RUNTIME18-ACTIONS3F5F-VERIFIED.json`;
sin símbolos sin resolver. El linker conserva el aviso LOAD/RWX del SDK.
`___end=$060FB0F4`:20236 bytes hasta`$06100000`; este dato no certifica hardware.
Programa A.BIN SHA256:
`0f357e7d92a355c8eae4f92f5260d1cad6b997dfa3a2960159074a3161677dbe`.

Mednafen1.32.1, `-nothrottle 0`, fuente y executable exactos. Observación
solo de lectura, con4096 bytes de instrucciones coincidentes:

- frame3008, cuenta20, timer35, resultado32769, `finished_presentation=1`;
- `fatal_owner=0` y `fatal_dependency=0`, presentación131 VBlanks;
- cámara RAM y adaptador coinciden en885/512 en ambos planos;
- 125 comandos VDP1,122 sprites normales:4000 bytes de comandos y13952 bytes
  de texturas coinciden con la VRAM emulada;
- fuente HUD131072 bytes y mapa8192 bytes coinciden en una misma imagen VDP2;
- ambos bancos de caracteres del ring contienen86656 bytes exactos de los677
  tiles de escena2, además del blob completo95978 en LWRAM;
- las206 páginas ROM y625024 bytes HOTSPR coinciden en esa misma LWRAM.

3008 frames consumen3464 VBlanks de gameplay: aproximadamente52,1fps medios
tomando60Hz nominales, incluyendo cambio de escena y esperas. No demuestra
60fps sostenidos ni una mejora A/B respecto al tramo corto del checkpoint
anterior. No hay prueba en Saturn físico con CD-R ni equivalencia universal
de framebuffer arcade: **no se declara ROM-EXACT ni aptitud para hardware**.

## Vídeo y reproducción

MP4 real de la ventana Mednafen,604×480 a30fps. Se conservan los segundos
17,1..82,4 de la captura original:65,3 segundos, sin aceleración, interpolación
ni reconstrucción HOST. Se retiran BIOS y espera estática posterior al final.
La captura conserva el tiempo repitiendo la imagen anterior si pierde una
muestra; esos30fps son del vídeo y no una medición de FPS nativos.
El fixture `result_capture_entry.c` mantiene los mismos inputs/seed y el final
real; únicamente omite el marcador diagnóstico verde del replay.

```sh
cd src/runtime
make -j4 test
# Python debe disponer de machine68k y capstone como los oráculos anteriores.
make ROM_PYTHON=python3 test-runtime18-continuation-rom
make test-runtime18-noinput-complete-match test-runtime18-noinput-complete-match-asan
cd ../..
R18_CAPTURE_RESULT=1 R18_REPLAY_TEST=noinput \
  sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

La última orden genera una imagen interna para Mednafen. La entrega de este
checkpoint contiene vídeo, capturas, informe, hashes y parche; ningún BIN/CUE.
Los logs y JSON detallados quedan en `validation/results/runtime18/action3f-5f/`.
