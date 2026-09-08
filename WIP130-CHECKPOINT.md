# WIP130 CHECKPOINT — contact37/floor-correction equivalence

## Scope
ROM-EXACT closure of the relation between floor candidate `Z + probe42`, correction `+$3C`, and current-turn contact bit `+$37.bit4` in the four floor-classifier variants used by the prone resolver. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## Closed
`$280DC` builds:

```text
D2 = +$0E(A0) + +$42(A0)
```

The four floor branches are structurally identical apart from threshold:

- `$281D8`: threshold `$0140`, `BSET #4,+$37` at `$281DE`;
- `$2828C`: threshold `$0100`, `BSET #4,+$37` at `$28292`;
- `$28366`: threshold `$0140`, `BSET #4,+$37` at `$2836C`;
- `$28570`: threshold `$0100`, `BSET #4,+$37` at `$28576`.

For every variant:

```text
if D2 >= T:
    no Z correction
    +$37.bit4 is not set by the floor branch

if D2 < T:
    +$37.bit4 = 1
    +$3C = T - D2
    +$3C += probe42
```

Since `D2 = Z + probe42`, the correcting branch simplifies to:

```text
+$3C = T - Z
```

and the later common `$F542` application lands Z exactly at T.

## Consequence
The prior `Z+probe42 >= T` case is **not an unknown contact branch**. It is an exact current-turn `contact37=false` outcome. Physical contact/release becomes eligible only on a turn where the floor branch actually performs the below-threshold correction and sets bit4.

This separates two concerns:
1. **contact timing rule is structurally closed** by the floor resolver;
2. **inherited Z provenance remains necessary** only to know on which turn the candidate crosses below T for actions that do not rebase Z.

For controller-state1 Ground entry from WIP129, `+$01=1` and primary-wrestler bit5 provenance is closed clear; therefore the first post-selection `$F518` deterministically evaluates this rule. A candidate at/above T simply remains held for that turn; it is not a reason to synthesize contact or release.

For state0 Ground entry, `+$01=0` still skips `$280DC`, so predecessor Z provenance remains independently blocking there.

## ROM anchors
`$280DC..$2811E`, `$281D8..$281F2`, `$2828C..$282A6`, `$28366..$28380`, `$28570..$2858A`, common correction application `$F542`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
