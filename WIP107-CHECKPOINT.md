# WIP107 CHECKPOINT — prone-selection actor32 entry provenance

## Scope
ROM-EXACT actor `+$32.bit2` provenance at the prone-selection surface. No live release or native-state aliasing.

## Closed
- Input/control `$F18A` precedes actor callbacks `$F4C2`.
- Ground dispatcher `$DCDE` requires `+$20.bit7` already set.
- Fresh main-state0 therefore cannot select Ground until first-entry state0 callback has run.
- State0 init clears actor32 bit2 at `$114CC` and probe42 at `$11500`.
- Thus Ground selected from ROM main-state0 has exact actor32 CLEAR and probe42 ZERO.
- Main-state1 remains inherited pending predecessor closure.
- Added `arcade_prone_selection_actor32_entry_policy()` and Test390.

## Regression
PASS Test329, Test388, Test389, Test390.

## Safety
ROM used only as temporary test input; no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP107-source.zip`
SHA256 `52aa9b6625c5b913f7c5ef5b061dfbf39a75dfe16b88e7883d92c07d86f87c87`
