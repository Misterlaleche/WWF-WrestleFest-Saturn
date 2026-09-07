# Test255 WIP46 — prone terminal-state profiles from ROM-pinned $FE callbacks

## Scope
Continue Ground Actions closure without changing live contact timing. Promote already-audited terminal callback states from Test294 into reusable evidence profiles, while preserving unknowns explicitly.

## ROM-EXACT / evidence-preserving code
- Added `arcade_prone_terminal_state_profile(raw, ...)`.
- The helper exposes only terminal `$FE` sequences that Test294 already pinned byte-for-byte:
  - raw `$0B`: terminal `$13B82` -> actor `+$20=7`;
  - raw `$10`: `$1439E` -> `+$20=7`;
  - raw `$13`: `$14ACC` -> `+$20=7`;
  - raw `$14`: `$14CCA` -> `+$20=7`;
  - raw `$23`: `$15D8C` -> `+$20=5`;
  - raw `$35`: `$176F2` -> `+$20=5`;
  - raw `$46`: `$145CC` -> `+$20=7`;
  - raw `$47`: `$145CC` -> `+$20=7`.
- Raw `$0E` is intentionally absent. Test294 classifies its three `$FF00` held frames, but does not pin an equivalent terminal `$FE` state sequence; no terminal state is inferred from neighbouring callbacks.
- No live state transition was wired into `game.c` in WIP46.

## Tests
- New Test328 PASS: exact terminal profile/address/state for the eight pinned raws; raw `$0E` and unrelated raws reject.
- Directed regression PASS: Tests319, 320, 321, 322, 323, 324, 325, 326, 327, 328.
- `tools/check_host.py` syntax PASS.
- ROM-dependent legacy tests were not rerun because checkpoint source contains no `program.bin`.

## Mandatory timing dependency — unchanged
- `ground_attack_update()` still uses the provisional `tick==12` damage scheduler.
- `ground_attack_visual_duration(...)=32` remains provisional for callback-held descriptors.
- Live `$FF00` release remains disabled until inherited `+$0E/+$42` produces arcade-equivalent `+$37 bit4` timing.
- Knowing an exact terminal state or exact damage value is not sufficient reason to move live timing.

## Fidelity
- **ROM-EXACT:** the eight terminal `$FE` addresses/states listed above, inherited from Test294 byte audits.
- **REGISTERED BUT NOT IMPLEMENTED:** applying those terminal states in live ground callbacks.
- **PROVISIONAL:** old `tick==12`, 32-tick fallback, and any unpinned raw `$0E` terminal behavior.

## Still open
1. Complete pre-hold `+$0E/+$42` lifecycle for every held occurrence.
2. Connect resolver `contact37` to live callback release only after that lifecycle is exact.
3. Remove 32-tick fallback.
4. Replace `tick==12` per action with exact callback producers; raw `$47` remains the first prepared candidate.
5. Continue exact damage/callback side effects for remaining ground raws.
6. Close raw `$0E` terminal behavior with direct ROM evidence.
7. Close PICK UP `$22` handoff.
8. Integrate the separate initial victim `+$65` producer research only after audit.

## Source checkpoint
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP46-source.zip`
- SHA256: `bd9486c8a5186d07245b84bfea1c78999fac74a307071685229b300eb51df3de`

No ROM image included. No BIN/CUE generated.
