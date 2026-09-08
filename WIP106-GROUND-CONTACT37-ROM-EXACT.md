# WIP106 — Ground contact37 bit4 classifier (ROM-EXACT research checkpoint)

Authority: WWF WrestleFest main 68000 program reconstructed from `wwfwfest.zip`.
SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`

## Closed structure

`$280DC` clears/rebuilds geometry state before callback dispatch. The common pre-dispatch computes:

```text
D0 = +$06(A0)
D1 = +$0A(A0) + +$40(A0)
D2 = +$0E(A0) + +$42(A0)
```

Therefore Ground held probe `+$42` directly biases the quantity tested by current-turn `+$37.bit4`.

The classifier dispatch index is:

```text
index = (WORD[$1C007E] & 7) + (BYTE[A0+$32] & 4)
```

Table `$28154`:

```text
0->$2818E 1->$28288 2->$2831C 3->$2818C 4->$28480 5->$2851E
6->$2818C 7->$2818C 8->$2818C 9->$2818C 10->$2818E 11->$28288 12->$28480
```

Direct `BSET #4,+$37(A0)` sites: `$281DE/$28292/$2836C/$28576`.

- `$2818E`: if `+$32.bit0` clear, bit4 when `D2 < $0140`.
- `$28288`: bit4 when `D2 < $0100`.
- `$2831C`: if `+$32.bit0` clear, bit4 when `D2 < $0140`.
- `$2851E`: bit4 when `D2 < $0100`.
- `$28480`: does not set bit4.

Because actor32 bit2 adds 4 to the dispatch index, it changes the geometry classifier family itself; it is not merely a release boolean.

## Ground consequence

```text
raw establishes/rebases +$0E
raw establishes/inherits +$42
actor32 bit2 selects classifier family
D2 = +$0E + +$42
current actor turn rebuilds +$37.bit4
callback consumes +$37.bit4
```

This closes the structural relationship among Z, probe42, actor32 and contact37. It does not yet authorize global physical release because several held raws retain inherited Z/probe provenance, `+$32.bit0` is an additional exact gate in two classifier families, and the Saturn runtime must reproduce the relevant global `$1C007E & 7` mode value.

## Raw $0B retained closure

For its audited active held branch:

```text
$133C6  MOVE.W +$0E(A2),+$0E(A0)
$13366  MOVE.W #-$20,+$42(A0)
```

so raw `$0B` does not require inherited Z/probe provenance there. Actor32/classifier state remains the blocker.

## Validation

Local Test388 verifies dispatch arithmetic, table entries, common `D2=+$0E + +$42` construction, all four bit4 setters, thresholds and the `+$32.bit0` early gate on `$2818E/$2831C`.

No runtime code changed. No BIN/CUE generated. No ROM bytes included.