# WIP165 CHECKPOINT — chained raw `$14/$23` inherited-Z contact timing

## Scope
ROM-EXACT numerical closure of the two remaining chained-Ground inherited-Z launch cases not covered by WIP136/WIP137: raw `$14` and raw `$23` initial held gate. Research checkpoint only; no live release enabled, no fallback removal, no `tick==12` migration, no BIN/CUE.

## Authority
Uses WIP132 chained provenance:

```text
starting high-word Z = exact floor threshold T
fractional +$10      = arbitrary retained value in 0..$FFFF
state0 probe42       = 0 before the next Ground action
```

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## Raw `$23` initial held gate
WIP160/WIP161 establish the initial `+$44=1..4` held branch and first-entry motion:

```text
+$5C = $0500
+$5E = $0048
+$01 = 2
```

No `+$42=+$20` write belongs to the initial `$15DFE` held gate. In the WIP132 chained state0 provenance the inherited probe is therefore exactly zero.

Discrete displacement after n completed movement integrations is:

```text
Delta23(n) = [n*$0500 - $0048*n*(n-1)/2] / 256 px
```

At n=36, `Delta=+2.8125 px`: contact is impossible for every fraction.
At n=37, `Delta=-2.3125 px`: even adding the maximum retained fraction `<1 px` leaves the accumulator below T, so the next pre-callback floor pass necessarily sets current-turn contact37.

Exact chained result:

```text
raw23 initial held gate: first geometric contact after 37 completed integrations
```

and this turn is fraction-invariant.

## Raw `$14`
Exact first-entry motion:

```text
+$5C = $0500
+$5E = $0048
+$01 = 2
```

The active held branch requires previous control phase `+$25==3`, then writes `+$42=+$20` before testing current-turn `+$37.bit4`.

Geometric floor contact therefore requires:

```text
high16(Z) + 32 < T
```

At n=41, `Delta=-25.625 px`: contact is impossible for every fraction.
At n=42, `Delta=-32.15625 px`: exhaustive enumeration of all 65536 retained low words gives exactly 10240 contact fractions and 55296 non-contact fractions. Equivalent boundary: fraction `<0.15625 px` contacts; fraction `>=0.15625 px` does not.
At n=43, `Delta=-38.96875 px`: geometry necessarily sets contact37 for every possible fraction.

## Critical consequence
Raw `$14` does **not** have one exact fixed contact integration count for arbitrary chained fractional history:

```text
possible geometric contact at integration 42 depending on +$10
geometrically guaranteed by integration 43
```

Even then, descriptor release is independently gated by exact controller old-phase `+$25==3` (WIP139–WIP141). The real release is the intersection of:

```text
actor25 == 3
AND
current-turn contact37 == 1
```

A fixed 42- or 43-tick substitute would therefore still be non-ROM-EXACT.

## Validation
Local Test429 pins raw14 `$14BA0/$14BA6/$14BEA/$14C0A/$14C14/$14C1A`, raw23 `$15DE2/$15DE8/$15DF4/$15DFE`, verifies no `+$42=+$20` writer in the raw23 initial setup window, and exhaustively evaluates all 65536 fractional low words.

## Safety
- No physical held release enabled.
- No 32-tick fallback removed.
- No actor25 scheduler shortcut introduced.
- `tick==12` unchanged.
- No ROM bytes persisted.
- No BIN/CUE generated.
