# WIP86 — end-to-end exact prone category selection from live victim subtype

## Runtime proof
- No new gameplay heuristic was introduced.
- The existing WIP82 guarded `ground_prone_category()` path is now covered end-to-end through `game_tick()` and the actual universal action table.
- With a valid victim state-4 low subtype `$07`:
  - stamina tier 0 -> exact prone category 6.
  - stamina tier 1/2 -> exact prone category 7.
- For Hulk Hogan (selection 0), physical button1 therefore selects:
  - category 6 -> raw `$0A`.
  - category 7 -> raw `$35`.
- If the ROM state-4 shadow is invalid, the native compatibility fallback remains category 5 -> raw `$08`.

## Tests
- Added Test373: exercises the live `game_tick()` path, not only the table helper.
- Directed Tests369–373 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP86-source.zip`
- SHA256: `bb393c84622e2927fcb05247717ab77248a336665a5260efe790f3645dcfa2d2`

## Fidelity boundaries
- ROM-EXACT: subtype `$07` category rule and action-table results.
- PROVISIONAL: production timing of some state-4 subtypes remains tied to the existing native impact boundary where explicitly documented.
- No held release enabled. No 32-tick held fallback removed. No `tick==12` replacement. No BIN/CUE. No ROM files.
