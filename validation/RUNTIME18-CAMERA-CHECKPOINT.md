# RUNTIME-18 — cámara compartida desde 426ed17

La cámara del ring, el streaming del mapa y los sprites comparten ahora los
cuatro WORDs originales `$17E6/$17EA/$17EE/$17F2`. Antes, `$26936/$2983C`
sólo actualizaban el adaptador de sprites, dejando el ring en su posición
de arranque. En el frame120 sin entrada, los sprites usaban451,578 y el ring
480,560. Ahora ambos usan451,578, sin añadir desplazamientos artificiales.

## Evidencia e implementación

ROM68000 SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.
Las fases importan los scrolls, destino `$1804/$1806` y escena `$007E` de la
RAM canónica, ejecutan las operaciones originales y publican los scrolls.
Se conserva el orden del frame; no se cambia la proyección `$247C` ni el
signo vertical de `$26E1A` (`2048-scrollY`).

`$2983C` compara sin signo sólo el canal A. Cuando alcanza un límite escribe
ambos canales; si A está dentro, deja B intacto. Se corrige la anterior
comparación con signo y el clamp independiente de B.

El nuevo oráculo ejecuta la ROM intacta en Musashi: **2.304 casos**, con
seguimiento, límites y composición de ambas fases; siete escenas, todas las
clases de cámara, estados del adaptador deliberadamente obsoletos, límites
unsigned, actores y D0–D7. La comparación falló antes del cambio por conservar
scrolls canónicos antiguos y pasa después. La regresión de combate exige
igualdad entre adaptador y RAM en cada presentación de ambas partidas.

El fixture HOST del frame activo se inicializa ahora en la RAM canónica con
los mismos320,512 que ya usaba el fixture ROM. Se mantienen sin cambios los
29 tuples esperados y el ACK. La revisión independiente no deja hallazgos.

Pasan `make test`, las48 regresiones HOST y sus48 variantes ASan/UBSan,
las dos partidas completas con y sin sanitizadores, los tests de transporte,
el pipeline ROM original R17 y el agregado ROM R18, incluido el oráculo nuevo.
La exportación de fuente vuelve a completar el combate y genera un A.BIN
normal idéntico byte a byte al que se ha inspeccionado en Mednafen.

## Discos y Mednafen

Disco normal con mando, arranque directo, sin menús ni audio:
`validation/results/runtime18/wrestlefest-runtime18.cue` y `.bin`.

- A.BIN SHA256: `406bf8990c22ebcfd3c8734b5dee71688bf019c13b4c14b451d51d72bfe38568`.
- BIN SHA256: `71cf3b028ef81ab54b2b60c7d59b2c506b984bc9313ab28878b4aaa63d7f7119`.
- CUE SHA256: `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`.

Replay separado sin entrada, semilla cero:
`validation/results/runtime18/replay-noinput/`.
A.BIN SHA256: `8995d7632340d8c33297d2c9534a3b6f996ee5a08c6aa9bf0dbed3273cff14ed`.
Mednafen alcanza el final real4391: resultado8001, cuenta0300, reloj0021,
sin errores. El disco normal también se arranca y se inspecciona en movimiento.
El observador identifica4096 bytes del ejecutable exacto y sólo lee memoria.
Los offsets del estado se extraen compilando `runtime18-observer-layout.c`
con el ABI SH-2 del SDK, sin instrumentar el juego. Los registros canónicos y
el adaptador coinciden, por ejemplo, en frame4241 del replay (544,560) y
frame311 del normal (523,564). Las capturas muestran el desplazamiento común.

Las capturas XWD se decodifican con FFmpeg: esta superficie Xvfb usa24 bits
por píxel y filas rellenadas; el antiguo conversor que suponía32 bits
producía bandas de color ajenas al juego.

## Reproducción y alcance

Instalar `validation/requirements-rom.txt` en Python3.12. Desde la raíz:

```sh
make -C src/runtime test
python3 validation/run-host.py src/runtime --sanitize --out validation/results/sanitizers
make -C src/runtime ROM_PYTHON=/ruta/venv/bin/python test-runtime18-rom-validation
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match
make -C src/runtime test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
DOCKER=/ruta/docker sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_REPLAY_TEST=noinput DOCKER=/ruta/docker sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

Abrir/grabar el **CUE**, manteniendo el BIN al lado. Las comprobaciones de
sectores, EDC/ECC, IP, ISO y payload se conservan por separado de la ejecución
en emulador. El paquete contiene fuente consolidada, assets canónicos,
discos, hashes y evidencias. La reconstrucción del A.BIN debe ser idéntica;
los timestamps ISO pueden cambiar. SDK, Docker y BIOS son dependencias externas.

Esta corrección no acredita velocidad arcade ni igualdad completa de
framebuffer. Se conservan las limitaciones de precarga del checkpoint anterior
(11.984 lecturas frías en el replay sin entrada). La prueba de este nuevo disco
en Saturn físico con CD-R corresponde al siguiente ensayo del usuario.
No se declara ROM-EXACT ni hardware validado. El frontend y el sonido ya
probados anteriormente permanecen fuera de este ejecutable de combate.
