# WIP105 CHECKPOINT — actor32 SET predicate profiles

## Scope
ROM-EXACT bounded classification of the five actor `+$32.bit2` SET sites that previously had only coarse context labels. No runtime aliasing and no BIN/CUE.

## ROM authority
Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

## Closed
- `$0F3BC`: main-state2 `$F34C` control/init SET only on demonstrated orientation-change path.
- `$1C4CA`: conditional CPU/control transition; writes `+$20=2` before SET and requires its global/actor/table gates.
- `$1C52E`: conditional CPU/control transition after `$1F7F0` carry-clear path and `+$33.bit4` gate; writes `+$20=2` before SET.
- `$1CED0`: linked-target CPU/control transition with orientation/global/actor/table gates; writes `+$20=2` before SET.
- `$1D85A`: raw75-specific AI/control path with orientation/range/latch/helper gates.
- Added `arcade_actor32_set_predicate_profile()` and Test388.

## Consequence
None of these SET sites is a universal raw `$10` actor32 producer. Raw `$10` still preserves incoming actor32 locally; its exact predecessor history remains a separate reachability question.

## Regression
PASS Test305, Test329, Test385 (with verified ROM supplied temporarily), Test388.

## Safety
`program.bin` was a temporary symlink for ROM-dependent tests and was removed before packaging. No ROM bytes are persisted.

## Source archive
`WWF-Saturn-Test255-WIP105-source.zip`
SHA256 `b74469bcf66c918bfc257dd397ddb7954b789bfd6f704398d47ebaec3b3a83df`
