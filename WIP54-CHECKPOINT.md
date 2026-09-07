# WIP54 — raw $10 target provenance integration

Status: research/model checkpoint only. No BIN/CUE. No ROM files.

## ROM-EXACT additions
- Integrated the externally researched and ROM-pinned Perfect Plex aftermath chain relevant to raw `$10` target selection:
  - callback `$15120` writes linked target `+$64=$0021` at `$15154`, hence target `+$65=$21`;
  - state-4 subtype callback `$1BD6C` writes target `+$65=$09` at `$1BDC8` exactly when target `+$71==$02`;
  - `+$70/$71==$0002/$02` is the critical stamina tier from the already implemented ROM `$24EC2` classifier.
- Added `arcade_prone_raw10_target_provenance()` and `arcade_prone_raw10_target_from_perfect_plex()`.
- This closes one concrete ROM-EXACT upstream route into raw `$10`'s category-7 target family.

## Safety boundary
- The attacker's `+$32 bit2` remains independent and unproven on raw `$10` entry.
- Raw `$10` live `$FF00` release remains blocked solely by `ARCADE_PRONE_RELEASE_BLOCK_ACTOR32`.
- No `release=true`, no removal of the 32-tick fallback, and no change to provisional `tick==12`.

## Tests
- Directed Tests329–338 PASS.
- New Test338 pins Perfect Plex -> target `$21` -> critical-tier `$09` -> raw `$10` category-7 reachability.

## Source
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP54-source.zip`
- SHA256: `4bde2065a97a63df86ea93ff6cf75e0a71da302a109fb207e0dac1d5c121fd06`

## Next
Return to the remaining blocker: prove the attacker's incoming `+$32 bit2` on the selection/caller path for raw `$10`. If closed, raw `$10` becomes eligible for isolated physical-release activation.
