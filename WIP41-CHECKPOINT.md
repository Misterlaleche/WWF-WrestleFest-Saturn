# Test255 WIP41 — prone held-phase runtime identity and inherited probe42

## Scope
Remove two remaining native-state inventions before any live `$FF00` release bridge is enabled.

## ROM-EXACT / evidence-preserving integration
- Added `arcade_prone_hold_phase_for_frame(raw, frame, &phase)` using the ROM-audited WIP37 held-profile metadata.
- Runtime `prone_hold_phase` is now synchronized from the exact descriptor frame whenever the action clock is on a `$FF00` frame.
- Raw `$0E` is therefore represented as three distinct held occurrences (frames 3/4/5 -> phases 0/1/2), rather than a single generic held state.
- The eight other reachable prone raws map to their single exact held occurrence.
- Removed the native `probe42_shadow=0` write from `ground_attack_begin_raw()`. WIP31/WIP35/WIP39 establish `+$42` as callback-temporal state which is explicitly cleared only on demonstrated callback paths; no ROM-backed generic ground-entry clear has been demonstrated.
- `raw_z_shadow` rebases remain limited to the WIP35-proven raw `$0E/$10/$13` target-Z paths.

## Tests
- Test314 PASS: `$FF00` clock advances only on explicit release.
- Test315 PASS: base resolver consumes persistent actor32/raw-Z/probe42 state.
- Test319 PASS: all mapped held callbacks consume contact37; only four immediate-probe gates mutate `+$42`.
- Test320 PASS: exact descriptor-frame -> held-phase mapping, including raw `$0E` phases 0/1/2.
- Test321 PASS: raw-ground entry no longer invents a `+$42` reset.
- `game.c` host compilation PASS (warnings are pre-existing style/sign warnings; no new compile error).

## Still not promoted
- Live `release=true` remains disabled in `ground_attack_update()`.
- The 32-tick fallback remains **PROVISIONAL** until the pre-hold raw-Z/probe lifecycle is sufficient to guarantee the same resolver input as arcade for every held occurrence.
- Immediate-probe callbacks are modeled, but enabling them live before inherited Z is closed can still cause premature release or a permanent hold.
- Damage/callback side effects, old `tick==12`, PICK UP `$22` handoff and initial `+$65` producer remain open.

## Fidelity
- **ROM-EXACT:** held descriptor identity/ordinal metadata; explicit callback clears already pinned in WIP39.
- **REGISTERED BUT NOT IMPLEMENTED:** live physical-release bridge.
- **PROVISIONAL:** 32-tick fallback and native damage timing.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP41-source.zip`
- SHA256: `ac0c6a669b8008b126296078c94cebfcd58879c4850f37a12ec37b95827c1474`
- Checkpoint commit: `70dd3afcb683cf36d9db93bf4040f335b5df7b6f`

No ROM image included. No BIN/CUE generated.
