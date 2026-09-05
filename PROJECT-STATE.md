# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test55 / V0.070**
- Host regression: **73 PASS result lines** from the integrated host harness
- Targeted ASan+UBSan: **CPU grapple finisher, CPU prone finisher, CPU contextual finishers, ringside throw geometry PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test55-V0070-source.zip`
- Snapshot SHA256: `f36eea7aaae6c9ebf5bdb4c44b6f6890cbf27908b1a244df98afb64df822c243`

### Recovery note

The previously registered `/WWF-Saturn/WWF-Saturn-Test52-V0067-source.zip` was found to be incomplete: it omitted 11 required parent files including `arcade_stamina.*`, `arcade_moves.*`, `arcade_ringside.*`, campaign sources and `program.bin`, so its own `check_host.py` could not compile. Test52 was deterministically recovered by applying the surviving Test52 changed/new files onto its intact Test51 parent and restoring the ROM-documented `ARCADE_OBJECT_USE_RAW_Z = 0x40` symbol required by the Test52 tests. The recovered Test52 passed the full host regression before subsequent work. Test55 is now the recoverable source of truth; do not resume from the incomplete Test52 ZIP.

### Major state reached through Test55

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects have original pickup/use wrestler animations and underflow-inclusive clocks: pickup 18 ticks, use 27 ticks.
- CPU weapon seeking preserves the ROM nearest-object and `+$B0` opponent-distance gate.
- Test51 decoded the `$1D032` held-weapon mid-range branch: `|dy| < $0C`, `$68 <= |dx| < $90`, weighted table `$1D066 = {$32,$32}`; demonstrated action `$1F` decision is exactly 50/50.
- Test52 corrected callback `$15A20` release geometry: state 2, wrestler Y copied, exact `+$60/-$60` X by facing, and `+$40` object height.
- Test53 validates that CPU grapple rota can naturally enter mapped trademark finishers and Saturn preloads the finisher overlay for the actual pair attacker.
- Test54 generalizes solo finisher progression to CPU slot 1 and enables Hogan Legdrop / Earthquake Vertical Splash against a recovered ready prone-diagonal context.
- Test55 enables CPU Boss Man interception and Slaughter/DiBiase behind trademark contexts through the recovered finisher state machine.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds and 50/50 weighting are exact.
- Exact initial object-coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55: the finisher identity, descriptors, graphics and readiness contexts are recovered arcade data, but the rule to take every ready critical-target opportunity is native Saturn policy pending exact 68000 AI recovery.

### Registered but not yet exact/complete

- Subsequent ringside object state-2 trajectory/physics, collision/damage, rendering and reavailability lifecycle.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
