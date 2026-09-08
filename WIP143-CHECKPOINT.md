# WIP143 CHECKPOINT — held `FF00` release-counter semantics

## Scope
ROM-EXACT classification of how callback-held Ground Actions leave descriptor counter `$FF00`. This follows WIP142's proof that the common descriptor engine owns actor `+$22/+24/+25`. Research/checkpoint only: no live release enabled, no fallback removed, no `tick==12` change, no BIN/CUE.

## Common descriptor-engine baseline
The fighter descriptor updater `$1C0C8..$1C126` establishes:

```text
+$22 == $FF00 -> freeze current descriptor phase
finite +$22    -> SUBQ.W #1,+$22; advance phase only on underflow
phase advance  -> ++WORD +$24/+25 phase index
non-loop end   -> MOVE.W #$00FE,+$24
kind/type 2    -> loop to phase 0
```

This matches the port's inclusive finite-duration accounting and `$FF00` freeze at the phase level.

However, callback release behavior is **not one universal operation**.

## Class A — callback clears `+$22`: same-pass phase advance
For the following demonstrated successful held branches, the callback executes `CLR.W +$22(A0)` before returning to the common updater:

```text
raw $0B   $13AB2
raw $0E phase0  $13E20
raw $0E phase1  $13EFE
raw $10   $142FA or $1435E (branch-dependent successful routes)
raw $14   $14C38
raw $23   $15E10
raw $35   $17836
raw $46/$47  $1451E or $14582 (successful branch-dependent routes)
```

Because the common updater runs after the callback in the same actor pass, its `SUBQ.W #1,+$22` turns zero into `$FFFF` with borrow/carry-clear and immediately advances the descriptor phase. At the phase-state level this class is compatible with an immediate one-phase advance, provided the callback's own predicates and side effects have first been reproduced exactly.

## Class B — raw `$13`: convert hold to finite countdown, do NOT advance immediately
Raw `$13` is structurally different.

At the active contact path:

```text
$14A72  test current-turn +$37.bit4
...
$14A8A  linked target -> main state4
$14A90  linked target -> WORD $0007 at +$64/+65
$14A96  pending damage $000A
...
$14AA6  MOVE.W #$0018,+$22(A0)
```

The callback does **not** clear `+$22`.

The common updater then runs in the same pass and decrements `$0018 -> $0017`. The actor remains on the same descriptor phase for a finite countdown before advancing.

Therefore the current Saturn abstraction:

```text
release_hold == true -> increment frame immediately
```

would be ROM-wrong for raw `$13`.

A faithful implementation needs a release operation capable of replacing `$FF00` with a finite counter (`$0018`) while preserving the current phase.

## Consequence for runtime design
A single boolean `release_hold` is insufficient as the long-term ROM-faithful API.

At minimum, held callback outcomes need distinct semantics such as:

```text
HOLD_KEEP
HOLD_CLEAR_AND_ADVANCE   // callback CLR.W +$22; common updater advances same pass
HOLD_SET_FINITE          // e.g. raw13: +$22=$0018, same phase
HOLD_STATE_TRANSITION    // callback exits/changes state rather than ordinary phase advance
```

The names above describe implementation classes, not invented arcade semantic labels.

Raw `$0E` phase2 and any branch that performs a terminal/state transition must be audited separately before assigning it to one of these classes.

## Safety conclusion
No held action is authorized for global physical release from this checkpoint.

In particular:
- do not enable `release_hold=true` generically;
- do not remove the 32-tick compatibility fallback;
- do not migrate `tick==12` damage;
- do not treat raw `$13` as an immediate descriptor-frame release.

## ROM anchors
Common frame updater `$1C0C8..$1C126`; raw `$0B` `$13AA8..`; raw `$0E` `$13DF8..`, `$13EF2..`; raw `$10` `$142D8..`; raw `$13` `$14A72..$14AA6`; raw `$14` `$14C1A..`; raw `$23` `$15DFE..`; raw `$35` `$1780A..`; raw `$46/$47` `$144F0..$14582`.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

No ROM bytes are persisted by this checkpoint.
