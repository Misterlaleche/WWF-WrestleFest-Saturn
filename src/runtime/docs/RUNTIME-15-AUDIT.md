# BLOCK-GAMEPLAY-MASTER-01 — RUNTIME-15 AUDIT

Status: GAMEPLAY-R01 CANDIDATE / cumulative / fail-closed.

Base: frozen RUNTIME-11 -> RUNTIME-12 -> RUNTIME-13 -> RUNTIME-14 -> RUNTIME-15. WIP160/WIP175/WIP176 are not gameplay bases.

Authority: wwfwfest.zip > audited FINAL ROM-EXACT packages > CLEAN-R01 > Saturn/Yaul > current implementation > historical WIPs.

## WP28 advance

The real-owner branch-A chain now reaches:

$00514E -> $008B3A -> $02088E -> $020958 -> $026936 -> $02983C -> $00F560 (next BLOCKED owner)

New in RUNTIME-15:
- ROM $020958 target/relation pass implemented from canonical ROM control/dataflow.
- TAG resolver $20CCC preserves distinct +$7A primary target and +$82 secondary opponent-pair target.
- TAG helper $20B44 preserves role/partner ownership checks.
- Royal primary candidate scan $20E00 preserves metric `(abs(adjusted_dx)>>2)+abs(dy)` and special state4/state5 +/-$48 X adjustment.
- Royal secondary candidate scan $20BDA preserves metric `(abs(dx)>>1)+abs(dy)` and ownership filter helper $20C88.
- global $1C168C candidate pointer and $1C1692 metric are retained numerically.
- WP21 camera aggregate/clamp owners $26936/$2983C are bound directly into WP28 through the already-audited camera implementation.
- invalid inherited actor pointers remain WF_BLOCKED; no fallback target selection is invented.

## Build gates

HOST TESTS: PASS with -Wall -Wextra -Werror.
SH-2 CROSS COMPILE / FULL ELF BUILD: PASS, 39 translation units.
FULL SATURN ELF LINK: PASS.
UNRESOLVED SYMBOLS: 0.
BIN/CUE: not generated.

## Remaining frame blocker

wf_runtime_frame() still does not complete a normal gameplay frame. The next mandatory unbound WP28 owner is ROM $00F560 actor-pair/global relation pass.

Direct-match bootstrap remains BLOCKED pending exact actor/global initialization authority.
