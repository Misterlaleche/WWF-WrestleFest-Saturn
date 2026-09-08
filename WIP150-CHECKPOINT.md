# WIP150 CHECKPOINT — corrected actor32/raw10 byte-level lifecycle

## Scope
Correct and close the ROM-EXACT actor `+$32.bit2` audit after discovering that Test409 counted only explicit `BSET/BCLR` operations and did not account for whole-word writes/masks that also alter byte `+$32.bit2`. Research/test only; no live runtime wiring and no BIN/CUE.

## Critical correction
MC68000 memory bit operations target byte `+$32`. Because the CPU is big-endian, `MOVE.W #$0004,+$32(A0)` stores `+$32=$00`, `+$33=$04`; it therefore CLEARs `+$32.bit2`, not SETs it.

The previous nine explicit mutations remain valid as the bit-operation subset, but are not the complete direct mutation set.

## Closed
- Explicit bit-2 SET/CLEAR/test census retained and corrected to include A1 test `$2414C`.
- Whole-field deterministic bit2 CLEAR events audited:
  - A0: `$588A`, `$74B0`, `$9446`, `$FAD2`, `$FB2A`, `$FBFA`, `$10000`, `$10034`, `$1052A`, `$108BE`, `$10930`, `$10A48`, `$125FC`, `$1A39C`, `$1C2E8`.
  - A1: `$6008`, `$BCB0`, `$104B6`, `$2029C`.
- `$985C ORI.W #$0003,+$32(A0)` proven to preserve bit2.
- CPU/AI main-state0 dispatch index0 -> `$1C2CC`; `$1C2E8 ANDI.W #$FBA7,+$32(A0)` is an exact additional state0 CLEAR boundary.
- raw `$7A`: desc `$1A348`, callback `$1A364`; `$1A39C MOVE.W #$1000,+$32(A0)` clears bit2.
- raw `$10`: selector `$E382..$E406`, desc `$141D8`, callback `$141EC`; no fresh demonstrated actor32 producer in selector/callback body.
- Safe raw10 policy:
  - exact state0 entry -> CLEAR;
  - controller-generated state1 -> CLEAR;
  - state6 first-entry -> CLEAR;
  - other state1 -> exact event history if known;
  - unknown state1 -> UNKNOWN and keep blocker.
- `state_set()` remains forbidden from mutating actor32 generically.

## Reachability classification
- `$0F3BC`, `$1C4CA`, `$1C52E`, `$1CED0`, `$1F354`: CONDITIONALLY REACHABLE at provenance level; later exact state0/state6/whole-field clears can cut history.
- `$1D85A` raw `$75`: NOT PROVABLE to raw10 with surviving SET; state0/state6 outcomes clear, state2 outcome preserves.
- `$214A4` linked A2 SET: NOT PROVABLE to later raw10 entry.

Unknown paths remain blockers; no heuristic introduced.

## Test
`tools/test410_actor32_raw10_rom.py` PASS against main program SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`.

## Research artifact
Local cumulative research package produced:
`WWF-WrestleFest-Saturn-actor32-raw10-ROM-EXACT-FINAL-package.zip`
SHA256 `4adf04b6a79803a95fb1fdf1f3485a9d97c5e863ee5e35ed43563685bc4814d5`.

The canonical WIP149 source ZIP was not available through File Library in this session, so no WIP150 source archive was fabricated from an older snapshot. Runtime/source integration is intentionally not claimed.

## Safety
No ROM bytes persisted. No BIN/CUE. No Ground `+$42/+$0E/contact37` subsystem changes.
