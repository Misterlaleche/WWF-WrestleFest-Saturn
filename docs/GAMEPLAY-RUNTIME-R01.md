# GAMEPLAY-RUNTIME-R01

Status: **CANDIDATE / fail-closed**.

Branch: `gameplay-runtime-r01`.

Current checkpoint: `BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`, cumulative from frozen RUNTIME-11 through RUNTIME-12/13/14/15/16.

Authority order:

1. `wwfwfest.zip` original arcade ROM.
2. Audited FINAL ROM-EXACT packages.
3. CLEAN-R01 workpacks.
4. Saturn / Yaul documentation.
5. Current implementation.
6. Historical WIPs.

WIP160/WIP175/WIP176 are not gameplay bases.

## Current build gates

- HOST TESTS: PASS with `-std=c11 -Wall -Wextra -Werror -pedantic`.
- SH-2 cross compile: PASS; full Saturn ELF link uses 41 objects.
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
- ROM `$020958` -> target/relation pass -> `wf_phase_target_relation_20958`
- ROM `$026936` -> gameplay camera aggregate -> `wf_phase_camera_26936`
- ROM `$02983C` -> gameplay camera clamp -> `wf_phase_camera_2983c`
- ROM `$00F560` -> pair/relation aggregate -> `wf_phase_pair_relation_f560`
- ROM `$00F18A` -> actor raw-state prepass -> `wf_phase_actor_prepass_f18a`
- ROM `$01C150` -> CPU controller -> `wf_phase_cpu_real`
- ROM `$024E58` -> pending damage/stamina tier -> `wf_phase_damage`

RUNTIME-16 integrates the ROM-derived `$00F560` pair/relation aggregate and `$00F18A` actor raw-state prepass. RNG-dependent `$0021B4`, active transition `$00F98C`, active WP24 producer `$00A0E0`, human-controller `$00DCDE`, follow-helper `$010E86`, and state-2 helper `$01129E` paths remain explicit fail-closed dependencies.

A normal branch-A real-binding integration now advances through `$00514E -> $008B3A -> $02088E -> $020958 -> $026936 -> $02983C -> $00F560 -> $00F18A -> $01C150` and stops fail-closed at mandatory unbound phase `$01F914` (fixed referee scheduler). No stub is used to move the blocker.

## WP01 / State5

The 143-selector map is present and all 43 unique callback families have executable runtime bodies for demonstrated behavior. Unknown downstream helpers or inherited CPU/register context remain explicit BLOCKED exits rather than approximations.

## CPU

CPU-FINAL is compiled into runtime code. Closed paths for states 5/6/7/8/9/A are integrated. RNG-dependent paths require valid externally supplied arcade RNG/register context. There is no `rand()`, synthetic seed, or fixed seed.

## Key OPEN/BLOCKED areas

- complete WP28 real-owner binding beginning with `$01F914`;
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
