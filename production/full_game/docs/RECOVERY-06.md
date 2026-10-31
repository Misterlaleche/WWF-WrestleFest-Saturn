# Recuperación06 — sesión viva en Saturn

Checkpoint parcial sobre05. Nuevo ejecutable: `wrestlefest-session06`.
Alcance: RESET→moneda→START→TAG/ROYAL→roster/compañero→AC0.
El núcleo05 conserva F8C, pero el main Saturn06 se detiene en AC0 y continúa
atendiendo IRQ/input/vídeo. No hay frames de combate ni progresión completa.
La Intro R04 aprobada conserva su propio build; BOOT06 no la sustituye.

## Arquitectura y límites

- Un CD/FRT/SMPC/reloj Saturn. Dos puertos físicos leídos una vez por tick.
  C introduce moneda, START inicia; cruceta/A controlan la selección original.
  Los puertos arcade3/4 quedan sin pulsaciones; multitap pendiente.
- RAM inicial: captura física de los primeros96 bytes de LWRAM en `user_init`,
  antes de cargar CD. RESET654 borra el resto. No se inyectan créditos, RNG,
  equipos ni confirmaciones. Mednafen parte de RNG0, observado, no impuesto.
  DIP explícito DSW1=EF/DSW2=FF; SSP1C3BD4, USP1C3FD4, SR2700.
- Modelo de placa:272 líneas, IRQ2 cada16, IRQ3 en248, ACK140002/140000;
  VBlank activo en bajo fuera de8..247. Latches pendientes respetan SR.
  Es orden de eventos lógico; no emulación de ciclos68000.
- El original publica mapas/paletas/scroll y registros de sprites. VDP2 usa
  bancos/pen originales, sin deduplicar por RGB. VDP1 presenta sprites vivos;
  no hay SEQ.DAT ni reproductor de capturas en este disco.
- LWRAM: PROGRAM512KiB + BACK512KiB. HWRAM contiene sesión78332B,
  plan53248B y staging128KiB; fin de BSS0x06093F40, dentro de1MiB HWRAM.
  VDP2: BG1/BG0 con128KiB cada uno, font128KiB, mapas32KiB. VDP1:
  atlas3115 tiles/398720B y1028 comandos/32896B. No depende de cartucho.
- Cada textura de fondo pasa por staging HWRAM; se comprueban sus128 bytes
  en VRAM. CD verifica tamaño/CRC antes de iniciar sesión. No hay lecturas
  durante selección; los recursos de combate aún no están en este atlas.
- Audio: se registran comandos originales, pero no se sintetiza sonido.
- Rendimiento: la deuda racional37609/39168 se conserva. El arranque/setup
  acumula retraso y después recupera ticks; NO se declara57.44485 sostenido
  ni fidelidad temporal de toda la ruta. Contadores FRT son CPU/32 y16 bits.
  Falta presupuesto de cargas/transiciones y validación en Saturn física.

## Reconstrucción

Requiere los paquetes aprobados restaurados, ROM externa cuyo programa SHA1
es `a726c46f350723ea73158bc78055d17d418b14e2`, SDK suministrado GCC13.2.0 y
xorrisofs. Desde `production/full_game`:

```sh
python3 tools/prepare_session_assets.py --help
python3 tools/prepare_session_assets.py --rom /ruta/wwfwfest.zip --out build/session-cd --header build/session-video/assets.h
YAUL_INSTALL_ROOT=/sdk WF_DISC_OUTPUT=/ruta/nueva/disc bash tools/build_session_saturn.sh
WF_ROM=/ruta/wwfwfest.zip make test sanitize
python3 tools/run_session_checks.py --rom /ruta/wwfwfest.zip --board --sanitize --out /ruta/nueva/board-checks
```

Los assets externos son ignorados por Git; el ZIP de disco contiene los
recursos derivados necesarios. ROM, BIOS y SDK se suministran por separado.
El script de build verifica las fuentes originales antes de copiar headers/CD
a `build/`; exporta los dos propietarios IRQ adicionales en el núcleo SH-2.
`Makefile.saturn` sigue siendo el build TEST03/checkpoint04 separado.

## Pruebas y alcance de las imágenes

`run_session_saturn_checks.py` requiere Mednafen1.32.1, xdotool/Xvfb y el
observador de `modo Royal/wp1980/tooling/mednafen`. Verifica identidad ELF/disco,
ROM del programa cargado, BIOS/emulador, frontera estable y conservación del
reloj. Usa sólo teclas de mando; el observador no escribe memoria invitada.
Opciones: `--cart none|extram1|extram4`, `--royal`; una misma imagen universal.
`--expect-video-error 4` sólo acepta el rechazo temprano de FRONT.TEX corrupto.
`make_bad_resource_disc.py ISO NUEVA_CARPETA --asset FRONT.TEX` altera un byte,
conserva el ejecutable y recalcula EDC/ECC de todos los sectores.

`check_live_picture.py` decodifica bitplanes originales independientemente
del empaquetado VDP2 y compara los píxeles visibles de los fondos en selección.
Excluye rectángulos de sprites y píxeles de texto porque la captura puede
tener otra fase de parpadeo. Sólo se aplica a fondos de paleta estable:
la confirmación TAG anima el pen15 mediante el propietario original$5EAE;
un dump y PNG separados en el tiempo no sirven para comparar esa animación.
No es una equivalencia completa frame a frame.
Las capturas PNG son nativas352x240 con16 píxeles de overscan a cada lado.
Vídeos MOV nativos, MP4 sin descartar PTS, dumps, informes y configuración se
conservan externamente. Git sólo contiene código, informes y hashes.

## Continuación exacta

1. Resolver presupuesto/temporización de RESET, BOOT y cambios de escena.
   No borrar deuda para hacer pasar el contador ni inventar una tasa menor.
2. Handoff explícito de Intro R04 y conexión de entrada/presentadora aprobadas
   a esta misma sesión. Mantener un propietario de cada periférico.
3. Conectar preparación F8C y recursos de combate; extender el atlas/caché
   antes de ejecutar lógica de juego. Después gameplay, CPU y progresión.
4. Repetir pruebas None/4MiB/1MiB y comparación visual; completar multitap,
   backend de audio y verificación física. No presentar AC0 como combate.
