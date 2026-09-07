# WIP82 — safe live consumption of exact victim prone category

- Added conservative runtime lifetime for `state4_64_valid`:
  - mirrored +$64/+65 remains eligible only across native prone carrier states `ST_DOWN` / `ST_RECOVER`;
  - any transition to another native state invalidates the shadow to prevent stale subtype reuse on a later knockdown.
- This is explicitly a native safety guard, not an assertion that native state enums numerically equal ROM main state 4.
- Added `ground_prone_category()`:
  - when a current valid state4 shadow is in exact low subtype 06/07/08/09, selection consumes ROM-EXACT victim category 5/6/7 via WIP81 helper;
  - otherwise it retains category-5 fallback rather than guessing category 6/7.
- Both button1 and both-button prone action lookup now use this guarded victim-side category.
- Button2/pin remains valid across recovered prone categories because Test74 already proves raw $48 for categories 5/6/7.
- Test368 PASS; `game.c` syntax PASS; WIP81 Tests366/367 remain PASS.
- Attacker-side prone classifier gates remain native/provisional; this checkpoint does not claim the complete $E0B8/$E110 predicate is live.
- Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP82-source.zip`
- SHA256: `03612d200a282124a3568b7510f8d8cc1e74e165b0d47d78fc73f41f3810b1cc`
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
