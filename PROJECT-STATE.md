# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test50 / V0.065**
- Local development commit: `a66eb61`
- Host regression: **68 suites PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test50-V0065-source.zip`
- Snapshot SHA256: `bc5fcee207cdb022deb0e21c11d94fa2bafa864e9b442ac4404edbaad411205d`

### Major state reached by Test50

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects now have original pickup/use wrestler animations and underflow-inclusive clocks: pickup 18 ticks, use 27 ticks.
- CPU weapon seeking preserves the ROM nearest-object and `+$B0` opponent-distance gate. Held-object direct use now reproduces the strict `$68` horizontal / `$0C` vertical branch and actions 30/31.

### Registered but not yet exact/complete

- Weighted CPU weapon-use branch for `$68..$8F`.
- Exact object state-2 physics, collision/damage, rendering and reavailability lifecycle.
- Exact initial object-coordinate conversion into the Saturn world model.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
