# WIP125 CHECKPOINT — actor32 bit5 floor-bypass SET inventory

## Scope
ROM-EXACT closure of direct actor `+$32.bit5` SET sites relevant to the `$280DC` geometry/floor bypass. Research/checkpoint only; no live release or timing changes.

## Direct SET census
Static executable audit finds exactly three direct `BSET #5,+$32(A0)` sites:

### `$FABA`
- gated by actor control conditions including `+$33.bit2` and raw/control byte `+$61==$6A`;
- immediately writes actor main state5 (`+$20=$0005`) and raw/control word `+$60=$006A`;
- writes pending damage 8.

### `$FAE4`
- reached from the alternate controller branch after `ANDI.W #$00C3,+$32(A0)` (which first clears bits 2..5);
- sets `+$33.bit2` and then sets `+$32.bit5`;
- immediately writes actor main state5 and `+$60=$006B`;
- explicitly sets actor Z `+$0E=$0100`.

### `$1AE70`
- special callback initialization branch;
- clears actor `+$01`, sets `+$32.bit5` and bit6;
- explicitly positions actor (`X=$0278`, `Y=$0199`, `Z=$014E`).

## Clears/replacements observed
Direct executable operations capable of clearing/replacing bit5 include:
- full `CLR.W +$32` sites;
- masks `ANDI.W #$00C3,+$32` and `ANDI.W #$0087,+$32`, both clearing bit5;
- `$19C20 BCLR #5,+$32(A0)`;
- other full-word replacement writes enumerated by the actor32 audit.

## Consequence
The floor-bypass bit is not produced by normal state0/state1 neutral callbacks themselves. All direct SET provenance begins in bounded special/state5 or explicitly positioned paths. However state0 init does not itself clear bit5, so a blanket claim that neutral Ground entry always has bit5 clear is still unsafe until predecessor exit/clear reachability from these three SET families is closed.

Combined with WIP124, inherited-Z uncertainty is now bounded by:
1. predecessor `+$01==0` at the single pre-selectable `$F518` pass;
2. inherited bit5 from one of these three bounded SET families without an intervening clear/replacement;
3. or a no-correction candidate `Z+probe42 >= floor threshold`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
