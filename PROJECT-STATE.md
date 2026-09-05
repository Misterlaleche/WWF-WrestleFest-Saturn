# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test61 / V0.076**
- Host regression: **78 PASS result lines** from the integrated host harness
- Targeted ASan+UBSan: **new `$280DC/$28128` object-dispatch helper PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test61-V0076-source.zip`
- Snapshot SHA256: `2953b7ccd38f1d84e8a3e46f94ac237a4ffa8ad8789a70f0a691a690158dc915`

### Recovery note

The old persistent Test52 ZIP was incomplete and could not compile by itself. Test55 deterministically recovered that work from the intact Test51 parent. Test61 descends from that recovered line and is now the only current recoverable source of truth; do not resume from the incomplete Test52 ZIP or older checkpoints.

### Major state reached through Test61

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
- Test59 corrected the state-2 model: `+$38/+$3A/+$3C` are resolver-generated contact corrections rather than demonstrated persistent launch velocities; `$FE82` probes `+$3E=$20`, retries with `-$20` after a miss, and skips XYZ integration after two misses.
- Test60 wires the exact `$FF54` state-4 clock into the native autonomous-object runtime.
- **Test61 decodes the bounded `$280DC -> $28128 -> $28154` dispatch used by ringside objects.** `$28128` selects by `($1C007E & 7) * 8 + (+$32 & 4)`; both `$FFD2` objects have `+$32=$0004`. For safely bounded global modes 0..6 the exact object handlers are: `0->$28288`, `1->$2818C/RTS`, `2->$2851E`, `3->RTS`, `4->RTS`, `5->$28288`, `6->$2851E`. Mode 7 remains explicitly unknown because an eighth pointer pair is not safely bounded in the ROM table.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds and 50/50 weighting are exact.
- Exact initial object-coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55: finisher identity, descriptors, graphics and readiness contexts are recovered arcade data, but the policy to take every ready critical-target opportunity is native Saturn policy pending exact 68000 AI recovery.

### Registered but not yet exact/complete

- Geometry inside resolver handlers `$28288` and `$2851E`, the producer/semantic mapping of global `$1C007E`, and resulting object damage/rendering semantics. Test61 intentionally stops at exact bounded dispatch rather than treating following opcodes as an eighth pointer pair.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

The next priority is to decode enough of `$28288` / `$2851E` and the writes/producers of `$1C007E` to reproduce a bounded object collision path. If that cannot be established rigorously, move to another listed gameplay priority without inventing values.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
