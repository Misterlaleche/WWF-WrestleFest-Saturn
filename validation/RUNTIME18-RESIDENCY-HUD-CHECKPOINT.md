# RUNTIME-18 — residencia gráfica, shake y HUD

Continuación de c569d20. Arranque directo de combate con mando, sin menús ni audio.
No se modifica el núcleo de decisiones del combate ni se sustituyen owners por
resultados artificiales. No hay declaración ROM-EXACT universal ni validación física.

## Implementación activa

- VDP1 conserva los tiles residentes. El contrato genérico compara bytes; el
  runtime usa identidades de tiles ROM inmutables para evitar además la búsqueda,
  lectura y comparación cuando el mismo slot ya contiene el mismo tile. No se
  usan punteros mutables como identidad. Los cambios se agrupan en rangos DMA;
  un aborto invalida el staging modificado y no confirma paletas parciales.
- Las cuatro ventanas CRAM son FG000, sprites100, HUD200, BG300 (hexadecimal,
 256 entradas cada una). Una caché común escribe sólo colores efectivos nuevos;
  bancos idénticos evitan también las conversiones/comprobaciones por entrada.
- `$27012/$27066` leen la tabla ROM `$27048`, modifican el RAM canónico y
  decrementan los contadores. `$26E1A` conserva la inhibición006F.bit4. Se
  ejecutan una vez en la fase de IRQ elegible, antes del worldcommit y ACK.
  Los sprites del frame ya fueron preparados por la cola arcade; no se añade
  un desplazamiento artificial a sus comandos. La cámara vuelve a importar RAM.
- NBG2 muestra FG0 mediante caracteres8x8 y nombres de patrón de una palabra,
  modo auxiliar1. Código12bits, paleta4bits, pen0 transparente, scroll fijo.
  La matriz arcade64x32 se repite verticalmente en el mapa Saturn64x64.
  C2000..C3FFF permanece fuera del HUD: es el buffer de sprites arcade.
- La fuente completa4096glifos ocupa únicamente el banco3 de VDP2. Se carga
  en64lecturas de sector al arrancar; no se reserva128KB adicionales en HWRAM.
  Los mapas de ring/HUD comparten banco1 en0000/2000/4000; caracteres world
  permanecen en bancos0/2. Los ciclos permiten los tres planos y acceso CPU.
  `$264AA` actualiza exactamente los dos colores de parpadeo del reloj.

Autoridad: programa ROM SHA256
`265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`.
Fuente31e12-0.ic33 SHA1`2e9418e372da85ea597977d912d8b35753655f4e`;
FONT8.BIN SHA256`a3045c6193d23c334c6af592cc1dfee029dc12eec810d3e2ce353df69580391a`.
La distribución de ciclos y el formato se contrastan con el manual oficial
[VDP2, secciones3.3 y4.6](https://antime.kapsi.fi/sega/files/ST-058-R2-060194.pdf).

## Evidencia y límites

- CORE48/48 y48/48 con ASan/UBSan. HUD y backend DMA también con sanitizers.
-800casos contra instrucciones originales de shake/scroll;256 de `$264AA`.
-4096glifos/262144píxeles contra el chip original;65536 combinaciones de
  código/paleta en el mapa; pruebas de dirtymap, exclusión de sprites y aborto.
- Cámara2304, world400 y composición D1FC365 casos contra la ROM;
  pipeline independiente R17 y paleta/dependencias de árbitro.
- Las dos partidas completas HOST y ASan terminan en4391 y14409,
  sin lecturas CD de programa ni sprites durante el combate.
- Perfil SH-2 en Mednafen1.32.1: mismos frames30–120,90ACK. c569d20 usa159
  VBlanks; este checkpoint131 (**1,456 VBlanks/frame**,17,6% menos tiempo de
  VBlank; alrededor de1,21 veces el ritmo anterior). No equivale a60fps.
  Se mide el reloj del Saturn emulado; el FPS del host sin throttle no es prueba.
- La primera implementación byte-a-byte empeoró184/90 y se rechazó; su
  evidencia queda conservada. El camino de identidad ROM/bancos obtiene131/90.
- En el perfil exacto se verifican118comandos VDP1/14720bytes de texturas,
  y la fuente128KB más el mapaHUD8192bytes dentro de la misma imagen VRAM VDP2.
  No se confunde un array HOST con la carga real del ejecutable.
- La revisión fresca no encontró fallos Critical/Important/Minor. La prueba
  física y los flujos frontend/monedas/audio no se juzgan como implementados.

El BIN/CUE normal definitivo (`A.BIN`27e619af…) llega en Mednafen a4391,
resultado8001, cuenta0300 y reloj0021, sin owner/dependencia fatal. A lo largo
del combate emite9.870.080bytes de texturas en1374rangos DMA y reutiliza381192
tiles; la línea base enviaba58.658.176bytes. La reducción es aproximadamente83%.
CRAM recibe1024escrituras en total, incluidas las cuatro ventanas de arranque;
no se vuelven a enviar las mismas paletas en los4390frames posteriores.

## Reproducción

Abrir o grabar el **CUE**, con el BIN al lado: una pista MODE1/2352.
Los sectores EDC/ECC, IP y todos los payloads ISO9660 se verifican por bytes.
El disco normal conserva el mando; el replay separado fija semilla0/sin entrada
sólo para validación. El perfil es exclusivamente diagnóstico.

```sh
python3 validation/run-host.py src/runtime --out validation/results/runtime18/core-host
python3 validation/run-host.py src/runtime --sanitize --out validation/results/runtime18/core-sanitizers
make -C src/runtime test-runtime18-yaul-dma test-runtime18-hud test-runtime18-hud-yaul test-runtime18-display-asan test-runtime18-ring-priority test-runtime18-ring-native-asan
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
make -C src/runtime ROM_PYTHON=/ruta/python test-runtime18-display-rom test-runtime18-camera-rom test-runtime18-referee-rom
python3 src/runtime/tests/r18/verify_hud_font_rom.py /ruta/wwfwfest.zip
python3 validation/run-rom.py src/runtime/audit/runtime18-cd/ROM68K.BIN /ruta/wwfwfest.zip --out validation/results/runtime18/rom
DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_REPLAY_TEST=noinput DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

Para los oráculos, instalar `validation/requirements-rom.txt`; machine68k se
usa sólo como oráculo HOST. El paquete incluye las cinco fuentes de datos
canónicas y el ZIP arcade de referencia local. Para regenerar FONT8.BIN:
`python3 src/runtime/tools/build_r18_font.py ROMZIP src/runtime/audit/runtime18-cd/FONT8.BIN`.
SDK/Yaul fijado, Docker y BIOS son externos. Los timestamps ISO pueden variar;
el ejecutable A.BIN debe reconstruirse idéntico al del disco probado.

**Saturn físico/CD-R: pendiente de la prueba del usuario.**

## Hashes del disco normal

A.BIN `27e619af45689cb50314a556fe32a360c5d947f84951961289ec0ebf3f88e235`.
BIN `37474f25cc4688be1cd568e4d8aec7e961cd036d4a50a104a1073544fbb5192e`.
CUE `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`.
