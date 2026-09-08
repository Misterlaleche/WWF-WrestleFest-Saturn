# WIP139 CHECKPOINT — actor +$23/+$25 control-phase scheduler closure

## Scope
ROM-EXACT closure of the control-phase provenance consumed by held Ground callbacks, especially raw `$14` gate `+$25==3`. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed
The common controller update at `$5B66` executes, in order:

```text
+$25 = old +$23
D0 = old +$23
D0 *= 11
D0 += +$A8
+$23 = TRANSITION_TABLE[D0]
```

The transition table base is selected globally:
- `$62BC` default;
- `$6314` when global `$1C015E.bit1` is set;
- `$6382` when that mode is active and global `$1C0161.bit0` selects the alternate table.

The routine then applies the `$1C11F4` occupancy/state arbitration and may restore `+$23=+$25` on the documented occupied/conflict path. Thus `+$25` is the previous control phase and `+$23` is the newly selected phase for the next pass.

A second controller path at `$607E` likewise writes `+$23` from its selected table and immediately mirrors it into `+$25`; this confirms that the field pair belongs to the control scheduler, not to the raw animation descriptor clock.

Actor `+$A8` is a dynamic table-column/offset input. At `$64F8`, it is derived from relation/control data (`NOT.W`, mask to low-byte domain, later low nibble mask on `+$A9`) rather than from a fixed raw-action frame counter.

## Ground consequence
Raw `$14` contact code is gated by:

```text
+$25 == 3
probe42 = +$20
current-turn +$37.bit4
```

The value `3` must therefore be reached through the controller phase scheduler above. It is unsafe to equate this gate with:
- descriptor frame index 3;
- elapsed animation ticks;
- a fixed number of vertical integrations;
- or `prone_clock.finished`.

The vertical parabola from WIP131/WIP135 remains exact once mode2 is active, but raw `$14` physical contact timing is the intersection of two independently exact conditions:

```text
controller old-phase +$25 == 3
AND
floor classifier sets current-turn contact37
```

This explains why a purely kinematic fixed-frame replacement would still be wrong even after Z trajectory closure.

## ROM anchors
`$5B3E..$5C1E`, especially `$5B66/$5B70/$5B74/$5B78`; table bases `$62BC/$6314/$6382`; arbitration `$5B7E..$5BDE`; secondary path `$6044..$6096`, especially `$6078/$607E`; `+$A8` producer `$64E8..$6502`; raw14 gate `$14C0A..$14C2A`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
