# WIP101 CHECKPOINT — raw $22 PICK UP terminal callback handoff

## Scope
ROM-EXACT audit and bounded model of raw-action `$22` PICK UP callback `$15B8C` using the user-supplied verified WrestleFest program ROM. No BIN/CUE generated. No ROM bytes are persisted.

## ROM authority
Reconstructed main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## Closed in WIP101
- `$15C32`: terminal handoff logic is active only when actor `+$25==$FE`.
- `$15C3C`: `+$FE.bit7` bypasses helper `$111C8` and reaches the target state-4 path.
- `$15C54`: otherwise helper `$111C8` is called; carry clear selects the alternate target state-5 path.
- Target state-4 path writes actor state7, target state4, then `ANDI.W #$00FF,+$64(A2)`: high flags clear while low subtype `+$65` is preserved.
- Alternate path writes target main state5 and target `+$60=$005F` without rewriting `+$64/+65`.
- Added `arcade_pickup_raw22_terminal_handoff()` and Test384.

## Still blocked / deliberately not wired live
- Existing native PICK UP victim handoff at tick18 remains PROVISIONAL.
- Runtime does not yet own exact live equivalents of actor `+$25`, `+$FE.bit7`, or helper `$111C8` carry at the callback terminal event.
- Therefore WIP101 does not replace tick18 and does not invent a callback-to-native-state alias.

## Regression
PASS Test352, Test376, Test384.

## Source materialization
`WWF-Saturn-Test255-WIP101-source.zip`
SHA256 `a254c9b65a3d79c34d902954105332d0ff073a346c4ece13655fac210eacbff7`
