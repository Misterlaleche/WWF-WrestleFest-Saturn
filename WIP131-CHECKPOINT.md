# WIP131 CHECKPOINT — inherited-Z held launch profiles

## Scope
ROM-EXACT closure of the vertical-motion initialization used by the six held Ground actions that still inherit starting `+$0E`: raw `$0B/$14/$23/$35/$46/$47`. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Common Z integrator
Per-frame common movement `$2208` dispatches from actor `+$01`:
- mode 0: no movement;
- mode 1: X/Y movement only;
- mode 2: integrates X/Y/Z.

In mode 2, `$2252..$22BE`:
- adds vertical velocity `+$5C` into Z;
- subtracts gravity/acceleration `+$5E` from `+$5C` each pass.

Thus once a held callback sets `+$01=2`, subsequent Z evolution is fully determined by starting Z plus `+$5C/+$5E`.

## Exact held profiles
### raw `$0B`
First-entry path `$13A2E..$13AA4` calls `$258E` with motion profile `$23`.
Profile `$23` at `$269C` yields:
- `+$5C=$0400`;
- `+$5E=$0058`;
- `$258E` sets `+$01=2`.

### raw `$14`
First-entry `$14B78..$14BDA` initializes:
- `+$5C=$0500` at `$14BA0`;
- `+$5E=$0048` at `$14BA6`;
- `+$01` initially cleared.
On the active follow-up path, when `+$24==0 && +$22==0`, `$14BEA` sets `+$01=2` before the `$14C1A` held/contact gate.

### raw `$23`
Phase path `$15DB8..$15DFE` initializes:
- `+$5C=$0500` at `$15DE2`;
- `+$5E=$0048` at `$15DE8`;
- `$15DF4` sets `+$01=2` before the preparatory held gate `$15DFE`.

### raw `$35`
Active held phase `$1778E..$1780A` initializes:
- `+$5C=$0600` at `$177A4`;
- `+$5E=$0048` at `$177AA`;
- `+$01` initially cleared;
- when `+$24==0 && +$22==0`, `$177DA` sets `+$01=2` before the `$1780A` contact gate.

### raw `$46/$47`
Shared callback `$14458..$144F0` initializes:
- `+$5C=$0600` at `$1446E`;
- `+$5E=$0048` at `$14474`;
- `+$01` initially cleared;
- when `+$25==0 && +$22==0`, `$144B8` sets `+$01=2` before the `$144F0` contact gate.

## Consequence
For every remaining Z-blocked held Ground action, the unknown is no longer arbitrary motion behavior. The ROM model is now:

```text
unknown inherited starting Z
+ exact action-specific launch velocity
+ exact gravity
+ exact mode-2 activation gate
+ exact per-turn floor/contact rule from WIP130
```

Therefore the remaining blocker is reduced to **starting-Z provenance at selection/launch**, not the subsequent trajectory or contact criterion.

This also shows why a fixed 32-tick release cannot be ROM-EXACT: different held raws have different launch velocities/gravity and contact occurs on the first turn their evolving `Z+probe42` crosses below the stage threshold.

## ROM anchors
Common movement `$2208/$2252..$22BE`, profile loader `$258E/$25B2`, raw0B `$13A9A..$13AA4`, raw14 `$14B78/$14BA0/$14BA6/$14BEA`, raw23 `$15DE2/$15DE8/$15DF4`, raw35 `$177A4/$177AA/$177DA`, raw46/47 `$1446E/$14474/$144B8`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
