# Test255 WIP8 — exact interstitial speaker path and state-entry transition

## Corrected ROM interpretation

`$B0C8` starts with `BTST #0,+$1D(A0)` and immediately returns for an unflagged object. Therefore the three `$2C` presentation objects are all created by `$AE20`, but the custom speaker script is advanced only for the object selected by mode.

- mode 1: object 2 / right Demolition member; script `$B25E`; one event marker; audio cursor starts at byte offset 2 in `$B14A`, therefore command `$316A`.
- mode 2: object 0 / left Demolition member; script `$B1A8`; two event markers; audio cursor starts at byte offset 4, therefore commands `$316B` then `$3178`.

The previous `active_ticks` field has been renamed `raw_duration_sum`; it was the sum of duration bytes, not a proved wall-clock duration. A native `$B0C8` mirror now preserves the inclusive countdown behavior and is host-tested.

## State-entry transition

The exact `$0B1C` branch is represented by `arcade_attract_transition.c/.h`:

- entering state 1: `$AE20` mode 1 -> `$264E2` -> `$69C8` TAG records -> hold `$0100` via `$21E6` -> `$264E2`;
- entering state 2: set Royal selector byte `$1C0161=1`, `$AE20` mode 2 -> `$264E2` -> `$69C8` Royal records -> hold `$0100` -> `$264E2`.

These are setup transitions around the 14-state machine, not extra attract states.

## Graphical identity

ROM-derived pixel comparison identifies family `$33` / left as Smash (ID 4) and family `$31` / right as Crush (ID 5). This is retained as graphical identification; no direct nominal resource-ID table was found in the 68000 presentation path, so code does not rely on a synthetic `$31 -> 5` program mapping.

## Validation

All focused Test255 audits pass: 14-state tables, presentation audio, interstitial scripts, native `$B0C8` event consumption, exact `$2C` pack, TAG/Royal records producer, and state-entry transition.

Canonical Test254/V0.269 is unchanged. Test255/V0.270 remains WIP. No BIN/CUE generated.
