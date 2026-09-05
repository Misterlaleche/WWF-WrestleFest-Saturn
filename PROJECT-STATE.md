# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test87 / V0.102**
- Baseline host regression through Test82: **79 host binaries PASS** when split 40/39 to avoid runner timeout
- Tests83-87 targeted validation: **PASS**
- Tests86-87 ASan/UBSan targeted validation: **PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test87-V0102-source.zip`
- Snapshot SHA256: `e1f9bc4f742f3a9f6d38be131b439468bf053399104dae248c476881bf3d35c4`
- Previous checkpoint: `/WWF-Saturn/WWF-Saturn-Test86-V0101-source.zip`, SHA256 `08bc89e9bd7041944fd3baf2109fd33eb95268857776050ccbc3a26e51dd8cb0`

### Recovery / lineage note

The old Test52 persistent ZIP was incomplete. Test55 recovered that line from Test51. Test65 exposed another manifest defect and Test66 fixed checkpoint packaging so active arcade/campaign modules and `program.bin` remain recoverable. An obsolete Test58-derived branch was later detected and removed before repo state was overwritten. Test87 continues from the validated Test85 line.

### Major state through Test74

- Arcade HUD/stamina model, CPU progression, ten-stage TAG campaign, Continue flow, persistent player stamina, CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are integrated.
- Ringside object pickup/use animation, CPU weapon seeking/use, release geometry, state-2 collision correction and state-4 reavailability lifecycle are substantially ROM-backed.
- Complete two-button action layer is recovered: input normalization, pickup/use, full 12×21×3 contextual action matrix, running counters, behind actions, stationary actions and contextual pin.
- CPU mapped trademark finishers execute for grapple, prone, interception and behind contexts; opportunity frequency for non-grapple finishers remains provisional.

### Test75-82 paired/ground-state progress

- Test75: exact raw classifier for prone categories `$05/$06/$07`.
- Test76: raw prone fields + wrestler + physical P/K resolve exact action ID through the 756-byte contextual matrix.
- Test77: callback entry signatures for Sleeper `$1C` and Restrain `$1D` recovered.
- Test78: complete behind-category `$10` raw predicate recovered.
- Test79: category `$08` requires existing `+$26` link, attacker raw state `$0B`, clear attacker `+$44` bits5/6; sets target `+$44` bit6 and universally maps to action `$29`.
- Test80: category `$09` requires existing link, attacker raw state `$0C`, attacker `+$45=1`; clears attacker `+$44` and universally maps P/K/P+K to `$15/$17/$15`.
- Test81: direct-immediate raw-state `$0B` audit finds only initial pair producer `$F726/$F72C` and action-$29 reentry `$165E4/$165EA`; initial producer has strict `|dx|<$48`, `|dy|<$10` final geometry gates.
- Test82: exact state-changing tail of action `$29` callback `$16582` modeled: first terminal completion returns both actors to `$0B`; second leaves attacker `$0C`, target `$FF`.

### Test83-87 semantic corrections / Restrain research

- **Test83:** `$F560` has exactly two callers in the two main frame-loop branches (`$0FD0`, `$1080`) and immediately invokes `$F574`. Therefore the `$0B/$0C/$29` machine is globally serviced and is not a Restrain-only callback path.
- **Test84:** Restrain callback values D0=`$08/$09` passed to `$10D3A` are effect/entity descriptor IDs, not contextual action categories. `$10D3A` allocates from pool `$1C1258` and indexes table `$10DDA` with `D0*6`; any category linkage is rejected.
- **Test85:** Restrain sets actor field `+$33 bit6` on both attacker and target; multiple later AI/control branches consume it, but apron-partner ownership was not established.
- **Test86:** consumers `$1E2DE/$1E30A` are proven to belong to the generic per-action callback dispatcher at `$1DFEA` (`+$61` table `$1E018`): action `$72` -> `$1E2A8`, actions `$00/$21` -> `$1E2F6`. They are not evidence of a dedicated apron-partner controller.
- **Test87:** exact restrained-target distance/range branches for those generic callbacks are modeled: action `$72` requires `+$B0 >= $28` and writes `+$BC=$28`; actions `$00/$21` require `+$B0 >= $30` and write `+$BC=$30`.

### Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds/weighting are exact.
- Exact initial ringside-object coordinate conversion into the Saturn world model.
- CPU priority for non-grapple trademark opportunities in Test54/Test55; move identities/readiness are recovered, opportunity frequency is native pending exact AI recovery.

### Registered but not yet exact/complete

- Which `+$33 bit6` consumer is the actual Restrain-dependent apron-partner entry branch.
- Semantic identity of the global `$F5E4->$F726` raw-pair arbitration path.
- Exact semantic/action callback mapping for paired category `$09` actions `$15/$17`.
- Full live Saturn mapping of raw paired actor fields/states.
- Indirect/dataflow producers and semantic mapping of global `$1C007E`; resolver mode 7 remains unknown.
- Remaining TAG double-team forms, LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stage/cage work, richer Rumble eliminations, attract/presentation and audio.

## Long campaign

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence; uncertain behavior remains explicitly provisional or registered-but-not-implemented.

Next priority for **Test88 / V0.103**: continue actor-role narrowing around `$1CA00/$1CBB4/$1CE06/$1D9BC`; in parallel use the documented headlock-to-corner-tag and prone-feet-partner ground double-team paths as the fallback target. Do not infer apron ownership solely from Restrain `+$33 bit6` tests.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
