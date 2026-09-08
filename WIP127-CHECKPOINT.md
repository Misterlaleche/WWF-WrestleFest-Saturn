# WIP127 CHECKPOINT — raw8E bit5 SET confined to upper actor slots

## Scope
ROM-EXACT producer/reachability closure for the third direct actor `+$32.bit5` SET family (`$1AE70`, state5/raw `$8E`). No live release/timing change and no BIN/CUE.

## Unique raw8E producer
Executable scan finds the direct raw/state5 `$8E` producer at `$202A6`.

The producer routine:
- starts A1 at actor address `$1C09E0`;
- scans exactly four actor records, stepping by `$010C`;
- selects a free record;
- clears its `+$44` and `+$32`;
- writes main state5;
- writes raw/control `+$60=$008E`.

Using the exact actor-slot table already closed in victim65 research:
- slot0 `$1C05B0`
- slot1 `$1C06BC`
- slot2 `$1C07C8`
- slot3 `$1C08D4`
- slot4 `$1C09E0`
- slot5 `$1C0AEC`
- slot6 `$1C0BF8`
- slot7 `$1C0D04`
- slot8 `$1C0E10`

the raw8E allocator targets only slots **4..7**.

## raw8E callback
State5 table maps raw `$8E` to descriptor `$1AE28` and phase dispatcher `$1AE34`.
Its first phase `$1AE50` contains the only raw8E-local direct `BSET #5,+$32(A0)` at `$1AE70`, alongside explicit position initialization including `Z=$014E`.
A later phase `$1AF4C` explicitly writes `Z=$0140`.

## Consequence for fighter Ground provenance
The third direct bit5 SET family is not produced in the four primary wrestler actor records slots0..3 by its unique raw8E producer; it belongs to the separately allocated upper-slot actor class 4..7.

Together with WIP126, all three direct `+$32.bit5` SET origins are now bounded:
- `$FABA/$FAE4`: controller state5 6A/6B chains that clear bit5 before state0 and preserve `+$01=1` for the first state0 pass;
- `$1AE70`: raw8E actor allocated only in slots4..7 and explicitly Z-positioned.

Therefore direct bit5 SET provenance is no longer an open blocker for the normal primary-wrestler Ground-entry class. This does not claim that arbitrary corrupted/copied actor32 words can never contain bit5; it closes the demonstrated executable SET origins.

## ROM anchors
`$20282`, `$2028A`, `$20292`, `$20298`, `$2029C`, `$202A0`, `$202A6`, `$202AE`, raw8E table entry `$1284C`, descriptor `$1AE28`, callbacks `$1AE50/$1AECA/$1AF4C/$1AF7E`, SET `$1AE70`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
