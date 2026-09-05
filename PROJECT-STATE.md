# WWF WrestleFest Saturn — private development repository

Private working repository for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- Test36 / V0.051
- Normal host regression: 56 suites PASS
- Focused Test36 ASan/UBSan: PASS
- Executable ROM stamina evidence audit: PASS
- Source checkpoint persisted in ChatGPT Library: `/WWF-Saturn/WWF-Saturn-Test36-V0051-source.zip`
- SHA256: `80fda3bcd1c47f5443a57c9ce0a4d5a4a7c0604321c8d1e550b037d4e7fccc0e`
- Full all-suite ASan/UBSan rerun: not claimed complete in this checkpoint.

Test36 adds ROM-derived CPU ringside-object seeking from program ROM `$1C228-$1C2CA` and `$1C740-$1C7E2`. The native runtime now preserves the exact opponent X-distance classes (`<0x50`, `<0xB0`, far), selects the nearer of two active object candidates by X, and diverts from the opponent only when the selected object is strictly closer in X than the opponent, matching the comparison against original actor `+$B0` at `$1C796`. Live ringside CPU movement uses that diversion target and suppresses opponent attack firing while seeking the object.

Object pickup/use callbacks are deliberately not invented. They remain registered-but-not-implemented until their ROM callbacks are traced.

The prior exact raw stamina-recovery exclusion predicate remains validated: states `0B`, `0C`, linked `FF -> 0C`, and state `05` pose `4A`. Semantic mapping of every original raw state to native `fighter_state_t` remains registered-but-not-implemented where evidence is missing.

## Long development campaign

Continue concrete, non-empty checkpoints through Test200. Priorities: finish semantic stamina recovery wiring; replace remaining provisional stamina-tier combat decisions with ROM-derived behavior; expand wrestler-specific actions/finishers; improve CPU and apron-partner AI; complete authentic TAG/double-team systems, ring-out/count-out/ringside behavior and objects, Royal Rumble eliminations, stages, records, continue/buy-in/regain-power, game over, attract/presentation and remaining arcade systems.

Do not claim arcade-exact behavior without ROM/manual evidence. Mark uncertain behavior provisional or registered-but-not-implemented.

The next hardware BIN/CUE build remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build or a Saturn-only blocker requires it.
