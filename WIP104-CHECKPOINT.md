# WIP104 CHECKPOINT — actor +$25 provenance / PICK UP clock guard

## Scope
ROM-EXACT audit of all `MOVE.B` writers to actor `+$25`, motivated by raw `$22` PICK UP terminal gate `+$25==$FE`.

## Closed in WIP104
- Exactly nine `MOVE.B` writers target `+$25(A0)` in the main program.
- Common machinery at `$5B66` and `$607E` performs `MOVE.B +$23(A0),+$25(A0)`.
- Other direct writers set explicit callback/state phase values (`1` at `$163DE`, `4` in the raw109 family).
- Therefore `+$25` is table/callback phase state and must NOT be aliased to the native exact animation descriptor clock.
- Added exhaustive Test387.

## Consequence for PICK UP
The WIP101 terminal gate `+$25==$FE` cannot be safely replaced by `prone_clock.finished`. The old native tick18 remains PROVISIONAL until the relevant `+$23/+25` phase lifecycle is reconstructed or another exact native event source is proven.

## Regression
PASS Test387, with WIP101 Test352/Test376/Test384 retained.

## Source materialization
`WWF-Saturn-Test255-WIP104-source.zip`
SHA256 `67ec855bcaee8788463d30a4c666a95e7f6de7602a3a0f11e67573f34ac5ad60`
