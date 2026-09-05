# WWF WrestleFest Saturn — private development repository

Private working repository for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- Test35 / V0.050
- Normal host regression: 35 suites PASS
- Executable ROM stamina evidence audit: PASS
- Source checkpoint persisted in ChatGPT Library: `/WWF-Saturn/WWF-Saturn-Test35-V0050-source.zip`
- SHA256: `1deb20ac7b53c19b8c372340dd691a8d453cf9c26646f4a9c6222cdea473cb9d`
- Full ASan/UBSan rerun for Test35: INCOMPLETE because the session execution window expired before the full suite completed; do not count it as PASS or FAIL.

Test34 implements the exact raw 68000 stamina-regeneration exclusion predicate recovered from program ROM `$8ECC-$8F44`: states `0B`, `0C`, linked `FF -> 0C`, and state `05` pose `4A`. Semantic mapping of every raw state byte to native `fighter_state_t` remains registered-but-not-implemented where evidence is still missing.

Test35 adds an executable `program.bin` evidence audit anchoring the complete `$8ECC` regeneration routine, human stamina table `$10830`, CPU progression table `$10848`, and tier routine `$24EC2`. This prevents later native rules from silently drifting away from recovered ROM behavior.

## Long development campaign

Continue concrete, non-empty checkpoints through Test200. Priorities: finish semantic stamina recovery wiring; replace remaining provisional stamina-tier combat decisions with ROM-derived behavior; expand wrestler-specific actions/finishers; improve CPU and apron-partner AI; complete authentic TAG/double-team systems, ring-out/count-out/ringside behavior, Royal Rumble eliminations, stages, records, continue/buy-in/regain-power, game over, attract/presentation and remaining arcade systems.

Do not claim arcade-exact behavior without ROM/manual evidence. Mark uncertain behavior provisional or registered-but-not-implemented.

The next hardware BIN/CUE build remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build or a Saturn-only blocker requires it.
