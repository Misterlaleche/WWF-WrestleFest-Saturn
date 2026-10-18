# CURRENT STATE — GAMEPLAY MASTER RUNTIME17 CONTINUATION 4

## Clasificación
- RUNTIME-17 advanced staging.
- No RUNTIME-18.
- No HARDWARE PASS todavía.
- Objetivo pendiente: primera BIN/CUE de validación de Gameplay con imagen real en Saturn.

## Autoridad suprema
- `wwfwfest.zip` / main 68000 SHA1 esperado: `a726c46f350723ea73158bc78055d17d418b14e2`.
- El handoff anterior completo contiene `wwfwfest_main68k.bin` y todas las autoridades heredadas.

## Baseline físico que debe extraerse
Dentro de `payload/previous_handoff/GAMEPLAY-MASTER-RUNTIME17-CONTINUATION-3-FULL-HANDOFF.zip`:
- `payload/checkpoints/RUNTIME17-CONTINUATION-2-LIVE-WORKTREE-WIP.zip`
Ese es el baseline vivo. NO reconstruir desde R16 ni desde `$7A00`.

## Scheduler heredado
Branch A:
`... → $8CF8 → $9052 → $899C → $8ECC → $2836 → $18C4 → $298EC → frame-ready → VBlank/ACK → post-ACK → $F9A → $FA0/$21B4`

Branch B:
`$285DA → $26A42 → ... → $7548 → ... → $8CF8 → $BCFE → $C04A → $C710 → $2836 → $298EC → frame-ready → VBlank/ACK → post-ACK → $F9A → $FA0/$21B4`

Owners históricos cerrados: `$01F914`, `$F4C2`, `$24062`, `$24E58`, `$FDEE`, `$10120`, `$10E6A`, `$7A00`, más post-$7A00 ya heredado.

## Contratos importantes heredados
- VBlank normal guarda/restaura D0-D7/A0-A6: transparente a D regs.
- `$10F2` incrementa frame counter exactamente una vez tras ACK.
- `$F9A CLR.W $1C006E` no toca D0-D7.
- `$FA0 -> $21B4` obligatorio cada iteración.
- RNG `$21B4`: incoming D0 se sobrescribe antes de contribuir; D1 restaurado; D2-D7 unchanged; estado RNG actualizado; D1-D7 persistentes inter-frame.
- `$298EC` nonempty: dos `LSL.W`; D0 high16 preservado y low16=`4*(last_id-1)`; D1 table-derived; D4 high16 preservado/low16=FFFF; D2,D3,D5,D6,D7 preservados.
- `$2836`: MOVEM preserva D0-D5; D6/D7 caller-visible; deeper `$D216 CLR.L D7`.
- `$9052`: segundo audio indexa `$0DEE` con low byte completo `$1C0162`.
- `$899C`: 13 rows, `CLR.L D1` por row.
- `$26A42/$26B76`: dir 0=Y++,1=X++,2=Y--,3=X--.
- Royal free fallback `$10AF0/$10B26`: primer slot libre 0→5.
- pending damage decimal 14 = `0x0E`.

## WP35/WP36
- WP35 R05 SHA256: `6cd5009bcb85c0479388f8518ff2b65d96e031b2bca7025610d5149057fb6be2`.
- WP36 authority SHA256: `cd9731cefe74aa1c6ca13c7d7cc181d5cf9864677116746321cf29d944899a80`.
- No reabrir salvo contradicción ROM demostrada.

## Trabajo hecho después del handoff anterior
El chat saliente auditó e integró `$2017A/$206FE` conceptualmente y observó:
- `test_runtime17_2017a`: PASS.
- `test_runtime17_royal_postack_e2e`: PASS.
- auditoría ROM `$2017A/$206FE`: PASS.
- regresión acumulativa: 43/43 PASS.
- GCC strict / Clang strict: PASS.
- ASan+UBSan dirigido: PASS.
- full SH-2/Yaul link: unresolved=0.

IMPORTANTE: por compactación/reset del entorno, ese delta posterior no está garantizado como source físico dentro del baseline archivado. Debe reaplicarse/reproducirse desde `LATEST-DELTA-2017A-STATE.md`, verificándolo contra la ROM. No dar por hecho que el source local ya lo tiene.

## Saturn toolchain
El handoff anterior incluye:
- `sh2eb-elf-yaul(1).zip`
- `libyaul-develop.zip`
Yaul fue compilado/instalado durante la sesión saliente y un ELF completo enlazó con unresolved=0.
Último ELF observado tras `$2017A`: SHA256 `9a24a34b77f8187e489da632e82a2b8d3995601789d3d9fb42b9a1299cfb3444`.
No asumir que ese ELF está en este paquete; reproducir el enlace.

## WP33 / disco
Autoridad preextraída: `payload/authorities/WF-CLEAN-R01-33-SATURN-BOOT-CDFS-BUILD-DISC-LAYOUT-FINAL.zip`.
- Ejecutable HWRAM `0x06004000`.
- master stack `0x06004000`.
- slave stack `0x06001E00`.
- 1st read addr `0x06004000`.
- 1st read size `0`.
- media final: MODE1/2352 BIN/CUE.
- ISO solo intermedio.
- usar `iso_to_bin_exact.py` + `verify_disc_exact.py`.
- no CDDA/marker files inventados.

## Bloqueo real al parar
El baseline `src/wf_saturn_entry.c` es un link-validation harness y termina `WF_BLOCKED`. Antes de build hay que conectar:
1. boot/reset/RNG autoritativo WP29;
2. una entrada Gameplay real y demostrada (ruta ATTRACT preferida si cierra todo el handoff);
3. renderer Saturn real;
4. bucle de frame/VBlank/ACK;
5. empaquetado WP33.

## Ruta de arranque investigada por el chat saliente
La opción más limpia parecía ATTRACT state `0x01` (Rumble):
- 736 gameplay updates según autoridad de attract integrada del repositorio maestro consultado;
- 6 actores IDs `{0x03,0x06,0x07,0x08,0x09,0x02}`;
- setup especial de estados 01/02;
- evita inventar un Direct-Match.
No codificarla hasta confirmar todos los globals/actores/RNG de entrada contra WP17/WP29/WP28 y ROM.

## Qué debe mostrar la primera build
- El disco arranca como Saturn game, no navegador musical.
- Da imagen.
- Entra a gameplay real autorizado.
- Ring/escenario y luchadores visibles.
- Actores animados y actividad CPU.
- Frames continúan con VBlank/ACK sin cuelgue inmediato.
No se exige aún frontend/HUD/audio/records/entrances finales de juego completo.
