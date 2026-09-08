# WIP141 CHECKPOINT — full 0..15 control-column semantics

## Scope
ROM-EXACT correction/completion of WIP140's raw `$14` phase-3 predecessor matrix. WIP140 correctly identified the scheduler/table mechanism but implicitly described only nominal 0..10 columns. This checkpoint closes the actual `+$A8` domain and supersedes that limited column interpretation. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Exact `+$A8` domain
At `$64EA..$64FC`:

```text
D1 = WORD[(+$8A relation)]
NOT.W D1
ANDI.W #$00FF,D1
MOVE.W D1,+$A8(A0)
ANDI.B #$0F,+$A9(A0)
```

Because the 68000 is big-endian, `+$A8/+A9` is the word written by `MOVE.W`, and the byte mask targets its low byte. Therefore after this sequence:

```text
WORD +$A8 is exactly in 0..15
```

At `$5B70/$5B74/$5B78` the scheduler performs:

```text
index = old_phase * 11 + WORD(+$A8)
new_phase = BYTE[table_base + index]
```

There is no clamp of `+$A8` to 0..10 before the load. Consequently values 11..15 are intentional linear offsets into the first five bytes after the nominal 11-byte row. At row boundaries this can read bytes that are physically contiguous with the next nominal row/block. This is exact ROM addressing, not an out-of-range condition invented by the port.

## Complete predecessors of new phase 3
For the default table `$62BC` (old phases 0..7):

```text
A8  0: 3 -> 3
A8  1: 2 -> 3
A8  2: 4 -> 3
A8  3: 3 -> 3
A8  4: 3 -> 3
A8  5: 3 -> 3
A8  6: 3 -> 3
A8  7: 3 -> 3
A8  8: no phase -> 3
A8  9: 3 -> 3
A8 10: 3 -> 3
A8 11: 2 -> 3
A8 12: 1 -> 3
A8 13: 3 -> 3
A8 14: 2 -> 3
A8 15: 2 -> 3
```

For `$6314` mode table (old phases 0..9):

```text
A8  0: 3 -> 3
A8  1: 2 -> 3
A8  2: 4 -> 3
A8  3: 3 -> 3
A8  4: 3/8 -> 3
A8  5: 3 -> 3
A8  6: 3 -> 3
A8  7: 3 -> 3
A8  8: 0 -> 3
A8  9: 3 -> 3
A8 10: 3 -> 3
A8 11: 2 -> 3
A8 12: 1 -> 3
A8 13: 3 -> 3
A8 14: 2 -> 3
A8 15: 2/7 -> 3
```

The `$6382` alternate table has the same phase-3 predecessor set as `$6314` for A8 0..15.

## Secondary assignment remains exact
WIP140's secondary-path conclusion remains unchanged:

```text
$62AA = 07 02 06 01 05 00   (cannot directly assign 3)
$62B0 = 09 04 08 03 07 02   (candidate index3 can assign 3)
$62B6 = 08 04 08 03 07 02   (candidate index3 can assign 3)
```

## Ground consequence
Raw `$14` gate `+$25==3` still means exactly that the pre-controller phase entering the current pass was 3. The full ways to establish that prior phase are broader than a nominal 11-column reading because `+$A8=11..15` is ROM-valid.

No fixed elapsed-tick or animation-frame equivalence follows from this. Runtime reachability still depends on relation-derived `+$A8`, global table mode, occupancy arbitration, secondary assignment, and current-turn `+$37.bit4`.

## ROM anchors
`$64EA..$64FC`, `$5B66..$5B78`, `$62BC/$6314/$6382`, `$600E..$6096`, `$62AA/$62B0/$62B6`, raw14 gate `$14C0A..$14C2A`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
