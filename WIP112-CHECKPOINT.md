# WIP112 CHECKPOINT — raw23 target producer split

## Scope
ROM-EXACT closure of the previously ambiguous raw `$23` linked-target state4 producer. No live damage/release change and no BIN/CUE.

## Closed
The two direct target producers belong to distinct callback routes:

- `$15F70` -> linked target main state4 + `WORD $0007`.
  - active non-`+$60.bit7` route;
  - requires current contact gate;
  - helper `$11412` carry clear;
  - helper `$11152` carry set;
  - the special combination actor `+$33.bit0=1`, target `+$33.bit0=1`, actor `+$33.bit2=0` instead takes the state5 choreography and therefore does not produce `$07`.
- `$1601A` -> linked target main state4 + `WORD $0009`.
  - active actor `+$60.bit7` branch;
  - requires target `+$32.bit4` or actor `+$FE.bit7`.

The `$15F70` branch also has exact pending damage `$0014`, but timing is deliberately not wired live while Ground contact timing remains provisional.

Added `arcade_raw23_target_state4_producer()` and `arcade_raw23_target_producer_t`.

## Regression
- Test392 PASS — neutral-entry held blocker classification unchanged.
- Test393 PASS — inherited-Z prehold blocker unchanged.
- Test394 PASS — neutral-entry Z guard unchanged.
- Test395 PASS — `$15F70/WORD07` and `$1601A/WORD09` producer split.

## Safety
- No physical release enabled.
- No 32-tick held fallback removed.
- No `tick==12` damage change.
- No ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP112-source.zip`
SHA256 `e82bc34c4b5d2ea303cec65fc178b0f0d20a811dec7381b54ad9d3df42498a94`

## Next
Return to predecessor/common-physics provenance of inherited `+$0E` for raw `$0B/$14/$23/$35/$46/$47`, while using the now-closed raw23 producer predicates for victim-state fidelity.
