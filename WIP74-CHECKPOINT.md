# WIP74 — width-aware runtime state4 victim shadow

- Added persistent `state4_64_shadow`, validity and initialization-phase fields to `fighter_t`.
- Added `state4_apply_rom_initial_producer()` which mirrors the WIP73 registry with exact byte-vs-word write semantics.
- Wired only two native events with one-to-one ROM-EXACT semantic identity:
  - Boss Man Slam impact -> `$17100`, victim word `$002A`.
  - Perfect Plex impact -> `$15154`, victim word `$0021`.
- No generic finisher subtype inference. Other finishers leave the shadow invalid until their producer is mapped.
- Test359 PASS; Test358 PASS; `game.c` host syntax check PASS.
- Shadow is not yet used to choose prone actions; state4 callback transitions must be live first.
- Source ZIP SHA256: `aa9d021caaa01f29d3750100fbe5f8b932fb5456180f38035da93baef998cce7`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
