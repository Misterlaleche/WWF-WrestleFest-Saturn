# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test85 / V0.100**
- Baseline host regression through Test82: **79 host binaries PASS** when split 40/39 to avoid runner timeout
- Tests83-85 targeted validation: **PASS**
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test85-V0100-source.zip`
- Snapshot SHA256: `0c42b3367b612cddf9e6a67e3a68a32839a2d3838dfcf642950dd4f011c50f14`
- Clean-extraction smoke: ZIP CRC PASS; extracted snapshot reruns Tests83/84/85 PASS
- Previous canonical checkpoint: `/WWF-Saturn/WWF-Saturn-Test82-V0097-source.zip`, SHA256 `c87275dea792073d8c8228235aed2f2d611d7e84e822c3b66cfdae7e9403ee47`

### Recovery / lineage note

The old Test52 persistent ZIP was incomplete. Test55 recovered that line from Test51. Test65 exposed another manifest defect and Test66 fixed checkpoint packaging so active arcade/campaign modules and `program.bin` remain recoverable. An obsolete Test58-derived branch was later detected and removed before repo state was overwritten. Test85 continues from the validated Test82 line.

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

### Test83-85 semantic corrections / double-team research

- **Test83:** `$F560` has exactly two callers in the two main frame-loop branches (`$0FD0`, `$1080`) and immediately invokes `$F574`. Therefore the `$0B/$0C/$29` machine is globally serviced and is not a Restrain-only callback path.
- **Test84:** Restrain callback values D0=`$08/$09` passed to `$10D3A` are effect/entity descriptor IDs, not contextual action categories. `$10D3A` allocates from pool `$1C1258` and indexes table `$10DDA` with `D0*6`; any category linkage is rejected.
- **Test85:** Restrain sets actor field `+$33 bit6` on both attacker and target. That exact flag is consumed by multiple later AI/control branches (`$1C408`, `$1CA00`, `$1CBB4`, `$1CE06`, `$1D9BC`, `$1E2DE` among others). Historical move-guide research states Restrain allows the partner into the ring to strike, but the specific apron-partner consumer branch is not yet proven, so no exact double-team lifecycle is claimed yet.

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

Next priority for **Test86 / V0.101**: identify actor roles around the direct `+$33 bit6` consumers, especially `$1CA00/$1CBB4/$1CE06/$1E2DE`, and prove which branch belongs to the apron/tag-partner controller. If that can be established, implement the first Restrain-dependent double-team entry; otherwise switch to the next guide-demonstrated double-team path (headlock-to-corner tag or prone-feet partner ground attack) without inventing semantics.

The next hardware BIN/CUE remains deferred until the long integration campaign is mature enough for an alpha-class Saturn build, unless a Saturn-only blocker requires one earlier.
