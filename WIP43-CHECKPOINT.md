# Test255 WIP43 — raw $47 fallback profile + ground timing dependency lock

## Scope
Preserve the user's explicit ground-timing observation and extend the raw-$47 callback evidence without enabling unsafe live timing.

## Mandatory dependency — do not lose this later
- The legacy `tick==12` path must **not** be removed merely because a ROM-EXACT damage value is known.
- Raw `$47` now has exact callback-side damage/effect profiles, but live application must wait until `+$37 bit4` is driven by the correct inherited `+$0E/+$42` lifecycle.
- Replacing `tick==12` before that physical contact clock is safe could move damage to the wrong frame, release too early, or hang a callback-held action.
- Therefore raw `$47` is ready as an exact producer profile but remains **REGISTERED BUT NOT IMPLEMENTED** for live damage scheduling.

## ROM-EXACT / evidence-preserving code
- WIP42 target-contact branches remain separate:
  - branch A: victim state 4, `+$64 -= 2`, `+$C7 += 1`, `+$68=10`; attacker `+$60 bit7`; effects `$2B/$32`.
  - branch B: victim state 5, `+$60=$77`, `+$C7 += 1`, `+$68=10`; attacker `+$1B=$0C`.
- Added `arcade_prone_raw47_fallback_profile()` from the already byte-pinned Test158 callback path at `$14606`.
- Raw `$47` fallback is actor-local, not a third victim-contact branch:
  - actor `+$20=4`
  - actor `+$64=5`
  - actor `+$68=3`
  - effect `$32`
  - global `$1C15D2=$0F1A`
  - actor `+$26 bit7`
  - facing `+$2E bit7` flipped
- Branch selection between contact A / contact B / fallback is still not inferred.

## Tests
- Test321 PASS: native ground entry preserves inherited `+$42`.
- Test319 PASS: all mapped `$FF00` gates consume resolver contact37; only four proven gates mutate `+$42`.
- Test320 PASS: raw `$0E` tracks three distinct held phases from descriptor frame identity.
- Test322 PASS: exact victim-contact damage is promoted only for demonstrated raw `$47`, value 10.
- Test323 PASS: raw `$47` contact branches A/B remain separately represented.
- Test324 PASS: raw `$47` fallback remains distinct and preserves its actor-local state/damage/effect writes.
- No `program.bin`, `gfx2.bin` or ROM ZIP is present in the checkpoint source.

## Runtime status
- `ground_attack_update()` still contains the old `tick==12` damage path: **PROVISIONAL**.
- `ground_attack_visual_duration(...)=32` still remains for callback-held descriptors: **PROVISIONAL**.
- Live `$FF00` `release=true` remains disabled: **REGISTERED BUT NOT IMPLEMENTED** pending exact pre-hold raw-Z/probe lifecycle.
- Raw `$47` branch selector/live side-effect application remains **REGISTERED BUT NOT IMPLEMENTED**.

## Still open in ground-actions closure
1. Complete pre-hold `+$0E/+$42` lifecycle for all held occurrences.
2. Connect resolver contact37 -> exact callback release only when inherited state is safe.
3. Remove 32-tick fallback.
4. Replace `tick==12` per action with exact callback producers; raw `$47` is the first prepared profile.
5. Continue exact callback damage/side effects for remaining ground raws.
6. Close PICK UP `$22` victim handoff.
7. Complete initial victim `+$65` subtype production across native knockdown paths.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP43-source.zip`
- SHA256: `f1a4e3d65ba3a454a1f1dc52e082e356f4654059594b68601a46ee801513b8ba`
- Checkpoint commit: `0c35f692f5e5981dd0cfeed3d9df9d58f14767af`

No BIN/CUE generated.
