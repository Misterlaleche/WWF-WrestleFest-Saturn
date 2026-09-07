# Test255 WIP45 — raw $47 extended callback lifecycle profiles

## Scope
Extend the already ROM-pinned raw `$47` callback evidence beyond damage A/B/fallback so eventual removal of the provisional `tick==12` path has the surrounding exact state transitions available as well.

## ROM-EXACT / evidence-preserving code
- WIP42/43 contact branches remain intact.
- Test185 side effect is now represented in raw `$47` branch A: the callback increments victim `+$E0` only for neighbouring raw `$46`; raw `$47` therefore takes the alternate arm and increments victim `+$E2` by 1.
- Added `arcade_prone_raw47_phase0_actor01()` from Test181:
  - callback phase `+$25=0`
  - actor `+$22=0`
  - writes actor `+$01=2`.
- Added `arcade_prone_raw47_terminal_profile()` from Test187:
  - phase `$FE` exits to actor state `+$20=7`
  - writes actor `+$04=$68`
  - sets actor `+$26 bit7`
  - calls `$10BD0` with `D0=$18`, `D1=-1`, `D2=0`.
- Test188 remains an important exclusion: the extra `$FFD0` fallback impulse belongs to neighbouring raw `$46`, not `$47`; it is not applied to the raw-$47 fallback profile.

## Tests
- Test319 PASS
- Test320 PASS
- Test321 PASS
- Test322 PASS
- Test323 PASS
- Test324 PASS
- Test325 PASS
- Test326 PASS: raw `$47` contact branch A exposes exact victim `+$E2 += 1`; branch B does not inherit it.
- Test327 PASS: raw `$47` phase-0 `+$01=2` write and phase-`$FE` terminal transition/call are preserved.
- `tools/check_host.py` registration for Tests325-327 was corrected so these tests are declared before the runner loop rather than after it.

## Mandatory dependency retained
- The old `tick==12` damage scheduler is still **PROVISIONAL** and remains live.
- Raw `$47` must not be switched to callback-timed live damage until inherited `+$0E/+$42` drives the same `+$37 bit4` physical contact timing as arcade.
- Live `$FF00 release=true` and removal of the 32-tick held fallback remain blocked on that same lifecycle.

## Fidelity
- **ROM-EXACT:** raw `$47` contact A/B/fallback evidence already established; branch-A victim `+$E2` counter side effect; phase-0 actor `+$01=2` gate/write; phase-`$FE` terminal state/call profile; explicit exclusion of raw-$46-only `$FFD0` impulse.
- **REGISTERED BUT NOT IMPLEMENTED:** live scheduling/application of the complete raw `$47` callback profile.
- **PROVISIONAL:** legacy `tick==12`, callback-held 32-tick fallback, and unresolved initial victim `+$65` production.

## Ground Actions status after WIP45
1. Pre-hold `+$0E/+$42` lifecycle remains the principal blocker.
2. Once exact, connect `contact37 -> release=true` and remove 32-tick fallback.
3. Raw `$47` is now substantially prepared for exact callback-side replacement of legacy damage timing, including entry/contact/fallback/terminal side effects.
4. Remaining ground raws still need equivalent exact damage/side-effect closure where evidence exists.
5. PICK UP `$22` victim handoff remains open.
6. Initial victim `+$65` producer remains open; WIP44 only closed downstream retype `8/9 -> 6/7` for raw `$0A/$0B/$10`.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP45-source.zip`
- SHA256: `7bc0b022ae408e4093803d631463078344b3a402124de6c5514b2eaba05f330a`

No ROM image included. No BIN/CUE generated.
