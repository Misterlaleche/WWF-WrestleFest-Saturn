# WIP68 — PICK UP raw $22 uses one exact ROM descriptor clock

Status: Ground Actions animation-clock checkpoint. No BIN/CUE. No ROM files.

## Closed
- `ground_pickup_begin()` resets the exact raw `$22` action clock.
- `ground_pickup_update()` advances that clock directly.
- `game_arcade_pose(ST_GROUND_PICKUP)` now reads the clock pose instead of independently resampling elapsed ticks.
- PICK UP completion is driven by `prone_clock.finished`.
- The exact six-frame descriptor remains 98 ticks total.

## Deliberately unchanged
- The victim handoff at native tick 18 remains **PROVISIONAL** and isolated.
- No claim is made that tick 18 is the arcade callback write point.
- No held Ground Action release was enabled.
- `tick==12` damage scheduling for Ground Attacks remains unchanged.

## Tests
- Test352 PASS: raw `$22` reset/tick/pose/finish all share one descriptor clock; total remains 98 ticks.
- Test231 PASS with full current game dependencies: PICK UP remains active for the full ROM visual clock.
- Python syntax PASS.
- Existing compile warnings are pre-existing style/sign warnings; no new link/runtime failure from this checkpoint.

## Next
Return to held Ground Actions inherited input closure. Where retained ROM evidence cannot prove an inherited producer, record the blocker and continue to another independently closable Ground Actions component rather than inventing state.

Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP68-source.zip`
SHA256: `f523eddec80a4542acce9fbc8c0d8b181abb4fe3df13f82923cfeb6fd9a03def`
