# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test82 / V0.097**
- Host regression: **79 host binaries PASS** when split 40/39 to avoid runner timeout
- Targeted ASan+UBSan: **pair category $08, pair category $09, action-$29 pair-state transition PASS**
- Clean-extraction recovery smoke: **ZIP CRC PASS; extracted snapshot contains `program.bin`, new pair-state sources, and reruns Tests79/81/82 PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test82-V0097-source.zip`
- Snapshot SHA256: `c87275dea792073d8c8228235aed2f2d611d7e84e822c3b66cfdae7e9403ee47`
- Previous canonical checkpoint: `/WWF-Saturn/WWF-Saturn-Test74-V0089-source.zip`, SHA256 `5f3b52220fb842fb23199dfb8519bd6264c72f93d4ad6bab4def9d13a195bd93`

### Recovery / lineage note

The old Test52 persistent ZIP was incomplete. Test55 recovered that line from Test51. Test65 later exposed a second manifest defect and Test66 fixed checkpoint packaging so active `arcade_*`, campaign modules and `program.bin` are retained. During Test67 continuation an obsolete Test58-derived branch was detected and removed before repo state was overwritten. Tests67-74 were reapplied on canonical Test66. Tests75-82 continue directly from Test74. `arcade_pair_state.c/.h` are now included in the explicit checkpoint manifest.

### Major state through Test74

- Arcade HUD/stamina model, CPU progression, ten-stage TAG campaign, Continue flow, persistent player stamina, CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are integrated.
- Ringside object pickup/use animation, CPU weapon seeking/use, release geometry, state-2 collision correction and state-4 reavailability lifecycle are substantially ROM-backed.
- Complete two-button action layer is recovered: input normalization, pickup/use, full 12×21×3 contextual action matrix, running counters, behind actions, stationary actions and contextual pin.
- CPU mapped trademark finishers execute for grapple, prone, interception and behind contexts; opportunity frequency for non-grapple finishers remains provisional.

### Test75-82 exact paired/ground-state progress

- **Test75:** exact raw classifier for prone categories `$05/$06/$07`: shared attacker/target gates plus target `+$65` / `+$70` split.
- **Test76:** raw prone fields + wrestler + physical P/K now resolve exact action ID through the 756-byte contextual matrix.
- **Test77:** callback entry signatures for action `$1C` Sleeper (`$1567C`) and `$1D` Restrain (`$15870`) recovered, including demonstrated raw target state/pose/flag writes.
- **Test78:** complete behind-category `$10` predicate recovered; geometry is only the final gate after raw attacker/target state/link/flag checks.
- **Test79:** category `$08` requires an existing `+$26` pair link, attacker raw state `$0B`, and clear attacker `+$44` bits 5/6; on success target `+$44` bit 6 is set. Category `$08` maps universally to action `$29` for P/K/P+K across all 12 internal wrestler IDs.
- **Test80:** category `$09` requires an existing pair link, attacker raw state `$0C`, attacker `+$45=1`; it clears attacker `+$44`. Category `$09` universally maps P/K/P+K to actions `$15/$17/$15`.
- **Test81:** exhaustive direct-immediate audit finds only two producer families for raw state `$0B`: initial pair producer `$F726/$F72C` and action-$29 reentry `$165E4/$165EA`. The initial producer is reached only after strict `|dx|<$48`, `|dy|<$10` final geometry gates. Indirect producers are not claimed absent.
- **Test82:** exact state-changing tail of action `$29` callback `$16582` modeled. First terminal `$FE` completion increments linked target phase and returns both actors to raw `$0B`; second completion places attacker in `$0C` with `+$44=$2000` and target in `$FF`. The non-bit7 path transfers `+$44` bit7 to the target and terminates its raw state.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds/weighting are exact.
- Exact initial ringside-object coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55; move identities/readiness are recovered, opportunity frequency is native pending exact AI recovery.

### Registered but not yet exact/complete

- Semantic identity of the initial `$F5E4->$F726` raw-pair arbitration path. The `$0B/$0C/$29` state machine is now demonstrated, but it is **not yet labeled as a specific arcade double-team** until its caller/actor roles are proven.
- Exact semantic/action callback mapping for paired category `$09` actions `$15/$17`.
- Full live Saturn mapping of raw paired actor fields/states; Test77-82 deliberately avoid guessing native state equivalence.
- Indirect/dataflow producers and semantic mapping of global `$1C007E`; resolver mode 7 remains unknown.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

Next priority for **Test83 / V0.098**: continue tracing the initial pair producer `$F574..$F77E` and its callers/actor roles. If its semantic relationship to Restrain/double-team can be proven, connect that lifecycle; otherwise keep the raw pair state machine exact but unlabeled and move to another evidence-backed TAG/AI priority.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
