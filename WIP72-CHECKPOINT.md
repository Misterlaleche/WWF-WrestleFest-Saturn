# WIP72 — exact state-4 prone-family transition core

- Added `arcade_state4_prone_family_step()` for the ROM-EXACT prone family `$05/$2A/$06/$07/$21`.
- Preserves the critical `+$64 bit7` latch semantics: subtype `$05` first entry may latch critical tier without transitioning; only a later initialized current-turn contact can select `$08/$09`.
- Encodes `$2A->$08`, `$06/$07->$08/$09` on `+$25==$FE`, and `$21->$08/$09` by `+$71==$02` after contact.
- No live ground-release change, no `tick==12` change, no held fallback removal.
- Test357 added; Tests354, Test355 and Test357 pass.
- Source ZIP SHA256: `5cdf6f0be7fa9b4eb9ef5d7e76d09b9c4ad16607812e2743989344d218981b1f`.
- No ROMs and no BIN/CUE.
