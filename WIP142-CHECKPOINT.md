# WIP142 CHECKPOINT — exact `+$25==$FE` terminal provenance

## Scope
ROM-EXACT closure of the terminal `+$25==$FE` value consumed by many held/action callbacks. This complements WIP139/WIP141: `+$25` can represent previous controller phase, but it can also become `$FE` through the common actor descriptor engine by a **word write to `+$24/+25`**. Research/checkpoint only; no live timing change and no BIN/CUE.

## Big-endian field rule
Actor `+$24/+25` is one 16-bit word:

```text
+$24 = high byte
+$25 = low byte
```

Therefore:

```text
MOVE.W #$00FE,+$24(A0)
```

sets exactly:

```text
+$24 = 00
+$25 = FE
```

This writer form is distinct from the `MOVE.B +$23,+$25` controller copy audited in WIP139.

## Exact writers
Static ROM audit finds two immediate word writers of `$00FE` to actor `+$24`:

- `$1AF28`
- `$1C10A`

`$1AF28` belongs to a separate small object/table animation helper around `$1AF04`.

The fighter-relevant writer is `$1C10A` inside the common actor descriptor/frame engine `$1C0C8..$1C126`.

## Common actor descriptor terminal
The main actor dispatcher `$1C03E` resolves the active descriptor according to actor main-state domain and then calls its callback followed by the common frame updater `$1C0C8`.

The exact terminal sequence is:

```text
$1C0EC  if +$22 == $FF00: hold; do not advance
$1C0F4  --+$22
$1C0FA  ++$24
$1C0FE  D0 = +$24
$1C102  compare D0 with descriptor phase-count at +$06(A1)
$1C106  if still in range -> continue descriptor step
$1C10A  MOVE.W #$00FE,+$24(A0)
```

Thus `+$25==$FE` is produced exactly when the common descriptor phase index advances past the descriptor's terminal phase, except for the documented special descriptor-type branch that clears/restarts instead.

This is **not** `prone_clock.finished` by assumption; it is the ROM's own common descriptor terminal state, represented in actor `+$24/+25`.

## Relationship to controller scheduler
Two independent exact mechanisms therefore feed `+$25`:

```text
A. controller path $5B66:
   +$25 = old +$23

B. common descriptor terminal $1C10A:
   WORD +$24/+25 = $00FE
   -> +$25 = FE
```

A held callback that tests `+$25==$FE` is therefore observing a value whose provenance must be determined from the active common actor update path; it is unsafe to treat every `+$25` value as only a controller phase.

## Ground consequence
The ubiquitous held terminal comparisons against `$FE` now have a direct ROM producer in the common fighter descriptor engine. This closes the **static producer** of the terminal value.

It does not yet authorize replacing live held timing with the Saturn `prone_clock.finished` flag: the port must first prove that its descriptor phase lifecycle is aligned with the ROM `+$22/+24/+25` update, including `$FF00` hold handling and callback ordering.

## ROM anchors
Actor dispatcher `$1C03E..$1C0C6`; common frame updater `$1C0C8..$1C126`; terminal writer `$1C10A`; auxiliary writer `$1AF28`.

## Safety
No runtime field changed. No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
