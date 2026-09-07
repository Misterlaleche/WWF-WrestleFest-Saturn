# WIP53 — raw $10 prone selection surface

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT / table-exact additions
- Raw `$10` prone selection is narrowed to exactly two logical selection IDs in the Test70 universal action tables: selection IDs `4` and `9`.
- For both, raw `$10` is selected only from prone category `7` with fresh physical button mask `1`.
- Test75 category rules therefore require target `+$21=4`, target `+$65` in the `9/7` family, and target `+$70 != 0` for this raw-$10 selection surface.
- The exhaustive actor `+$32 bit2` mutation inventory contains no immediate set/clear site in classifier `$E0B8..$E17C`; selector-local actor32 bit2 is preserved.

## Still open
- The incoming actor `+$32 bit2` VALUE before the prone selector/caller chain remains unproven.
- Therefore raw `$10` live `$FF00` release remains blocked by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No `release=true` activation, no 32-tick fallback removal, and no `tick==12` replacement is made.

## Tests
- Test329..Test337 directed regression PASS.
- New Test337 pins raw `$10` selection IDs/category/button/target family and preserves the live-release safety guard.

## Next
Trace the caller/main-state path feeding the prone selector for selection IDs 4/9 and prove the incoming `+$32 bit2` value. Only then may raw `$10` be considered for isolated live-release activation.
