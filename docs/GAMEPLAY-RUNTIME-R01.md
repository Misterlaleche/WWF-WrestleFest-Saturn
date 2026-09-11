# GAMEPLAY-RUNTIME-R01

Status: **CANDIDATE / fail-closed**.

Branch: `gameplay-runtime-r01`.

Current checkpoint represented by this document: `BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13`, cumulative from the frozen RUNTIME-11 checkpoint.

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
- SH-2 CROSS COMPILE: PASS, 37 translation units.
- FULL SATURN ELF LINK: PASS against Yaul specs.
- UNRESOLVED SYMBOLS: 0.
- BIN/CUE: not generated.
- Hardware validation: not yet performed for this gameplay runtime.

These build gates do not mean the gameplay runtime is complete. The Saturn entrypoint deliberately refuses to synthesize an undemonstrated direct-match bootstrap.

## WP28 status

Real bindings currently exist for:

- ROM `$00514E` -> input decode/latch -> `wf_phase_input_real`
- ROM `$008B3A` -> payment/regain/refill owner -> `wf_phase_refill_8b3a`
- ROM `$01C150` -> CPU controller -> `wf_phase_cpu_real`
- ROM `$024E58` -> pending damage/stamina tier -> `wf_phase_damage`

A normal branch-A frame currently stops fail-closed at mandatory unbound ROM phase `$02088E`. Missing required phases are not treated as success.

## WP01 / State5

The 143-selector map is present and all 43 unique callback families have executable runtime bodies for demonstrated behavior. Unknown downstream helpers or inherited CPU/register context remain explicit BLOCKED exits rather than approximations.

## CPU

CPU-FINAL is compiled into runtime code. Closed paths for states 5/6/7/8/9/A are integrated. RNG-dependent paths require valid externally supplied arcade RNG/register context. There is no `rand()`, synthetic seed, or fixed seed.

## Key OPEN/BLOCKED areas

- complete WP28 real-owner binding;
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
