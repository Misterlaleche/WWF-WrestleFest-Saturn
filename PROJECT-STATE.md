# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test63 / V0.078**
- Host regression: **60/60 host executables PASS, 80 PASS result lines**
- Targeted ASan+UBSan: **new `$2851E` resolver helper PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test63-V0078-source.zip`
- Snapshot SHA256: `e52ec17fc7693504f667028984d366e54149d66c0c9845a2af95283f446321d9`
- Previous recoverable checkpoint: `/WWF-Saturn/WWF-Saturn-Test62-V0077-source.zip`, SHA256 `0dc6121b637b14848277dab06fa16f91385460958412a7dd86646670974d50ba`

### Recovery note

The old persistent Test52 ZIP was incomplete and could not compile by itself. Test55 deterministically recovered that work from the intact Test51 parent. Test63 descends from that recovered line and is now the current source of truth.

### Major state reached through Test63

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
- Test61 decodes the bounded `$280DC -> $28128 -> $28154` dispatch used by ringside objects. For safely bounded global modes 0..6 the exact object handlers are `0->$28288`, `1->RTS`, `2->$2851E`, `3->RTS`, `4->RTS`, `5->$28288`, `6->$2851E`; mode 7 remains unknown.
- **Test62 implements the exact `$2851E` second-column resolver** used by both `$FFD2` objects in modes 2 and 6: Y bounds `$110/$140`, Z floor `$100` plus object `+$42`, X bounds `$1C8/$470`, contact-code-4 window `$294 <= X < $38C`, and exact correction/flag ordering.
- **Test63 audits every absolute-long reference to global `$1C007E` and proves all 13 direct immediate writes are only values 3 or 4.** These direct setup paths therefore select the no-op object route, but indirect/copied producers remain unproven and are not assumed absent.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds and 50/50 weighting are exact.
- Exact initial object-coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55: finisher identity, descriptors, graphics and readiness contexts are recovered arcade data, but the policy to take every ready critical-target opportunity is native Saturn policy pending exact 68000 AI recovery.

### Registered but not yet exact/complete

- Geometry inside resolver handler `$28288`.
- Indirect/dataflow producers and semantic mapping of global `$1C007E`; Test63 only proves direct immediate absolute writes.
- Exact arcade input mapping for human weapon use.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

Next priority: follow dataflow around `$1C007E` enough to determine whether modes 2/6 can occur for live thrown ringside objects; in parallel decode `$28288` only where arithmetic can be reproduced exactly. If that path stops being safely bounded, move to another gameplay priority rather than inventing geometry.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
