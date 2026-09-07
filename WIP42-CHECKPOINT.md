# Test255 WIP42 — raw $47 exact contact-damage/side-effect profiles

## Scope
Start replacing the legacy ground `tick==12` damage path with callback evidence, without changing live timing before the physical contact lifecycle is closed.

## ROM-EXACT / evidence-preserving code
- Added `arcade_prone_exact_contact_damage(raw,&damage68)`.
- Only raw `$47` is currently promoted: Tests156/157/184/186 independently pin both demonstrated resolved target-contact branches and both write victim `+$68=$000A` (10).
- Raw `$46` deliberately remains unpromoted even though it shares callback `$14458`; separate reachability/branch proof is required before claiming its target-contact damage.
- Added `arcade_prone_raw47_contact_profile(branch,...)` for the two already pinned raw `$47` resolved-contact branches:
  - branch A: victim `+$20=4`, `+$64 -= 2`, `+$C7 += 1`, `+$68=10`; actor `+$60 bit7`; effects `$2B/$32`.
  - branch B: victim `+$20=5`, `+$60=$77`, `+$C7 += 1`, `+$68=10`; actor `+$1B=$0C`.
- The branch selector is intentionally not inferred or implemented.

## Tests
- Test322 PASS: exact contact damage is exposed only for demonstrated raw `$47`, value 10; `$46` and other raws remain unknown.
- Test323 PASS: branches A/B preserve their separately pinned actor/victim side effects.
- Regression PASS: Test314, Test315, Test319, Test320, Test321.

## Runtime status
- No live damage scheduling changed in WIP42.
- Legacy `tick==12` remains **PROVISIONAL** in `ground_attack_update()`.
- Raw `$47` callback damage/effects are now represented as **ROM-EXACT evidence profiles**, ready to replace the legacy path once resolver/contact timing is safe to consume live.
- This prevents silently treating raw actions as `AMOVE_NONE -> default 20` as if that were exact; however the existing provisional runtime behavior is retained until a safe migration is possible.

## Still open
- Exact pre-hold `+$0E/+$42` lifecycle for all held occurrences and live `$FF00` release.
- Exact callback damage/side effects for the remaining ground raws.
- Remove `tick==12` only per-action as exact producers become available.
- PICK UP `$22` victim handoff and initial `+$65` producer remain open.

## Fidelity
- **ROM-EXACT:** raw `$47` demonstrated target-contact damage 10 and the two documented contact branch profiles.
- **REGISTERED BUT NOT IMPLEMENTED:** branch selection/live callback application for raw `$47`.
- **PROVISIONAL:** legacy `tick==12` damage scheduling and 32-tick held fallback.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP42-source.zip`
- SHA256: `ac1ceab5e9f3a378f727f75c0ea8ea5899bd3112052de146e4962bf86b1e6bd1`

No ROM image included. No BIN/CUE generated.
