# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test51 / V0.066**
- Host regression: **49 host binaries completed; 68 PASS assertions/messages emitted**
- Targeted ASan+UBSan: **CPU weapon direct-use + mid-range weighted-use PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test51-V0066-source.zip`
- Snapshot SHA256: `06f95c2faa2b1a28c6c8013df8cd361f1d1bd9912e99be63e4a6482b9b4a5ee6`

### Major state reached by Test51

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects have original pickup/use wrestler animations and underflow-inclusive clocks: pickup 18 ticks, use 27 ticks.
- CPU weapon seeking preserves the ROM nearest-object and `+$B0` opponent-distance gate.
- **Test51:** decoded and implemented the second held-weapon CPU branch at `$1D032`: `|dy| < $0C`, `$68 <= |dx| < $90`, weighted selector table `$1D066 = {$32,$32}`. Result index 0 selects action `$1F`, so the demonstrated decision is exactly 50/50. Actions 30/31 share the original weapon-use descriptor.

### Provisional

- The native deterministic RNG feed used by the Saturn CPU weapon branch is provisional; the ROM-proven bounds and 50/50 weighting are not provisional.
- Exact initial object-coordinate conversion into the Saturn world model remains provisional.

### Registered but not yet exact/complete

- Exact object state-2 physics, collision/damage, rendering and reavailability lifecycle.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
