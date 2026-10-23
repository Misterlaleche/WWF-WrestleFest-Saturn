# Movimiento CPU y llave $22 — 23 de octubre de 2026

Continuación de `a4be15fb09e1e5fe2f83a4410eca4d0bce39162a`. Se integran
el movimiento `$1D7B6`, su frenado `$1DA66` y la acción `$22` con sus
continuaciones alcanzadas. La entrega es un vídeo y dos capturas de
Mednafen, sin otro paquete BIN/CUE. La partida sigue incompleta.

## Implementación y evidencia ROM

El estado CPU2 no tenía implementación: el primer caso natural, en669,
debía retornar desde `$1D7BE`, pero el port bloqueaba. La implementación
incluye esperas, umbrales, reacciones ponderadas, barrido de nueve actores,
rescate TAG, decisión `$1DE66` y continuación de estado5. Estado3 resta dos
a la magnitud bajo la condición original. El controller16 reutiliza el
helper `$1F15C`, incluyendo el scratch `$15E4` omitido por su duplicado.

2263/2263 comparaciones contra ROM intacta: un snapshot natural conservado
de669, más2262 casos derivados. Se comparan los nueve actores, RNG, regiones
globales enumeradas por el script y D0–D7; no toda la RAM. Se ejercitan1467
llamadas RNG,71 continuaciones estado5,12 decisiones y40 casos de estado3.
Las19 anclas de ramas obligatorias se alcanzan. Pasa `test_state5` existente.

La tabla de acciones `$12614[$22]` apunta a descriptor `$15B6C` y callback
`$15B8C`. Se incorporan sus seis poses y periodos, adquisición, retención,
liberación y resultado, además de las rutas alcanzadas de `$111C8`, bonus
`$90D6`, limpieza `$21358`, activación de compañero `$215B6` y texto GIVE UP
de `$2503C`. Son transcripciones C nativas; el oráculo68000 no se enlaza en
Saturn. Solo se intercepta la salida de sonido `$2052` en esas pruebas.

1946/1946 casos, incluido el snapshot natural460/slot5; toda la RAM
`$1C0000..$1C1FFF`, FG0 de16KiB y D0–D7 coinciden. Se alcanzan26 sitios de
rama. La mayoría son fixtures derivados para probar límites, no combates
naturales. Los punteros usados deben identificar actores y el tier del
compañero CPU se acota a0–2; no se declara cobertura de direcciones inválidas.

## Precarga consolidada

Las trazas de la nueva ejecución muestran214 lecturas de tiles sin entrada
y318 con control antes de consolidar. Su unión añade520 IDs a los3993
históricos, sin retirar ninguno. Los4513 tiles se copian byte a byte de
`SPRITES.PAK`; no se generan ni retocan gráficos.

El payload ocupa577664 de655360 bytes de caché; quedan607 slots de fallback.
La tabla ordenada de IDs usa16384 bytes separados, conservando compatibilidad
con cabeceras4096 y8192. Ocho límites1204/2044/2045/2195/4092/4093/4513/5119
verifican builder y loader, bytes, capacidad, packs incorrectos y recarga
fallida. Los fallos previos de builder y loader se conservan como evidencia.

Se añaden cuatro páginas ROM observadas258/263/270/271 a las172 anteriores.
La reproducción HOST final registra **ROM=0 y SPRITES=0 lecturas CD durante
703 frames sin entrada y560 con control**. El probe comprueba ahora también
residencia al terminar una ventana limitada, además del endpoint de partida.
Esto no demuestra que futuras acciones nunca necesiten datos adicionales.

## Resultado integrado y límites

Sin entrada: el aliado alcanza el frame700 en estado`8002`, X769/Y389,
energía106/106 real/visible. El bloqueo669 desaparece. El siguiente bloqueo
HOST es703, slot1, acción`003F`, dependencia`$12614`.

Con RIGHT60–179 y A180–183: el bloqueo460 desaparece. En550, slot5 ejecuta
`4022`, mientras el jugador conserva92/92 y el aliado106/106. El siguiente
bloqueo HOST es560, slot0, acción`005F`, dependencia`$12614`. No se sustituye
ninguna de esas acciones pendientes por éxito, espera ficticia o salto.

Los endpoints históricos4391/14409 no se atribuyen a este árbol. No se ha
completado un combate corregido. Quedan `$3F` y `$5F` como próximos owners
demostrados, antes de retomar el frontend.

## Mednafen y vídeo

Dos builds SH-2 GCC13.2.0 del árbol final enlazan sin símbolos pendientes.
El SDK conserva sus avisos LOAD/RWX. Los finales de programa dejan33644
bytes hasta`$06100000` en la captura700 y32492 en el fixture con perfil550;
no equivalen a una certificación general de memoria o hardware.

Mednafen1.32.1 ejecuta ambas imágenes. Los fixtures hacen todos los pasos
reales y se detienen después de ACK700/550 para leer un estado estable;
no fuerzan posiciones, energía, animaciones ni resultado. Los observadores
identifican la WRAM con4096 bytes del ejecutable exacto y solo leen memoria.
Admiten los sufijos reales `.lto_priv.N` del linker, sin alterar sus mapas.

En700 se verifican124 comandos sprite VDP1; en550,126. Comandos y texturas
coinciden con la VRAM emulada; también la fuente y el mapa HUD completo VDP2.
En700, ring114918 bytes,176 páginas ROM y577664 bytes de tiles coinciden
en la misma LWRAM real. Ambos estados tienen fatal_owner=0.

El vídeo procede de32 segundos de lectura XWD de la ventana real, con
Mednafen `-nothrottle 0`. Se entregan los segundos19–32, retirando el arranque
BIOS; no hay aceleración ni reconstrucción HOST. La captura codifica30fps
y mantiene la imagen previa si pierde una muestra:32 frames retenidos en
los32 segundos completos, registrados en el sidecar. Esos30fps son del vídeo,
no una medición del juego. La pausa final es el stop diagnóstico en550.

El perfil SH-2 de450→550 registra100 frames/111 VBlanks, aproximadamente
54fps a60Hz, y cero llamadas de lectura de archivo en esa ventana. Es una
medición de ese tramo, sin afirmar60fps sostenidos o una mejora A/B de toda
la partida. No se ha probado Saturn físico con CD-R ni igualdad universal
de framebuffer arcade; no se declara ROM-EXACT ni aptitud de hardware.

## Pruebas y reproducción

`make -j4 test`:36 ejecutables, exit0. HUD y recorte VDP1 pasan normal y
con ASan/UBSan; la prueba de caché de sprites también pasa. El corpus final verifica
22675 comparaciones de framebuffer. Una revisión independiente reejecuta
ambos oráculos y los ocho límites de precarga, sin defectos importantes de
producción. Detectó una regresión del replay CPU aislado al enlazar HOTSPR
nuevo con loader antiguo: se corrige reconstruyendo su pack histórico desde
el mismo commit; vuelve a alcanzar el bloqueo esperado703.

Con la ROM y assets originales en `src/runtime/audit/runtime18-cd`, un
Python con `machine68k` y el SDK local:

```sh
make -j4 -C src/runtime test test-runtime18-hot-sprites test-runtime18-sprite-crop
make -C src/runtime test-runtime18-hud-stamina-separation test-runtime18-hud-stamina-separation-asan test-runtime18-sprite-crop-asan
make -C src/runtime ROM_PYTHON=/ruta/python test-runtime18-cpu-state2-rom test-runtime18-action22-rom test-runtime18-hot-boundaries

python3 src/runtime/tests/r18/allies_run_probe.py /tmp/wf-noinput --frames 703
python3 src/runtime/tests/r18/allies_run_probe.py /tmp/wf-controlled --frames 560 --attack

R18_CAPTURE_STOP=700 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 R18_PROFILE_START=450 R18_PROFILE_STOP=550 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

El nombre del target de límites de cabecera es
`test-runtime18-hot-boundaries` (véase Makefile). Los snapshots binarios
y manifests quedan versionados; el test CPU congela sus dependencias en
`a4be15f` para evitar que un owner posterior cambie su significado. Los
replays con `--require-complete --frames 20000` deben seguir fallando en
703/560 hasta implementar los owners indicados; no son pruebas verdes.

Evidencia completa en `validation/results/runtime18/movement-action22/` y
el oráculo CPU en `validation/results/runtime18/cpu-state2-investigation/`.
`checkpoint-summary.json` conserva los hashes y las observaciones reales.
