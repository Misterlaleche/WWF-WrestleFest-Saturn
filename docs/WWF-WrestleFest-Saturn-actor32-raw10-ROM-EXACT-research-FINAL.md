# WWF WrestleFest Saturn — actor `+$32.bit2` / raw `$10` — corrected ROM-EXACT research FINAL

Status: **ROM-EXACT CLOSED for the direct bit lifecycle and safe raw `$10` provenance policy.**

Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

This document supersedes the previous actor32/raw10 package and Test409 wording wherever they described the nine explicit `BSET/BCLR` sites as the complete mutation census.

## Critical correction

`actor +$32.bit2` is bit 2 of the byte at offset `+$32`. On big-endian MC68000, `MOVE.W #$0004,+$32(A0)` stores byte `+$32=$00`, byte `+$33=$04`; therefore it CLEARS `+$32.bit2`, it does not SET it.

## Explicit bit-2 operations

SET:
`$0F3BC A0`, `$1C4CA A0`, `$1C52E A0`, `$1CED0 A0`, `$1D85A A0`, `$1F354 A0`, `$214A4 A2`.

CLEAR:
`$114CC A0`, `$11E04 A0`.

Direct tests:
`$1D938 A0`, `$2414C A1`.

## Whole-field operations that deterministically CLEAR `+$32.bit2`

A0:
`$0588A CLR.W`, `$074B0 MOVE.W #$0007`, `$09446 MOVE.W #$8003`, `$0FAD2 ANDI.W #$00C3`, `$0FB2A ANDI.W #$00C3`, `$0FBFA ANDI.W #$00C3`, `$10000 MOVE.W #$0004`, `$10034 MOVE.W #$0004`, `$1052A CLR.W`, `$108BE MOVE.W #$0001`, `$10930 MOVE.W #$0003`, `$10A48 MOVE.W #$0001`, `$125FC ANDI.W #$0087`, `$1A39C MOVE.W #$1000`, `$1C2E8 ANDI.W #$FBA7`.

A1:
`$06008 CLR.W`, `$0BCB0 MOVE.W #$8005`, `$104B6 CLR.W`, `$2029C CLR.W`.

`$0985C ORI.W #$0003,$32(A0)` writes the word but preserves bit2.

The main CPU/AI state dispatch table at `$1C1F4` maps state0 to `$1C2CC`; that handler executes `$1C2E8 ANDI.W #$FBA7,$32(A0)`, so CPU/AI state0 has an exact actor32 CLEAR boundary.

`$1A39C` belongs to raw `$7A` (`desc $1A348`, callback `$1A364`) and clears bit2 while replacing the word with `$1000`.

No demonstrated direct `MOVE.B`, register-source MOVE, arithmetic RMW, memory shift, MOVEM writer to `+$32`, or long write starting at `+$30` adds another direct bit2 mutation in the audited executable forms. Bulk actor-memory clears remain a separate reset mechanism.

## raw `$10`

Selector `$E382..$E406` requires linked A1, actor `+$34.bit5=0`, actor `+$21=0/1`, actor `+$AF=0`, target `+$26.long=0`, target `+$21=4`, target `+$65=1`, X delta `<$10`, Y delta `<$50`, and equal `+$2E` bytes; it selects `$10` at `$E3FC`.

Raw `$10` identity: `desc $141D8`, callback `$141EC`; next raw `$11` descriptor begins at `$14658`.

Neither the selector nor callback body contains a demonstrated direct actor32 mutator. The known helper calls on these paths do not provide a demonstrated actor32 mutation. Raw `$10` therefore consumes inherited provenance rather than establishing a fresh actor32 value.

## Safe provenance policy

- state0 after its exact state0 handler -> actor32 known CLEAR;
- state6 first-entry `$11E04` -> actor32 known CLEAR;
- controller-generated state1 from `$F228/$F2A2` -> actor32 known CLEAR;
- any other state1 -> inherit the last exact actor32 event;
- unknown state1 history -> actor32 UNKNOWN and raw `$10` remains blocked.

`state_set()` must never mutate actor32 generically.

## SET-site reachability to raw `$10`

`$0F3BC`, `$1C4CA`, `$1C52E`, `$1CED0`, `$1F354`: CONDITIONALLY REACHABLE at the provenance layer; later exact state0/state6/whole-field clears can cut the history.

`$1D85A` raw `$75`: NOT PROVABLE. Raw `$75` can lead to state6 (clear), state0 (clear), or state2 (preserve); a complete preserving state2→state1→raw10 choreography is not statically closed.

`$214A4` linked A2 SET: NOT PROVABLE for later raw10 entry.

NOT PROVABLE does not mean impossible and must never be collapsed to CLEAR.

## victim `+$65`

The separate FINAL `victim +$64/+65` research remains authoritative. Raw `$10` requires target state4 subtype `$01`; no actor32 value is inferred from that subtype.

## Closure

ROM-EXACT CLOSED: byte-level meaning, explicit bit operations, whole-field direct clears, CPU/AI state0 clear, state6 clear, raw7A clear, raw10 selector/callback identity, and exact safe blocker policy.

NOT PROVABLE FROM CURRENT STATIC EVIDENCE: complete live choreography from raw75 or linked A2 SET to raw10, and exhaustive dynamic reachability of every non-controller state1 producer. Those map to `UNKNOWN -> keep blocker`; no heuristic is authorized.

No BIN/CUE and no ROM bytes are persisted.