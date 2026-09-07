# WIP62 — actor32 false-alias elimination for remaining CPU SET sites

Status: source/research safety checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT / control-domain closure
- `$1C4CA`, `$1C52E`, and `$1CED0` remain exact `BSET #2,+$32(A0)` sites coupled to a pending main-state-2 write.
- Test132 independently pins the actual raw CPU/AI state-2 range handler entry at `$1D7B6` and its range logic at `$1D902`.
- Therefore the three `$1Cxxx` SET sites are in the CPU dispatcher/control core and are **not** the raw-state-2 range handler itself.
- They must not be wired to `arcade_ai_state2_range_transition()` merely because both involve a state-2 transition.

## Runtime consequence
- No new SET was wired in `game.c`.
- `$1F354` remains the only currently event-wired actor32 SET, via the separately pinned AI state-0 approach event.
- raw `$10` remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No live `$FF00` release, no 32-tick fallback removal, and no `tick==12` timing change.

## Code/tests
- Renamed the generic candidate classification to `ARCADE_PRONE_RAW10_ACTOR32_CONTEXT_CPU_DISPATCH_CORE` to prevent conflation with the actual state-2 handler.
- Added Test346 false-alias guard.
- Directed Tests329/340/342/343/344/345/346 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Fidelity
- Exact mutation addresses and state-2 coupling: ROM-EXACT from Tests305/307.
- Raw-state-2 handler entry/range logic: ROM-EXACT from Test132.
- Exact event identities of `$1C4CA/$1C52E/$1CED0`: still OPEN / REGISTERED BUT NOT IMPLEMENTED.

## Next
With the unsafe `state2_range` alias excluded, recover the exact branch predicates/caller identities for `$1C4CA/$1C52E/$1CED0`. If those cannot be proven from retained evidence, continue Ground Actions closure on the next independently recoverable inherited input rather than guessing.
