# WIP146 CHECKPOINT — exact held callback release predicate matrix

## Scope
ROM-EXACT refinement of WIP143–WIP145: classify which held Ground callback outcomes are contact-only and which require helper carry predicates before mutating `+$22` or leaving the hold. Research/checkpoint only; no live release enabled, no fallback removed, no `tick==12` change, no BIN/CUE.

## Why this matters
The current native scaffold historically reduced many held callbacks to:

```text
contact37 -> release_hold=true
```

The ROM proves this is too permissive for several actions. `+$37.bit4` is necessary, but not always sufficient.

## Contact-only clear-and-advance cases
These callbacks execute `CLR.W +$22(A0)` directly on the contact path, before any later branch can veto that counter mutation:

```text
raw $0B       gate $13AA8 -> CLR.W +$22 at $13AB2
raw $0E p1    gate $13EF2 -> CLR.W +$22 at $13EFE
raw $14       gate $14C1A -> CLR.W +$22 at $14C38
raw $23       gate $15DFE -> CLR.W +$22 at $15E10
```

For the held-counter operation itself, current-turn `contact37=1` is sufficient in these four cases. Their later target/state side effects remain separate fidelity requirements.

## Helper-gated clear-and-advance cases

### raw `$0E` phase0
After contact `$13DF8`:

```text
JSR $11152
BCC -> bypass active clear path
JSR $11412
BCS -> bypass active clear path
CLR.W +$22 at $13E20
```

Therefore the demonstrated clear path requires:

```text
contact37
AND carry($11152) = SET
AND carry($11412) = CLEAR
```

### raw `$35`
After contact `$1780A` the same helper polarity precedes `$17836 CLR.W +$22`:

```text
contact37
AND carry($11152) = SET
AND carry($11412) = CLEAR
```

### raw `$10`
After contact `$142D8` and probe clear:

```text
JSR $11152
if carry SET:
    CLR.W +$22 at $142FA
else:
    JSR $11192
    if carry SET:
        CLR.W +$22 at $1435E
    else:
        no demonstrated clear on that branch
```

Thus the clear predicate is:

```text
contact37 AND (carry($11152) SET OR carry($11192) SET)
```

where `$11192` is evaluated only on the `$11152` carry-clear branch.

### raw `$46/$47`
After contact `$144F0`:

```text
JSR $11152
if carry SET:
    JSR $11412
    carry CLEAR -> CLR.W +$22 at $1451E
    carry SET   -> no clear on that route
else:
    JSR $11192
    carry SET   -> CLR.W +$22 at $14582
    carry CLEAR -> no clear on that route
```

Therefore the demonstrated clear predicate is:

```text
contact37 AND (
    (carry($11152) SET AND carry($11412) CLEAR)
    OR
    (carry($11152) CLEAR AND carry($11192) SET)
)
```

## raw `$13` finite re-arm is also helper-gated
Raw `$13` contact gate `$14A72` calls `$11152` before the `$0018` counter write.

The finite re-arm path is:

```text
contact37
AND carry($11152) = SET
-> target state4/word07 + damage path
-> MOVE.W #$0018,+$22 at $14AA6
```

If `$11152` returns carry clear, control branches away from the `$0018` write into a different state/subtype side path. Therefore raw `$13` cannot be modeled as `contact37 -> SET_FINITE(0x18)` unconditionally.

## raw `$0E` phase2
WIP144 remains authoritative: phase2 contact is `HOLD_STATE_TRANSITION`, not a counter release, and its exact state outcome is helper/branch dependent.

## Updated callback-operation matrix

```text
raw/phase   operation on demonstrated active path   sufficient predicate
--------------------------------------------------------------------------
0B / 0      CLEAR_AND_ADVANCE                       contact37
0E / 0      CLEAR_AND_ADVANCE                       contact37 & C11152 & !C11412
0E / 1      CLEAR_AND_ADVANCE                       contact37
0E / 2      STATE_TRANSITION                        branch-dependent (WIP144)
10 / 0      CLEAR_AND_ADVANCE                       contact37 & (C11152 | C11192*)
13 / 0      SET_FINITE(0018)                        contact37 & C11152
14 / 0      CLEAR_AND_ADVANCE                       contact37
23 / 0      CLEAR_AND_ADVANCE                       contact37
35 / 0      CLEAR_AND_ADVANCE                       contact37 & C11152 & !C11412
46/47 / 0   CLEAR_AND_ADVANCE                       contact37 & ((C11152 & !C11412) | (!C11152 & C11192))
```

`C11192*` for raw `$10` is evaluated only if `C11152` is clear.

## Runtime consequence
Before enabling physical held releases, native runtime must represent the required helper carry results (or prove an equivalent native predicate) and feed them into the typed callback operation. Contact37 alone is insufficient for raw `$0E` phase0, `$10`, `$13`, `$35`, `$46/$47`.

This removes another source of false-positive release from the old boolean scaffold.

## Safety
No runtime code changed. No physical release enabled. No 32-tick fallback removed. No `tick==12` migration. No ROM bytes persisted.

## ROM anchors
`$13AA8..`, `$13DF8..$13E20`, `$13EF2..$13EFE`, `$142D8..$1435E`, `$14A72..$14AA6`, `$14C1A..$14C38`, `$15DFE..$15E10`, `$1780A..$17836`, `$144F0..$14582`; helpers `$11152/$11192/$11412`.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
