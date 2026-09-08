# WIP124 CHECKPOINT — first-selectable Ground Z normalization window

## Scope
ROM-EXACT temporal reduction of inherited `+$0E` provenance before Ground selection. No runtime release/timing change and no BIN/CUE.

## Closed scheduler fact
Ground dispatcher `$DCDE` requires actor `+$20.bit7` already set.

When any callback writes a fresh main state in low `+$20`, bit7 is clear. On the next actor pass, `$F4C2`:
1. observes `+$20.bit7` clear;
2. snapshots state history;
3. sets `+$20.bit7`;
4. calls `$F518` before the state callback.

Therefore Ground cannot be selected on the first input pass immediately after a fresh state0 transition. Before the first state0 Ground-selectable input frame, the actor has passed through `$F518` at least once.

## Exact floor-normalization algebra
When `$F518` enters `$280DC` (`+$01 != 0`) and actor `+$32.bit5` does not bypass geometry:

```text
D2 = Z + probe42
```

For the floor-classifier variants used by the existing prone resolver, when `D2` is below threshold T (`$0140` or `$0100`):

```text
+$3C = T - (Z + probe42)
+$3C += probe42
      = T - Z
```

Then `$F542` applies:

```text
Z += +$3C
  = T
```

Thus a correcting pass lands actor `+$0E` exactly on the floor threshold, independently of the inherited probe value.

## Remaining bounded cases
This does NOT prove universal floor Z before selection. The inherited-Z blocker is now restricted to cases where the pre-selectable `$F518` pass does not normalize Z, specifically:
- `+$01 == 0`, so `$280DC` is skipped;
- actor `+$32.bit5 == 1`, so `$280DC` bypasses geometry;
- or candidate `Z + probe42` is already at/above the floor threshold, so the BCC path performs no floor correction.

State0 callback later clears probe42 at `$11500`, but that clear occurs after the first `$F518` pass; therefore a positive inherited probe can in principle participate in the no-correction candidate test and must not be erased retroactively.

## Consequence
For raw `$0B/$14/$23/$35/$46/$47`, inherited Z is no longer an open arbitrary value problem: any provenance that proves the pre-selectable pass executed geometry with candidate below T is ROM-EXACT floor-normalized and can drop the Z blocker. Only the bounded skip/no-correction provenance classes remain to audit.

## ROM anchors
`$DCDE`, `$F4C2..$F4F2`, `$F518..$F546`, `$280DC..$2811E`, floor paths `$281D8..$281F2`, `$28288..$282A6`, `$28366..$28380`, `$28570..$2858A`, state0 probe clear `$11500`.

## Safety
No live release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
