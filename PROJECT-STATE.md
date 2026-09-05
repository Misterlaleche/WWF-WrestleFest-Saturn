# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test60 / V0.075**
- Host regression: **78 PASS result lines** from the integrated host harness
- Targeted ASan+UBSan: **state-2 probe orchestration and native state-4 object lifecycle PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test60-V0075-source.zip`
- Snapshot SHA256: `a91ceea8c3a4a366a93a158c524345bff471f7003446cb6ee2810a3fbefdd0da`

### Recovery note

The old persistent Test52 ZIP was incomplete and could not compile by itself. Test55 deterministically recovered that work from the intact Test51 parent. Test60 descends from that recovered line and is now the only current recoverable source of truth; do not resume from the incomplete Test52 ZIP or older checkpoints.

### Major state reached through Test60

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
- Test56 models ROM `$FA14` reset: both autonomous ringside objects become available state `3`, unheld, with exact raw `+$0E=$0100`; normalized Saturn X/Y placement remains provisional.
- Test57 decoded object dispatcher `$FDEA` and state-2 handler `$FE82`; demonstrated tail `$FEE6/$FEEE/$FEF6` applies `+$38/+$3A/+$3C` corrections to XYZ and collision flag bit 4 advances state 2 to state 4.
- Test58 decoded state-4 handler `$FF54`: fresh state-4 entry returns to available state 3 on the 49th handler invocation.
- **Test59 corrects the state-2 model:** `+$38/+$3A/+$3C` are not demonstrated persistent launch velocities. `$FE82` calls shared collision/boundary resolver `$280DC`, which clears those fields before each probe and writes correction deltas only on contact. `$FE82` probes with `+$3E=$20`, retries with `-$20` only after a miss, and skips XYZ integration entirely after two misses. A host helper now preserves that exact two-probe orchestration without inventing collision geometry.
- **Test60 wires the exact `$FF54` state-4 clock into the native autonomous-object runtime.** Injected state-4 objects remain unavailable for the demonstrated duration and become state 3 on handler call 49; raw Z `$0100` and native bookkeeping are restored for reuse.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds and 50/50 weighting are exact.
- Exact initial object-coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55: finisher identity, descriptors, graphics and readiness contexts are recovered arcade data, but the policy to take every ready critical-target opportunity is native Saturn policy pending exact 68000 AI recovery.

### Registered but not yet exact/complete

- Full `$280DC` world-boundary/collision geometry for ringside objects and resulting damage/rendering semantics. The old “initial state-2 velocity producer” item is closed by Test59 because `$FE82` regenerates correction fields through `$280DC` rather than consuming demonstrated persistent launch velocities.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next priority is either to decode enough of `$280DC` to reproduce the object boundary/collision path safely, or, if that cannot be bounded rigorously, move to another listed gameplay priority without inventing values.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
