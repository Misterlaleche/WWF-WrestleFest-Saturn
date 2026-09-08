# WIP167 CHECKPOINT — direct `$1C007E` provenance and floor-contact eligibility

## Scope
ROM-EXACT direct-event census for global word `$1C007E`, which supplies the low-three-bit base index of the WIP106 floor/contact37 classifier. Research checkpoint only: no live release, no fallback removal, no `tick==12` change, no BIN/CUE.

## Direct absolute writer census
The main executable contains exactly 13 direct absolute immediate word writes to `$1C007E`:

```text
value $0003:
$0748 $07D6 $127E $5310 $58FE $5944 $69E0 $81D0 $84A6

value $0004:
$1B7A $7BD0 $AE56 $B646
```

No direct absolute immediate writer stores 0,1,2,5,... to this address.

The bulk byte-clear routine beginning `$1F22` clears the range from `$1C0060` through `$1C3AD3`, therefore it directly establishes `$1C007E=0` as part of that reset class.

This checkpoint deliberately does **not** claim that an as-yet-unclassified dynamically addressed store can never target the word. The positive result is an exact direct-event provenance contract: after a demonstrated bulk clear, value 0 is known until an intervening exact writer; after any one of the 13 direct writers, value 3 or 4 is known.

## Exact classifier consequence
WIP106 dispatches using:

```text
index = ($1C007E & 7) + (actor +$32.bit2 ? 4 : 0)
```

Table `$28154` gives:

```text
index0 -> $2818E
index3 -> $2818C (RTS)
index4 -> $28480
index7 -> $2818C (RTS)
index8 -> $2818C (RTS)
```

`$28480` contains no `BSET #4,+$37`; `$2818C` is an immediate RTS.

Therefore for the exact direct-event values:

```text
$1C007E=3 -> floor bit4 impossible for bit2 clear or set
$1C007E=4 -> floor bit4 impossible for bit2 clear or set
```

For direct-event provenance `$1C007E=0`:

```text
bit2 SET   -> index4/$28480 -> floor bit4 not produced
bit2 CLEAR -> index0/$2818E
```

and `$2818E` begins with:

```text
BTST #0,+$32(A0)
BNE  skip-classification
```

so its floor-contact path additionally requires actor32.bit0 CLEAR.

## Narrow valid contact context
Within this direct-event provenance class, the `$2818E` floor branch capable of generating held Ground `contact37.bit4` requires:

```text
$1C007E == 0
actor32.bit2 == 0
actor32.bit0 == 0
candidate (+$0E + +$42) < $0140
```

This is a much narrower exact context than a generic `contact37` boolean.

## Validation
Local Test431 exhaustively scans all `MOVE.W #imm,$1C007E` sites, pins the bulk-clear base, validates the `$28154` dispatch table, `$2818C` RTS, the `$2818E` bit0 gate and absence of a bit4 setter in `$28480`.

## Safety
- No dynamically addressed writer is excluded without proof.
- No global classifier mode is inferred from frontend/native mode names.
- No physical held release enabled.
- No 32-tick fallback removed.
- `tick==12` unchanged.
- No ROM bytes persisted.
- No BIN/CUE generated.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
