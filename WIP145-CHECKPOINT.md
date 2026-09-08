# WIP145 CHECKPOINT — exact callback-before-common-updater hold operation model

## Scope
ROM-EXACT closure of the ordering and counter semantics needed for a future typed held-action clock API. This follows WIP143/WIP144. Research/checkpoint only: no live release enabled, no held fallback removed, no `tick==12` change, no BIN/CUE.

## Exact actor-pass ordering
For the active fighter descriptor path, the action/state callback executes before the common frame updater `$1C0C8..$1C126`.

Therefore a held callback does not directly 'advance an animation frame'. It mutates actor fields/state, and the common updater then consumes the resulting `+$22/+24/+25` state in the same actor pass if the actor remains on that descriptor path.

## Common updater finite-counter rule
For a finite `+$22` value:

```text
SUBQ.W #1,+$22
BCC     return       ; no underflow -> remain same phase
underflow            ; zero became FFFF
ADDQ.W #1,+$24       ; advance phase index
...
```

Hence ROM duration `N` represents an inclusive `N+1` updater-pass residency, matching the already recovered Saturn finite-duration clock convention.

## Exact application of held callback outcomes

### HOLD_KEEP
Callback leaves `+$22=$FF00`.

Common updater sees `$FF00` and returns without decrement or phase change.

### HOLD_CLEAR_AND_ADVANCE
Callback executes:

```text
CLR.W +$22
```

Common updater then runs in the same pass:

```text
0000 -> SUBQ -> FFFF with underflow
-> ++ phase index
```

Thus the correct native equivalent is:

```text
set current remaining counter to 0
then execute one normal common-clock tick
```

It is not conceptually 'skip to next frame' before callback side effects.

### HOLD_SET_FINITE
Raw `$13` proves:

```text
MOVE.W #$0018,+$22
```

The common updater runs in the same pass and produces:

```text
0018 -> 0017
```

The actor remains on the same descriptor phase. Repeated common updater passes continue:

```text
0017 ... 0001 -> 0000 -> FFFF/underflow -> phase advance
```

The phase therefore advances on the 25th common-updater invocation counting the callback pass itself, i.e. after the contact pass plus 24 further actor passes, assuming no intervening callback state change.

The correct native equivalent is:

```text
set remaining = 0x0018
then execute one normal common-clock tick in the same pass
```

### HOLD_STATE_TRANSITION
Raw `$0E` phase2 proves a callback can leave the old held descriptor through explicit state choreography rather than by changing `+$22`.

In that case the native runtime must apply the state transition and must not tick/advance the obsolete held descriptor as if it were a release.

## Required native abstraction
A ROM-faithful callback result can be represented structurally as, for example:

```text
operation = KEEP | CLEAR | SET_FINITE | STATE_TRANSITION
finite_counter = valid only for SET_FINITE
```

and processed in this order:

```text
1. resolve callback predicates and side effects
2. apply callback counter/state operation
3. if the same descriptor remains active, run exactly one common clock tick
4. otherwise continue in the newly selected state/descriptor path
```

This ordering is the important invariant; enum names are port implementation terminology, not claimed arcade semantic labels.

## Compatibility with current Saturn clock
The existing finite-duration and `$FF00` freeze mechanics are structurally compatible with the ROM common updater. The unsafe piece is only the current boolean-style held release abstraction, because it cannot encode raw `$13` finite re-arming or raw `$0E` phase2 state transition.

No physical held action is authorized for activation solely by this result.

## ROM anchors
Common fighter descriptor updater `$1C0C8..$1C126`; raw `$13` finite re-arm `$14AA6`; raw `$0E` phase2 `$13FEC..$141D6`; WIP143 clear-and-advance callback sites.

## Safety
No runtime code changed. No physical release enabled. No 32-tick fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
