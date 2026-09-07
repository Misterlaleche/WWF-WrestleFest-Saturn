# WIP85 — live victim state4 clock ownership and exact visual phases

## Runtime
- `fighter_t` now owns `state4_clock`, an exact descriptor clock associated with the mirrored ROM state-4 `+$64/+65` subtype.
- Every exact mirrored state-4 producer resets that clock to the corresponding subtype descriptor.
- A global per-combat-tick state4 clock pass advances only finite descriptor time with `release_hold=false`.
- Callback-held `FF00` frames therefore never auto-release.
- `game_arcade_pose()` now renders the exact state-4 clock pose for valid in-ring `ST_DOWN/ST_RECOVER` carrier states when the clock has a valid recovered pose.
- This does not assert that native enum values equal ROM main state 4; `state4_64_valid` remains the proof gate that a ROM state-4 producer was actually mirrored.

## Visible consequences
- raw `$13/$14` exact victim subtype `$07` displays its recovered held pose `$16` and remains held until a future proven callback release.
- raw `$35` exact victim subtype `$1A` displays its first recovered finite pose `$16`, advances after the exact 17-tick inclusive phase to pose `$13`, then remains on its recovered `FF00` hold.
- Native fallback art remains active whenever no exact valid state-4 shadow/pose exists.

## Tests
- Test371: exact producer owns/resets the victim state4 clock and does not release FF00.
- Test372: exact victim state4 poses render and finite phases tick even while the attacker owns the native ground-action update; FF00 stays held.
- Directed Tests369–372 PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP85-source.zip`
- SHA256: `def560fa0de82a023ef85ff668f529cf35ace1a210a115c10fd29901f93a994c`

## Fidelity boundaries
- ROM-EXACT: descriptor identity, pose identity, finite relative durations and held-frame identity after a mirrored producer.
- PROVISIONAL: the native ground `tick==12` boundary at which WIP83 currently mirrors unique ground producers.
- REGISTERED BUT NOT IMPLEMENTED: callback release predicates for victim state4 FF00 frames, branch-dependent raw `$23`, and full actor-slot scheduling parity.
- No held Ground Action fallback removed. No `tick==12` replacement. No BIN/CUE. No ROM files.
