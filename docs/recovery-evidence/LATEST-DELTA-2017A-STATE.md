# LATEST DELTA — `$2017A/$206FE` ROYAL POST-ACK

Este documento conserva el delta completado por el chat saliente después de `RUNTIME17-CONTINUATION-2-LIVE-WORKTREE-WIP`.

## Archivos que se habían creado/modificado
Nuevos:
- `include/wf_r17_postack_2017a.h`
- `src/wf_r17_postack_2017a.c`
- `tests/test_runtime17_2017a.c`
- `tests/test_runtime17_royal_postack_e2e.c`
- `tools/audit_runtime17_2017a_rom.py`

Modificados:
- `include/wf_runtime.h`: añadir `global_0092`, `global_015e`, `global_0164`.
- `include/wf_gameplay_bindings.h`: incluir/registrar API del owner post-ACK.
- `src/wf_gameplay_bindings.c`: bindings pasan a 36, `$2017A` en slot 35, `wf_phase_postack_2017a`.
- `src/wf_scheduler.c`: Royal post-ACK despacha `$2017A`; después aplica gates caller `$1C16C4/$1C16C5` para rutas `$1B4E/$1AFC` fail-closed; normal `$1172 JMP $F9A`.
- `Makefile` y `Makefile.saturn-link`: incluir `src/wf_r17_postack_2017a.c`.

## Contrato ROM exacto `$2017A`
Entrada:
- Si `$1C007C == 0`: retorno, D0-D7 unchanged.
- Si `mode $1C0161.bit0 == 0`: retorno, D0-D7 unchanged.

Si `$1C16C5.bit0` ya está set:
- escanea `$1C0156[3..0]` descendente;
- D0 = index*2;
- D3 = índice actual en early return;
- si los cuatro son cero: D0 final 0, D3.low=FFFF, set bit1 en `$1C16C5`.

Si bit0 no estaba set:
- escanea primero cuatro WORDs `$1C1586` descendente; D1/D3 son caller-visible.
- escanea 9 actores desde `$1C05B0`, stride `$10C`:
  - D2=8 con DBF -> low final FFFF.
  - D3 cuenta activos con `flags32.bit4=0` y `role.bit7=0`.
  - D4 cuenta activos con `flags32.bit4=0` y `role.bit7=1`.
  - D5 = puntero ROM del último candidato D3; si no hay candidato, preservar D5 de entrada.
  - D6 = 2*actor index para activos con `flags32.bit4=1` y `role.bit7=0`, escribiendo `0x3001` en `$1C0156 + 2*index`.

Alias writes exactos por el raw table:
- index4 -> `$1C015E` => `global_015e`.
- index5 -> `$1C0160` => `mode_0160=0x30`, `mode_0161=0x01`.
- index6 -> `$1C0162` => `progression_0162=0x3001`.
- index7 -> `$1C0164` => `global_0164`.
- index8 -> `$1C0166` => `global_0166=0x3001`.

Tras scan:
- D3=0: set bit0 `$16C5`, clear `$169E`, return.
- D3>1: return.
- D3=1 y D4!=0: return.
- D3=1 y D4=0:
  - set bit2 `$16C5`;
  - call `$206FE`;
  - clear `$169E`;
  - referee `$1C1214 = 0x8009`;
  - store D5 a `$1C0092`;
  - selected actor: `$20=1`, `$AE=0x000C`;
  - scan slots actor 4..7 para primer inactive; activar y escribir `$44=0`, `$32=0`, `$20=5`, `$60=0x008E`;
  - D0 es DBF counter al retorno.
- D7 preservado por `$2017A`.

## `$206FE`
- MOVEM guarda/resta D0/A0.
- D1-D7 untouched.
- limpia cuatro bloques de 16 bytes en direcciones equivalentes a `$C0744`, `$C0844`, `$C0944`, `$C0A44`.
- en owner nativo debe pasar por callback existente `referee_clear_206fe`.
- preservar shadow D regs alrededor del callback para reflejar el contrato ROM.

## Caller gates después de `$2017A`
En post-ACK Royal:
- `$1C16C4` es high byte de `global_16c4`; si nonzero -> ruta de escena `$1B4E`, mantener fail-closed mientras no esté owner completo.
- `$1C16C5` es low byte de `global_16c4`; si ==3 -> ruta `$1AFC`, fail-closed.
- otherwise -> normal `$1172 JMP $F9A`.

## Tests/auditoría observados
- `test_runtime17_2017a`: PASS.
- `test_runtime17_royal_postack_e2e`: PASS; cubría ACK → `$10F2` → `$2017A` → `$1172/$F9A` → siguiente `$FA0/$21B4`.
- `audit_runtime17_2017a_rom.py`: SHA1 canónica + firmas `$2017A`, loops, branches, activation, `$206FE`: PASS.
- regresión global observada: 43/43 PASS.

## Reproducción requerida
Este delta debe reaplicarse al baseline y volverse a probar antes de continuar porque el snapshot físico posterior a la integración no sobrevivió a la compactación de la sesión. La especificación de arriba debe contrastarse con `wwfwfest_main68k.bin`; si hay contradicción, la ROM gana.
