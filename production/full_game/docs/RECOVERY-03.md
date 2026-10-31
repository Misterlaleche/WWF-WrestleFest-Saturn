# Recuperación del checkpoint03

Disco parcial de integración MODE/ROSTER; no beta completa. Un main, VBlank,
SMPC y reloj compartidos. TEST03 conserva sus recursos, capas y parches. El
estado terminal conserva modo/slot y espera la integración del propietario ROM;
no arranca combate ni convierte el slot visual en ID de luchador.

## Herramientas y recursos

- SDK proporcionado Yaul/GCC13.2.0, Linuxamd64, montaje sólo lectura.
- Imagen compilación local `sha256:4aabb94283fcd504a91649e24394d0242d619b22ebeb549e1b8ab97a74676ed6`.
- Mednafen1.32.1, caché `full`, regiónNA, velocidad1, frameskip0, sin backend
  audio de juego. Imagen `sha256:e2ba1cf888d1eadfb52ab715234fb3cee51fbf820cb5afba3810347fdecb3f53`.
- BIOS mpr-17933.bin SHA256 `96e106f740ab448cf89f0dd49dfbac7fe5391cb6bd6e14ad5e3061c13330266f`.
- Paquete TEST03 SHA256 `65daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a`.
  Siete recursos comprobados mediante vendor/frontend_test03/SOURCE.json.
- Los originales están en Downloads; restaurador tools/restore_packages.py.

## Compilar

En Linux desde production/full_game, con python3,make,xorrisofs:

```sh
export YAUL_INSTALL_ROOT=/sdk
export WF_FRONTEND_PACKAGE=/ruta/WF-SATURN-MODE-ROSTER-HARDWARE-PASS-HANDOFF-R01
export WF_DISC_OUTPUT=/ruta/nueva/disco
bash tools/build_saturn.sh
```

El script verifica originales y recursos, deriva sólo el renderer TEST03,
recompila forzosamente (el SDK no registra cambios de flags) y genera el disco.
No sobrescribe un BIN/CUE congelado. El ISO incluye timestamps de herramienta;
no se exige que dos ISO recompilados tengan el mismo hash. Sí se conservan los
hashes del ejecutable y de cada disco realmente probado.

```sh
WF_ROM=/ruta/wwfwfest.zip make test sanitize
python3 tools/run_platform_checks.py --reference-tools /ruta/wp1980/tooling/mednafen \
 --cue /ruta/disco/wrestlefest-full-game.cue --elf build/saturn/wrestlefest-full-game.elf \
 --out /ruta/nueva/prueba --cart none --sdk /sdk --bios /bios/mpr-17933.bin
```

Repetir con `--cart extram1 --royal`, `--cart extram4` y
`--cart none --confirm-during-scroll`. El runner usa Xvfb aislado, teclado→mando
normal y /proc O_RDONLY. No escribe RAM, no usa savestate ni modifica Mednafen.
Los contadores tienen una publicación par/impar: las lecturas que atraviesan
una escritura se omiten y se vuelve a observar; toda violación en una muestra coherente activa
falla la prueba. No cuentan repeticiones de terminales ni tiempo del host.

## Evidencia final

Todos comparten programa SHA256
`dde3287d49170f1a1b655150d5dc0c27a6f6ec72047843415ea030b3fa0be63e`.
BIN de disco MODE1/2352 SHA256
`2001eb3a75e6893587fae57078de8aee2e9448169d03eb739402e490e89e431c`.
430 sectores verificados con EDC/ECC y A.BIN idéntico al ELF convertido.

| Prueba | ID SDK | Bytes SDK | 4MiB validado | Publicaciones activas distintas |
|---|---:|---:|---:|---:|
| None / TAG | 0 | 0 | no | 247 |
| 1MiB / ROYAL | 90 | 1048576 | no | 264 |
| 4MiB / TAG | 92 | 4194304 | sí | 246 |
| A mantenido durante scroll / None | 0 | 0 | no | 200 |

Las cuatro terminaron con error0 y confirmación esperada. El sondeo adicional
es de16 puntos en extremos de ocho bancos de512KiB, con restauración después
del sondeo; no es un test exhaustivo de cada byte de RAM. El SDK realiza antes
su propia detección, que escribe posiciones de sondeo durante el arranque.

Deuda máxima observada2: el reloj la conserva. Esto no demuestra todavía la
cadencia perfecta del juego ni sus ventanas de rendimiento; falta instrumentar
preparación/subida gráfica y todas las escenas y combates. No hay prueba física
del disco03. Los MOV nativos se conservan en work/platform03-tests/final-*/.
Los MP4 entregados se comparan frame a frame por PTS con esos MOV.

El renderer ahorra122880B reutilizando el buffer MODE como fondo de ROSTER.
El fin de estáticos del mapa está alrededor de0x0607E000. No equivale a un
presupuesto completo de combate ni activa todavía cachés ampliadas.

## Continuación exacta

1. Propietario compartido CD/FRT y transiciones VDP1/VDP2. Adaptar Intro R04
   deteniendo su controlador PTMR antes de transferir el control a Yaul.
2. Vincular el propietario ROM de la selección y equipos, no sus slots visuales;
   integrar Entrada/Presentadora con el adaptador ya probado y RNG/eventos reales.
3. Sustituir fixtures y bucles demo de TAG/Royal, integrar progresión completa.
4. Arena y cachés dinámicas estándar/4MiB, conservando idéntico gameplay.
5. Medir todos los recorridos/ventanas, corregir picos y probar disco completo.

Direcciones contradictorias>10: error explícito antes de la tabla; semántica
ROM OPEN. No se inventa una normalización. Los restantes pendientes de SPEC.md
siguen abiertos; el checkpoint03 no cierra las fasesB/C/D/E/F/G completas.
