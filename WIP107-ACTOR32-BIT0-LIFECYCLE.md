# WIP107 — actor +$32.bit0 lifecycle (ROM-EXACT research checkpoint)

Authority: WWF WrestleFest main 68000 program reconstructed from `wwfwfest.zip`.
SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`

## Direct mutation inventory

Executable ROM contains exactly:

```text
SET:
$1179E  BSET #0,+$32(A0)
$188D2  BSET #0,+$32(A0)

CLEAR:
$12C68  BCLR #0,+$32(A0)
$18808  BCLR #0,+$32(A0)
$18AE4  BCLR #0,+$32(A0)
$19728  BCLR #0,+$32(A0)
```

No other direct `BSET/BCLR #0,+$32(A0)` exists in the 0x80000-byte main program.

At `$188D2` the mutation is explicitly paired:

```text
BSET #0,+$32(A0)
BCLR #0,+$32(A3)
```

so bit0 can be transferred between actor roles and must be treated as persistent actor lifecycle state, not a callback-local boolean.

## Relation to Ground contact37

WIP106 proves geometry families `$2818E/$2831C` begin with:

```text
BTST #0,+$32(A0)
BNE  skip-classification
```

Therefore raw held actions routed through either family need exact bit0 provenance in addition to bit2, Z and probe42. Families `$28288/$2851E` do not use this early bit0 gate.

## Safety

This checkpoint does not claim which of the six bit0 mutation paths precede any particular Ground raw action. It therefore does not remove a fallback. It converts the new contact37 dependency into a finite producer/clear problem suitable for reachability tracing.

## Validation

Local Test389 exhaustively scans the ROM for direct bit0 SET/CLEAR encodings and validates the `$188D2` A0/A3 transfer.

No runtime code changed. No BIN/CUE generated. No ROM bytes included.