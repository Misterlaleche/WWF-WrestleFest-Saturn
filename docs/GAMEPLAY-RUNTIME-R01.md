# GAMEPLAY-RUNTIME-R01

Status: **CANDIDATE / fail-closed**.

Branch: `gameplay-runtime-r01`.

Current checkpoint: `BLOCK-GAMEPLAY-MASTER-01-RUNTIME-14`, cumulative from frozen RUNTIME-11 through RUNTIME-12/13/14.

Authority order:

1. `wwfwfest.zip` original arcade ROM.
2. Audited FINAL ROM-EXACT packages.
3. CLEAN-R01 workpacks.
4. Saturn / Yaul documentation.
5. Current implementation.
6. Historical WIPs.

WIP160/WIP175/WIP176 are not gameplay bases.

## Current build gates

- HOST TESTS: PASS with `-Wall -Wextra -Werror`.
- SH-2 / Yaul compile: PASS.
- FULL SATURN ELF LINK: PASS.
- UNRESOLVED SYMBOLS: 0.
- BIN/CUE: not generated.
- Hardware validation: not yet performed for this gameplay runtime.

These gates do not mean gameplay is complete. The Saturn entrypoint still refuses to synthesize an undemonstrated direct-match bootstrap.

## WP28 status

Real bindings currently exist for:

- ROM `$00514E` -> input decode/latch -> `wf_phase_input_real`
- ROM `$008B3A` -> payment/regain/refill owner -> `wf_phase_refill_8b3a`
- ROM `$02088E` -> actor spatial/classification -> `wf_phase_spatial_2088e`
- ROM `$01C150` -> CPU controller -> `wf_phase_cpu_real`
- ROM `$024E58` -> pending damage/stamina tier -> `wf_phase_damage`

RUNTIME-14 closes `$02088E` directly from canonical ROM bytes, including the live active-bit7 scan, exact 18-row `$0208EC` table, unsigned WORD comparison behavior, start-row gates from `+$33.bit2` / `+$32.bit0`, and WORD write to actor `+$30`.

A normal branch-A real-binding integration now advances through `$00514E -> $008B3A -> $02088E` and stops fail-closed at mandatory unbound phase `$020958`. `$020958` is not approximated from partial TAG/Royal target subroutines.

## WP01 / State5

The 143-selector map is present and all 43 unique callback families have executable runtime bodies for demonstrated behavior. Unknown downstream helpers or inherited CPU/register context remain explicit BLOCKED exits rather than approximations.

## CPU

CPU-FINAL is compiled into runtime code. Closed paths for states 5/6/7/8/9/A are integrated. RNG-dependent paths require valid externally supplied arcade RNG/register context. There is no `rand()`, synthetic seed, or fixed seed.

## Key OPEN/BLOCKED areas

- complete WP28 real-owner binding beginning with `$020958`;
- exact direct-match actor/global bootstrap;
- remaining collision choreography and ground/release dependencies;
- full pin/referee choreography;
- full match rules;
- full ring-out;
- TAG gameplay;
- Royal gameplay;
- final VDP2 ring/world integration;
- real hardware gameplay validation.

Do not call this runtime FINAL or gameplay complete while these remain open.
