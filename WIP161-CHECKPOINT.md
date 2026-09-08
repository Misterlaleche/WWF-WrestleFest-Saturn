# WIP161 CHECKPOINT — raw $23 +$44 subdispatcher and initial held-motion provenance

## Scope
Continue from canonical WIP160 by closing the internal raw `$23` `+$44` substate dispatcher far enough to disambiguate the two motion writer tuples. Research/test checkpoint only: no held physical release enabled, no 32-tick fallback removed, no `tick==12` change, no BIN/CUE.

## ROM-EXACT subdispatcher
Raw `$23` callback `$15D08` masks `+$44(A0)` to low nibble and dispatches through table `$15D20`:

```text
0 -> $15D44
1 -> $15DB8
2 -> $15DB8
3 -> $15DB8
4 -> $15DB8
5 -> $175FC
6 -> $176C4
7 -> $1770C
8 -> $15E42
```

The state-0 terminal path writes main state5 and increments `+$44`. Global `$1C0161.bit1` may override the result to `+$44=8`, so the early split is:

```text
0 -> 1   when global bit1 clear
0 -> 8   when global bit1 set
```

The global bit is explicitly mode-switched by BSET/BCLR at `$F9D6/$F9F4`; therefore neither route may be globally assumed.

## Initial held gate is no longer motion-ambiguous
Substates `+$44=1..4` all enter shared callback `$15DB8`. On callback first entry it establishes:

```text
$15DE2  +$5C = $0500
$15DE8  +$5E = $0048
$15DF4  +$01 = 2
```

and the held contact gate is:

```text
$15DFE  BTST #4,+$37(A0)
$15E10  CLR.W +$22(A0) on the contact path
```

Therefore the motion provenance for the **initial raw-$23 held gate `$15DFE`** is uniquely `$0500/$0048/2`, qualified by internal substate `+$44=1..4`.

## The second motion tuple belongs to a different subcallback
`+$44=8` dispatches to `$15E42`, whose first-entry motion setup is:

```text
$15E58  +$5C = $0600
$15E5E  +$5E = $0048
$15E64  D0 = 2 -> common helper establishes action motion byte
```

Its later probe/contact sequence is distinct:

```text
$15EA8  +$42 = +$20
$15EAE  BTST #4,+$37(A0)
$15EBE  CLR.W +$42(A0)
```

Thus WIP160's two raw-$23 writer tuples are not two unresolved alternatives for one held gate. They belong to different `+$44` subcallbacks and different contact sequences.

## Later +$44 graph
The demonstrated terminal transitions include:

```text
1->2 or 5
2->3 or 5
3->4 or 5
4->5
5->6
6->7
7->8
```

The 1..4 shared path uses `+$36==1` to force 5; otherwise it increments `+$44`.

## Runtime consequence
A ROM-faithful native model should qualify raw `$23` motion/contact behavior by internal `+$44` context (or an equivalent proven gate identity), rather than expose one raw-only motion profile.

For the initial held gate `$15DFE`, motion provenance is now structurally closed. This **does not** authorize physical release: actor32/Z/probe/contact/helper/action-phase validity still have to satisfy the context-qualified readiness contract.

## Test
New `tools/test426_raw23_substate_rom.py` passes against the verified main program and pins the dispatch table, both motion tuples, initial held clear path, later probe path, substate transitions and global bit1 mode toggle.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## Safety
- No physical held release enabled.
- No 32-tick fallback removed.
- No `tick==12` migration.
- No ROM bytes persisted.
- No BIN/CUE generated.

## Next
Build the context-qualified held-release readiness matrix using the now-closed raw23 initial motion context, then resolve remaining helper-carry and actor32 provenance one raw/phase at a time. Do not globally enable release.
