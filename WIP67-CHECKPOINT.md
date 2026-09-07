# WIP67 — finite raw Ground Actions terminate from the exact descriptor clock

Status: Ground Actions runtime timing checkpoint. No BIN/CUE. No ROM files.

## Closed
- Finite raw ground actions now terminate from `prone_clock.finished`, the same ROM-derived descriptor clock that drives their visible pose.
- This removes the duplicated native `tick >= finite_duration` terminal decision for finite raw actions.
- Callback-held `$FF00` actions deliberately retain the 32-tick compatibility fallback until their physical `+$0E/+$42/+$32/+$37` lifecycle is closed action-by-action.

## Safety
- No held release blocker changed.
- No `tick==12` damage timing changed.
- No raw `$47` damage/effect profile was moved to tick-based contact.
- PICK UP `$22` remains unchanged in this checkpoint.

## Tests
- Test351 PASS: finite raw action clock terminates exactly at `arcade_animation_finite_ticks()`, while held raw `$10` remains unfinished without explicit release.
- Tests319/334/335/349 PASS.
- Python syntax PASS.

## Next
Migrate finite PICK UP `$22` pose and terminal timing to the exact action clock while preserving its still-PROVISIONAL victim handoff.

Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP67-source.zip`
SHA256: `b86edbc4fcf068832f90ad64fb70feae64a410a55bd738164564e458ea49bae0`
