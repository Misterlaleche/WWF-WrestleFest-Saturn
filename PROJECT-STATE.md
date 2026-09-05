# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test52 / V0.067**
- Host regression: **50 host binaries completed; all PASS**
- Targeted ASan+UBSan: **ringside throw geometry + live object use PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test52-V0067-source.zip`
- Snapshot SHA256: `2cd2bdfaf9273766fb56d6852214a9f90259ac990872681a2b88c5ec4d6911ac`

### Major state reached by Test52

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects have original pickup/use wrestler animations and underflow-inclusive clocks: pickup 18 ticks, use 27 ticks.
- CPU weapon seeking preserves the ROM nearest-object and `+$B0` opponent-distance gate.
- Test51 decoded the `$1D032` held-weapon mid-range branch: `|dy| < $0C`, `$68 <= |dx| < $90`, weighted table `$1D066 = {$32,$32}`; demonstrated action `$1F` decision is exactly 50/50.
- **Test52:** corrected callback `$15A20` release geometry. The object enters state 2, copies wrestler Y, receives exactly `+$60/-$60` X according to facing, and `+$40` height. Native ringside objects now retain a Z/height component. This replaces the old provisional 24-pixel release offset.

### Provisional

- The native deterministic RNG feed used by the Saturn CPU weapon branch is provisional; the ROM-proven bounds and 50/50 weighting are not provisional.
- Exact initial object-coordinate conversion into the Saturn world model remains provisional.

### Registered but not yet exact/complete

- Subsequent object state-2 trajectory/physics, collision/damage, rendering and reavailability lifecycle.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
