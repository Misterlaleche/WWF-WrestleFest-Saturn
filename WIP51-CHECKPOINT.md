# Test255 WIP51 — hard live-release guard

## Scope
Prevent accidental activation of any callback-held `$FF00` phase before its exact inherited inputs are closed.

## Evidence-preserving integration
- Added `arcade_prone_hold_live_release_ready(raw,phase)`.
- The helper returns true only when the exact blocker mask is zero.
- With current audited evidence, no held prone phase is live-ready.
- Raw `$10` remains the closest candidate: its only blocker is actor `+$32 bit2` lifecycle.
- No runtime release timing changed; `ground_attack_update()` still does not pass live release into the action clock.

## Tests
- Test335 PASS: all 11 held occurrences remain guarded; raw `$10` is blocked solely by actor32.
- Regression PASS: Tests329–334.
- `check_host.py` syntax PASS.

## Fidelity
- ROM-EXACT: blocker composition from previously pinned Z/probe evidence.
- REGISTERED BUT NOT IMPLEMENTED: live `$FF00` release.
- PROVISIONAL: 32-tick fallback and `tick==12` damage scheduling.

## Next
Close actor `+$32 bit2` value/lifecycle at raw `$10` entry. Do not infer it from unrelated state-2 producers.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP51-source.zip`
- SHA256: `7f8646f5f606d44e934e3fc23b4a4020980545b6ec52c05528cfaaa5ab7d9881`

No ROM image included. No BIN/CUE generated.
