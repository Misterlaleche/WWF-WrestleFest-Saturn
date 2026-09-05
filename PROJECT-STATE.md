# WWF WrestleFest Saturn — private development repository

Private working repository for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- Test33 / V0.048
- Local Git commit: `918ffb0`
- Host regression suites: 52 PASS
- Source checkpoint persisted in ChatGPT Library: `/WWF-Saturn/WWF-Saturn-Test33-V0048-source.zip`
- SHA256: `a2cbe7cb03e26ad2cae5e72b7809cc42c734592c42d200002e366652d2a5bca0`

Test33 closes deterministic CPU prone-action selection: non-zero prone opponents can receive wrestler-specific ground drops or submissions; zero stamina remains a legal pin priority.

## Long development campaign

Continue concrete, non-empty checkpoints through Test200. Priorities: exact stamina/state behavior, CPU and partner AI, wrestler-specific moves, TAG/double-team systems, ring-out/count-out and ringside behavior, Royal Rumble eliminations, stages, records, continue/buy-in/regain-power, game over, presentation and remaining arcade systems.

Do not claim arcade-exact behavior without ROM/manual evidence. Mark uncertain behavior provisional or registered-but-not-implemented.

The next hardware BIN/CUE build remains deferred until the long Test21→Test200 integration campaign is mature enough for an alpha-class Saturn build.
