# Test255 WIP24 checkpoint

Status: source-only WIP. No BIN/CUE generated.
Canonical validated baseline remains Test254 / V0.269.

## New in WIP24 — referee visual audit
- Reconstructed exact arcade `program.bin` and `gfx2.bin` locally from the user-supplied ROM set only for verification; neither is stored in this checkpoint.
- Confirmed arcade gfx set `$0C` is the referee animation bank.
- Built `cd/REF12.PAK` with every valid ROM-exact referee pose currently recovered from gfx set `$0C`.
- `REF12.PAK`: 19 poses, 255 unique tiles, 328 records, 22,855 bytes.
- `REF12.PAK` SHA256: `fe9b0ad9f82d1d7a680fffc26a02f84288f45b7144db616c43a0a680167edfdf`.
- Added `assets/REF12.PAL`, `referee_assets.h`, builder/audit tooling and `docs/VISUAL-ARCADE-PERFECT-AUDIT.md`.
- Test282 proves all 19 packaged poses are ROM-exact and that the 10 referee poses already used by Saturn map byte-identically to arcade gfx-set `$0C`.

## Current Saturn referee -> ROM-exact pose mapping
- idle -> 1
- walk_1 -> 2
- walk_2 -> 3
- walk_3 -> 0
- walk_4 -> 1
- kneel -> 4
- arm_up -> 5
- slap -> 6
- signal_start -> 15
- signal -> 16

## Remaining exact referee art
Arcade poses `7,8,9,10,11,12,13,14,17,19` are recovered and packaged but their exact runtime producers/timings are not yet proven.

Labels:
- **ROM-EXACT**: all 19 REF12 pose compositions and the ten mappings above.
- **PROVISIONAL**: existing Saturn referee movement/follow/timing logic until recovered from ROM.
- **REGISTERED BUT NOT IMPLEMENTED**: exact runtime producers/timings for poses 7-14,17,19.

## Presenter status
- Existing 7 presenter images remain original arcade pixels.
- Exit choreography/timing remains native/provisional.
- Exact rear-view/back-facing presenter sequence remains REGISTERED BUT NOT IMPLEMENTED and is the next visual reverse-engineering target.

## Validation
- Test282 passes against reconstructed ROM/GFX hashes:
  - program SHA256 `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`
  - gfx2 SHA256 `a3b5e1fedd6c41cd398ba0cb437cddb2f50b3ae0a0dc178a10b70a7ed5d010ba`
- ROM/GFX files removed before source packaging.

## Library
- `/WWF-Saturn/WWF-Saturn-Test255-WIP24-source.zip`
- SHA256 `a6de4d03768f4e22bf93f927ab6ef0cc78529b96410e44229dc1b0e3d5c695ab`
