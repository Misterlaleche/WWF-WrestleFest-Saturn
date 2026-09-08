# WIP137 CHECKPOINT — raw35/raw46/raw47 chained contact timing

## Scope
ROM-EXACT numerical closure of first floor-contact timing for chained-Ground provenance (WIP132) for raw `$35` and shared raw `$46/$47`. Starting high-word Z is exact floor threshold T; fractional `+$10` may be arbitrary. Research/checkpoint only; no live release/timing change and no BIN/CUE.

## raw `$35`
WIP131/ROM callback `$1778E` initializes:

```text
+$5C=$0600
+$5E=$0048
```

The active path sets `+$01=2`. While vertical velocity is nonnegative, `$177F8..$17804` arms `probe42=+$20`; the value remains until contact and is cleared at `$17814`.

For n completed movement integrations:

```text
Delta35(n) = [ n*$0600 - $0048*n*(n-1)/2 ] / 256 px
```

Contact requires `high16(Z) + $20 < T`, equivalently descent strictly below `T-32` at the high-word classifier.

At n=48:

```text
Delta35(48) = -29.25 px
```

Even with the smallest fraction, high-word +32 is not below T.

At n=49:

```text
Delta35(49) = -36.75 px
```

Adding any fractional F<1 px still leaves the candidate strictly below T-32. Therefore the next pre-callback geometry pass necessarily sets contact37. Fractional `+$10` cannot shift this turn.

## raw `$46/$47`
Shared first phase `$14458` initializes:

```text
+$5C=$0600
+$5E=$0048
```

The initial held/contact callback `$144F0` uses the inherited/neutral probe (zero in the WIP132 state0 chained provenance); its later `+$20` probe at `$146E6` belongs to a secondary phase and is not the initial contact source.

Using the same vertical recurrence but contact threshold `high16(Z) < T`:

At n=43:

```text
Delta(43) = +4.03125 px
```

so contact is impossible for every fraction.

At n=44:

```text
Delta(44) = -2.0625 px
```

Even adding the maximum subpixel fraction leaves Z below T. Therefore the next pre-callback geometry pass necessarily sets contact37 after 44 completed integrations.

## Consequence
For WIP132 chained-Ground starting provenance:
- raw `$35`: first guaranteed contact after 49 completed vertical integrations;
- raw `$46/$47`: first guaranteed contact after 44 completed integrations;
- both timings are invariant over all possible retained `+$10` fractions.

Combined with WIP136, three held timing families now have exact fraction-invariant contact counts for chained entry: raw `$0B`, raw `$35`, raw `$46/$47`.

This does not authorize global live release: first-entry provenance outside WIP132 and exact phase/lifecycle wiring in the Saturn runtime remain separate requirements.

## ROM anchors
raw35 `$1778E..$17814`; raw46/47 `$14458..$145CC`; secondary probe `$146E6`; common movement `$2252..$22BE`; actor ordering `$F518..$F55E`; floor resolver `$280DC`.

## Safety
No runtime change. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
