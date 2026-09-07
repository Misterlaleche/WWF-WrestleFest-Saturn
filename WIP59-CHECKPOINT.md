# WIP59 — raw $10 actor32 provenance domain classification

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## Evidence-preserving closure
- `$1C4CA`, `$1C52E`, and `$1CED0` are no longer left as completely context-unknown raw-$10 candidates.
- Existing ROM-pinned CPU actor-loop/control evidence (Tests 85/91/307) places these `$1Cxxx` state-2 `BSET #2,+$32(A0)` producers inside the CPU/AI actor-control domain, outside the raw-action callback table.
- `$1F354` remains the more specifically identified AI state-0 distance/approach transition.

## Consequence
- None of the four exact state-2 `+$32 bit2` SET sites is a callback-local raw `$10` producer.
- Raw `$10` therefore inherits actor32 bit2 through controller/state history. A single universal producer must not be invented.
- Exact sub-branch/caller identity for `$1C4CA/$1C52E/$1CED0` remains open.

## Runtime safety
- raw `$10` remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No additional state-2 SET is wired into `game.c`.
- No `release=true` activation.
- No 32-tick fallback removal.
- No `tick==12` replacement.

## Code/tests
- Added `ARCADE_PRONE_RAW10_ACTOR32_CONTEXT_CPU_ACTOR_CONTROL`.
- Updated candidate metadata for `$1C4CA/$1C52E/$1CED0`.
- Updated Test342 to reflect the narrower controller-domain classification.
- Added Test343 to enforce that all four candidates are controller-history sources, not falsely promoted raw-$10-local sources.
- Directed Tests329..343 PASS; `tools/check_host.py` Python syntax PASS.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP59-source.zip`
- SHA256: `4c3b951a1f1c8dec375657cb1ed18e3e9bd8fc98cd6c383900eac7cd3c465715`

## Next
Trace the exact CPU/AI state-history path into the selection-ID 4/9 raw `$10` entry and determine whether actor32 bit2 is guaranteed SET, conditionally SET, or may remain clear. Only then can the raw `$10` actor32 blocker be removed.
