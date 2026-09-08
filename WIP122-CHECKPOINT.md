# WIP122 CHECKPOINT — raw46/raw47 exact contact and terminal lifecycle

## Scope
ROM-EXACT closure of the shared raw `$46/$47` contact tree, raw-specific side effects, fallback, and `+$25==$FE` terminal behavior. Research/checkpoint only; no live timing migration and no BIN/CUE.

## Closed contact tree
At shared contact37 gate `$144F0`:

### Branch A — linked target state4
Requires:
- `$11152` carry set;
- `$11412` carry clear.

Effects:
- linked target main state4;
- target low state4 word is transformed by the existing branch (`+$64 -= 2` after state4 assignment path);
- target pending damage `+$68=$000A` (10);
- target `+$C7 += 1`;
- attacker `+$60.bit7` set;
- effects `$2B/$32` emitted.

Raw-specific counter:
- raw `$46` (`+$61==$46`) -> target `+$E0 += 1`;
- raw `$47` -> target `+$E2 += 1`.

### Branch B — linked target state5
Requires:
- `$11152` carry clear;
- `$11192` carry set.

Effects:
- linked target main state5;
- target `+$60=$0077`;
- target pending damage `+$68=$000A` (10);
- target `+$C7 += 1`;
- attacker `+$1B=$0C`.

### Fallback — attacker-local
All other resolved-contact helper combinations reach the attacker fallback:
- attacker main state4 + word `$0005`;
- attacker pending damage 3;
- global `$1C15D2=$0F1A`;
- attacker `+$26.bit7` set;
- facing bit7 flipped;
- effect `$32`.

Raw `$46` only: fallback additionally applies the `$FFD0` impulse through the existing helper call at `$14642`; raw `$47` skips that extra impulse.

## Terminal
At `+$25==$FE`, shared `$145CC` terminal path:
- attacker main state7;
- attacker raw `$04=$0068`;
- attacker `+$26.bit7` set;
- `$10BD0` called with `D0=$18`, `D1=-1`, `D2=0`.

## Consequence
Raw `$46/$47` post-contact/terminal behavior is structurally closed. Remaining live barrier is inherited Z provenance and faithful contact/release timing.

## ROM anchors
`$144F0`, `$1450C`, `$14518`, `$14532`, `$14562`, `$14578`, `$1459C`, `$145CC`, `$14606`, `$14642`.

## Safety
No physical release enabled. No held fallback removed. No `tick==12` timing migration. No ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
