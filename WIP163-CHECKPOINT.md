# WIP163 CHECKPOINT — exact helper-carry contracts `$11152/$11192/$11412`

## Scope
ROM-EXACT closure of the three helper predicates consumed by held Ground callbacks. Research checkpoint only: no runtime release enabled, no 32-tick fallback removed, no `tick==12` change, no BIN/CUE.

## ROM authority
Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## `$11152` — linked state4 subtype + profile10 overlap
Exact sequence:

```text
A1 = LONG +$26(A0)
BYTE +$21(A1) must be $04
BYTE +$65(A1) must be $08 or $09
D0 = $000A
JSR $E958
```

Return carry is SET only when all state/subtype tests pass and `$E958` reports overlap. Failure of any predicate returns carry CLEAR.

Exact contract:

```text
C11152 =
    linked_target_main_state == 4
    AND linked_target_low_subtype in {08,09}
    AND profile10_overlap(A0,A1)
```

## `$11192` — linked state5 +$61 marker + profile10 overlap
Exact sequence:

```text
A1 = LONG +$26(A0)
BYTE +$21(A1) must be $05
BYTE +$61(A1) must be $61
D0 = $000A
JSR $E958
```

Return carry is SET only when all predicates pass and profile10 overlap succeeds.

```text
C11192 =
    linked_target_main_state == 5
    AND linked_target_byte61 == $61
    AND profile10_overlap(A0,A1)
```

## `$11412` — reciprocal-link predicate
Exact sequence:

```text
A2 = LONG +$26(A0)
A2 = LONG +$26(A2)
CMPA.L A0,A2
```

Carry polarity is deliberately inverted relative to a boolean 'linked' name:

```text
reciprocal link equality -> carry CLEAR
mismatch                  -> carry SET
```

Therefore every WIP146 predicate using `!C11412` means exact reciprocal-link equality.

## `$E958` profile `$0A` geometry
`$E958` receives A0 as tested actor, A1 as linked actor, and D0 profile index. For profile `$0A`, table `$E9BA + 10*8 = $EA0A` contains signed words:

```text
left   = -8
right  = +104
top    = -72
bottom = +24
```

Facing comes from `A1 +$2E.bit7`.

When facing bit7 is clear:

```text
X interval relative to A1: (-8, +104]
Y interval relative to A1: (-72, +24]
```

When facing bit7 is set, the X pair is exchanged and negated:

```text
X interval relative to A1: (-104, +8]
Y interval relative to A1: (-72, +24]
```

The comparison asymmetry is exact ROM behavior: lower bound is strict, upper bound inclusive. `$E958` does not inspect Z.

## Ground consequence
Helper carries are no longer opaque timing inputs. They can be reproduced from exact live fields once the native runtime has validity for:

- linked actor identity / reciprocal link;
- target ROM-domain main state;
- target state4 low subtype for `$11152`;
- target `+$61` for `$11192`;
- A0/A1 X,Y and linked facing bit7.

WIP163 therefore removes geometry itself as a blocker. It does **not** yet make `$11192` live-ready because target `+$61==$61` provenance remains unresolved in the Saturn runtime.

## Validation
Local Test427 ROM-pins `$11152`, `$11192`, `$11412`, `$E958` facing transform and profile10 row `[-8,+104,-72,+24]`.

## Safety
- No physical held release enabled.
- No helper result synthesized from native state resemblance.
- No 32-tick fallback removed.
- `tick==12` unchanged.
- No ROM bytes persisted.
- No BIN/CUE generated.
