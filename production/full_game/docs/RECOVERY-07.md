# Recuperación del checkpoint07

Base: a295ce6026891a6b94ae6f55c3e27a3cf24aafd0.
Rama: feature/wrestlefest-full-game-rom-exact. Originales intactos.
Alcance: sesión nativa hasta primeros frames de combate, NO beta completa.

## Uso

Descomprimir disco-parcial y guardar BIN/CUE juntos. Abrir CUE en Mednafen1.32.1,
BIOS/SDK/ROM del usuario por separado. Sin cartucho o4MiB;1MiB usa el camino base.
C introduce moneda antes del combate, START acepta, cruceta/A seleccionan modo,
luchador y compañero. En combate cruceta y A/B/C son entradas de acción nativas.
Para jugar esta integración, dar C y START durante inicio/selección. La ruta
attract también entra al combate, pero todavía no está integrado el regreso a
selección por crédito/START después de entrar en gameplay. Resultados/finales
siguen sin enlazar; un límite de propietario conserva el fallo explícito.

## Construcción

Restaurar el ZIP de fuentes completo y recursos07 en build/session-cd;
los tres atlas y su header pueden regenerarse desde la ROM verificada:

```sh
python3 tools/prepare_session_assets.py --rom "$WF_ROM" --out build/session-cd --header build/session-video/assets.h
python3 tools/prepare_combat_assets.py --source /ruta/a/recursos07 --out build/session-cd
YAUL_INSTALL_ROOT=/sdk WF_DISC_OUTPUT=/ruta/nueva bash tools/build_session_saturn.sh
```

La segunda orden coteja diez hashes HW1 antes de copiar. HOTSPR es R18D de HW1,
no el R18P de TAG R3. El build incorpora owners originales sin editarlos; las
proyecciones en tools/build_session.py verifican coincidencia única antes de
cambiar enlaces de transporte o reservas. SH2 GCC13.2.0, flags y hashes en
core-build.json; imagen Docker de construcción y emulación en configuration.json.
No usar ROYINIT, semilla ni estado directo para arrancar esta sesión.

Las fuentes de frontend y su inclusión SDK se preparan por build_session_saturn;
la salida es wrestlefest-session07.bin/cue. Se conserva también ELF/mapa y el
archivo de núcleo. Los recursos y las capturas quedan fuera de Git; sus hashes
sí están publicados. El ZIP de recursos evita depender de una caché efímera.

## Verificación

```sh
WF_ROM=/ruta/wwfwfest.zip make test
WF_ROM=/ruta/wwfwfest.zip make sanitize
python3 tools/run_session_checks.py --board --sanitize --rom "$WF_ROM" --out /ruta/nueva
```

Harness native: tools/run_session_saturn_checks.py, --combat, --royal opcional,
--cart none/extram1/extram4, --cue y --elf congelados coincidentes. Mando real
xdotool; observer de sólo lectura y Mednafen1.32.1 full. --attract evita crédito
anticipado y verifica el arranque completo. Cada informe guarda su harness/hash.
Los MP4 conservan todos los PTS de los MOV; no certifican por sí solos el ritmo.
La prueba negativa altera FRONT.TEX en otra copia y debe fallar antes del tick0.

## Memoria y reloj

Antes de F8C: PROGRAM512KiB y BACK512KiB ocupan LWRAM. HWRAM contiene RAMoriginal,
registros/runtime y workspace de vídeo. Después: escena00201000, ROM00221000,
staging00255400 y tiles00261400 según mapa HW1. Workspaces de escenas y combate
son excluyentes y se limpian con salida oculta. No se reutiliza el runtime.
El atlas4MiB sólo se carga después de validación completa de cartucho.

Reloj37609/39168 único. Cargas únicamente en clears síncronos y bind26E66,
además de la cesión de recursos. No se pausa fade ni espera lógica. Pacer conserva
fracción/deuda y registra load_fields separadamente. La preparación no borra
ningún sobrecoste que ya haya ocurrido durante una escena activa.

## Continuación exacta

1. Título/attract sin crédito alcanza76 ticks de deuda tras BOOT(max3).
   Perfilar CPU/VDP1/esperas/CD con wf_combat_work/wait y campos físicos; recuperar
   velocidad de Entrada/Presentadora/combate sin borrar deuda. O2 selectivo o
   residentes adicionales requieren nuevo presupuesto de RAM; quedan42860B.
2. Completar comparación sincronizada de sprites y tiempos con escenas aprobadas.
   El fondo de Presentadora pasa;52 diferencias de Entrada con captura desfasada
   no acreditan un bug ni equivalencia. No ocultar ese resultado exploratorio.
3. Enlazar scene_resume y owners originales de postmatch: resultados TAG/Royal,
   progresión, regreso del attract y moneda/START con gameplay activo.
4. Integrar IntroR04 completa, multitap, audio original y validación física.
   No sustituir owners por callbacks vacíos ni sembrar equipos/RNG.

Se conservan todos los checkpoints, fuentes, vídeos, ZIP aprobados y recursos.
