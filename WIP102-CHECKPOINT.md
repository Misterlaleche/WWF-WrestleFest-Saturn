# WIP102 CHECKPOINT — complete actor32 bit2 mutation context inventory

## Scope
ROM-EXACT context classification of the two remaining immediate actor `+$32 bit2` mutation sites that were still marked UNCLASSIFIED.

## Closed in WIP102
- `$11E04` is `BCLR #2,+$32(A0)` inside main-state 6 descriptor `$11DD0` / callback `$11DE0`.
- `$0F3BC` is `BSET #2,+$32(A0)` inside the distinct control routine beginning at `$0F34C`.
- The complete nine-site immediate actor32 mutation inventory now has bounded contexts; no entry remains `ARCADE_ACTOR32_CONTEXT_UNCLASSIFIED`.
- Added contexts `ARCADE_ACTOR32_CONTEXT_MAIN_STATE6_CALLBACK` and `ARCADE_ACTOR32_CONTEXT_CONTROL_F34C`.
- Added Test385 and updated Test329 expectations.

## Safety
This does NOT promote any `$1C4CA/$1C52E/$1CED0` CPU-dispatch SET to a live raw-$10 producer. Raw `$10` remains blocked until its exact controller-history sub-branch is proven.

## Regression
PASS Test305, Test329, Test385.

## Source materialization
`WWF-Saturn-Test255-WIP102-source.zip`
SHA256 `010787dbfd4371ee66b4318bf9844afb29922b1d3505d1f7acb2813d9ab10164`
