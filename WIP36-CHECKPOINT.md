# Test255 WIP36 — native prone shadow-state integration scaffold

## Integrated
- `fighter_t` now carries persistent native shadows for ROM actor `+$0E` (`raw_z_shadow`), `+$42` (`probe42_shadow`) and `+$32` (`actor32_shadow`).
- `fighter_t` also carries an exact action animation clock for callback-held prone descriptors.
- Normal fighter startup seeds shadow raw Z to `$0140`, matching the common fighter setup constant pinned in ROM; no ground-state reset is introduced.
- Entering a raw prone action applies only the WIP35-proven target-Z rebases: raw `$0E` target Z +0, raw `$10` target Z +$50, raw `$13` target Z +$20.
- The native action clock preserves exact finite descriptor timing and refuses to advance a `$FF00` frame unless an explicit release event is supplied.
- Normal-combat `$D12` contact37 composition is now exposed as `arcade_contact37_from_base_context(stage, actor32, raw_z, probe42)`.
- `ground_attack_update()` now computes that exact contact predicate over the persistent shadows and advances finite raw descriptor phases through the exact action clock.

## Not yet promoted
- `$FF00` release is deliberately still disabled in gameplay. Several callbacks program `+$42` in internal callback subphases rather than simply on entry to the held animation frame; releasing directly from frame identity would be wrong.
- The legacy 32-tick state timeout therefore remains for callback-held ground actions until those callback subphases are modeled.
- Raw damage timing/callback side effects remain provisional.
- PICK UP victim handoff remains provisional.

## Tests
- Test314: callback-held action clock advances a `$FF00` frame only on explicit release.
- Test315: normal-combat contact37 composes stage mode + persistent actor32/raw-Z/probe42 shadows without a timeout.
- Test290 re-run passes after the integration scaffold.

No ROM image is included. No BIN/CUE generated.
