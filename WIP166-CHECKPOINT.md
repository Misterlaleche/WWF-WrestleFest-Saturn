# WIP166 CHECKPOINT — actor32.bit0 controller-state1 separation and chained preservation

## Scope
ROM-EXACT refinement of actor `+$32.bit0` provenance for held Ground readiness. Research checkpoint only: no live release, no fallback removal, no `tick==12` change, no BIN/CUE.

## Controller state1 is not the `$1179E` SET path
Controller entries `$F228` and `$F2A2` write:

```text
main state = 1
+$AE = 4
```

State1 dispatcher `$11652` indexes its pointer table by `+$AE & $3F`.
Exact table entries include:

```text
index 1 -> $11796
index 4 -> $11ABA
```

The direct bit0 SET at `$1179E` is inside the **index-1** `$11796` first-entry callback:

```text
BTST #7,+$1C(A0)
...
BSET #0,+$32(A0)  ; $1179E
```

The controller-generated `+$AE=4` path instead executes `$11ABA`. Its first entry sets `+$01=1` but contains no direct `BSET/BCLR #0,+$32(A0)`.

Therefore the WIP108/WIP129 controller-state1 provenance class must not inherit the `$1179E` SET merely because both are main state1.

## State0/state7 preservation
Normal state0 init `$114B2..$115F2` contains no direct bit0 mutation. Standard state7 recovery `$11E42..` likewise contains no direct bit0 mutation.

Thus state7 -> state0 does not itself establish bit0, but it preserves whatever exact bit0 provenance reached state7.

## Held-Ground preservation
The six direct bit0 mutation addresses remain exactly:

```text
SET   $1179E, $188D2
CLEAR $12C68, $18808, $18AE4, $19728
```

None lies inside the audited held-Ground callback families for raw `$0B/$0E/$10/$13/$14/$23/$35/$46/$47`.

Therefore a context that enters a held Ground chain with exact bit0=0 and executes only these held callbacks plus standard state7->state0 recovery preserves exact bit0=0 into the next chained Ground selection.

This is a provenance-class statement, not a claim that every state0 actor has bit0=0. In particular state1 subphase `+$AE=1` can set it.

## Ground consequence
For WIP132 chained Ground, actor32.bit0 can be removed as a classifier blocker **only when the chain begins from an exact bit0-clear event/provenance and no intervening bit0 event occurs**. Native state similarity is insufficient.

## Validation
Local Test430 pins controller `$F228/$F2A2`, state1 table indices 1/4, `$1179E`, `$11ABA`, and verifies the absence of bit0 direct events from state0/state7 and all audited held callback windows.

## Safety
- No physical held release enabled.
- No generic state0/state1 bit0 default invented.
- No 32-tick fallback removed.
- `tick==12` unchanged.
- No ROM bytes persisted.
- No BIN/CUE generated.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
