# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test74 / V0.089**
- Full host regression: **91 PASS result lines, exit 0**
- Targeted ASan+UBSan: **physical input, complete action matrix, behind actions, live contextual pin PASS**
- Clean-extraction recoverability: **ZIP CRC PASS; extracted snapshot reruns all 91 host PASS results with exit 0**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test74-V0089-source.zip`
- Snapshot SHA256: `5f3b52220fb842fb23199dfb8519bd6264c72f93d4ad6bab4def9d13a195bd93`
- Previous canonical checkpoint: `/WWF-Saturn/WWF-Saturn-Test66-V0081-source.zip`, SHA256 `221ae2a615312960d2e9e477d6f245716b33a671a0742a94f056cf1de2703743`

### Recovery / lineage note

The old Test52 persistent ZIP was incomplete. Test55 recovered that line from Test51. Test65 later exposed a second manifest defect and Test66 fixed checkpoint packaging so active `arcade_*`, campaign modules and `program.bin` are retained. During the Test67 continuation, an obsolete branch reconstructed from Test58 was detected before GitHub state was overwritten; its conflicting Library snapshot was removed. Tests67-74 were then reapplied on top of the canonical Test66 snapshot. Test74 is now the unique source of truth.

### Major state reached through Test66

- Arcade HUD/stamina model, CPU stamina progression and ten-stage TAG campaign are integrated.
- Victory advances campaign; defeat/draw enters Continue; retry preserves stage/opponents; player stamina persists by wrestler identity.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are active.
- Opponent selection reproduces the ROM pool/weight structure and forces Hawk/Animal at stages 5 and 10, with a runtime guard until their graphical banks are implemented.
- Ringside objects use original pickup/use animations and recovered state lifecycle through available/held/thrown/post-throw states.
- Test51: CPU held-object mid-range branch `$1D032`, strict Y/X bounds and exact `$1D066={$32,$32}` 50/50 action-$1F decision.
- Test52: callback `$15A20` release geometry, state 2, exact `+$60/-$60` X and `+$40` height.
- Test53-55: CPU trademark finisher execution for grapple, prone, interception and behind contexts; non-grapple opportunity policy remains provisional.
- Test56-60: object reset, state-2 correction semantics, two-probe controller, state-4 49-call reavailability clock and live runtime integration.
- Test61-65: bounded `$280DC` resolver dispatch and both exact resolver families, including sloped-wall arithmetic, contact flags and exact two-probe orchestration. Mode 7 / indirect `$1C007E` producers remain unproven.
- Test66: checkpoint packaging recoverability invariant fixed.

### Test67-74 physical input/action progress

- **Test67:** ROM `$64EA..$6532` active-low gameplay input normalization: four direction bits, two physical action buttons, and fresh-edge `current & ~previous`.
- **Test68:** human ringside pickup `$DEAE->$F0BA->$F106` now uses either fresh physical action button for action `$70`; synthetic `INPUT_GRAB` is no longer the normal pickup path.
- **Test69:** held-object category `$11` resolves physical button1/button2/both to human action `$1E` in all 12 internal wrestler tables; `$1F` remains the separate CPU weighted branch.
- **Test70:** complete contextual action matrix recovered and byte-verified: **12 wrestlers × 21 categories × 3 button combinations = 756 bytes** from pointer table `$E4FE`.
- **Test71:** category `$12` running-opponent counter action selection exposed exactly per wrestler/button combination; callback/damage remains separate until decoded.
- **Test72:** recovered behind context: same orientation, `|dx|<$50`, `|dy|<$10`; category `$10` maps button1->$1C Sleeper Hold, button2->$1D Restrain, both->$FF. Full paired lifecycle remains to be reconstructed.
- **Test73:** stationary category 0 is button1->$00, button2->$72, P+K->$FF for all wrestlers. Normal Saturn A+B->FINISHER and stationary P+K->Big Boot shortcuts were removed; synthetic hooks remain diagnostic/internal only.
- **Test74:** all recovered prone categories `$05/$06/$07` map physical button2 to action `$48`. Live ground K now enters the existing cover/pin state; P+K keeps ground-action priority and Saturn R no longer maps to synthetic PIN.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds/weighting are exact.
- Exact initial ringside-object coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55; move identities/readiness are recovered, opportunity frequency is native pending exact AI recovery.

### Registered but not yet exact/complete

- Indirect/dataflow producers and semantic mapping of global `$1C007E`; resolver mode 7 remains unknown and is not guessed.
- Exact action callback/damage semantics for several contextual entries in the recovered 21-category matrix.
- Full Sleeper/Restrain paired lifecycle and the demonstrated double-team paths that depend on Restrain.
- Exact differentiation and runtime routing of prone categories `$05/$06/$07` beyond the now-exact pin action `$48`.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

Next priority for **Test75 / V0.090**: decode the exact predicates that distinguish prone categories `$05/$06/$07` and expose them as a ROM-backed classifier. Then use that classifier to replace additional synthetic ground-action selection where the action matrix proves the result. In parallel, continue tracing callbacks `$1C/$1D` toward exact Sleeper/Restrain lifecycle and demonstrated double-team entry without inventing semantics.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
