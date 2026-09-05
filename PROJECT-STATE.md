# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test58 / V0.073**
- Host regression: **76 PASS result lines** from the integrated host harness
- Targeted ASan+UBSan: **CPU finisher routes, ringside throw geometry, available-object reset, state-2 integrator and state-4 reavailability PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test58-V0073-source.zip`
- Snapshot SHA256: `3a412eed3d54009cce11c2a5bf057073e9e127462983291f12775226fc55c328`

### Recovery note

The old persistent Test52 ZIP was incomplete and could not compile by itself. Test55 deterministically recovered that work from the intact Test51 parent. Test58 descends from that recovered line and is now the only current recoverable source of truth; do not resume from the incomplete Test52 ZIP.

### Major state reached through Test58

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects have original pickup/use wrestler animations and underflow-inclusive clocks: pickup 18 ticks, use 27 ticks.
- CPU weapon seeking preserves the ROM nearest-object and `+$B0` opponent-distance gate.
- Test51 decoded the `$1D032` held-weapon mid-range branch: `|dy| < $0C`, `$68 <= |dx| < $90`, weighted table `$1D066 = {$32,$32}`; demonstrated action `$1F` decision is exactly 50/50.
- Test52 corrected callback `$15A20` release geometry: state 2, wrestler Y copied, exact `+$60/-$60` X by facing, and `+$40` object height.
- Test53 validates that CPU grapple rota can naturally enter mapped trademark finishers and Saturn preloads the finisher overlay for the actual pair attacker.
- Test54 enables CPU Hogan Legdrop / Earthquake Vertical Splash against recovered prone contexts; opportunity priority remains provisional.
- Test55 enables CPU Boss Man interception and Slaughter/DiBiase behind trademark contexts; opportunity priority remains provisional.
- **Test56:** ROM `$FA14` reset is modeled: both autonomous ringside objects become available state `3`, unheld, with exact raw `+$0E=$0100` height. Normalized Saturn X/Y placement remains provisional.
- **Test57:** decoded the object dispatcher at `$FDEA` and state-2 handler `$FE82`. Its exact integration tail adds `+$38/+$3A/+$3C` to object X/Y/Z (`+$06/+$0A/+$0E`); collision flag bit 4 at `+$37` advances state 2 to state 4.
- **Test58:** decoded state-4 handler `$FF54`: first entry starts phase 1 with timer `$10`; each expiry reloads `$10` and increments the phase; phase 4 writes state `3` at `$FFC0`. From fresh state-4 entry, reavailability occurs on the 49th handler invocation.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds and 50/50 weighting are exact.
- Exact initial object-coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55: finisher identity, descriptors, graphics and readiness contexts are recovered arcade data, but the policy to take every ready critical-target opportunity is native Saturn policy pending exact 68000 AI recovery.

### Registered but not yet exact/complete

- Producer of initial state-2 object velocity fields `+$38/+$3A/+$3C`; Test57 deliberately does not invent launch velocities.
- Full collision/damage semantics around state-2 object impact and rendering details.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
