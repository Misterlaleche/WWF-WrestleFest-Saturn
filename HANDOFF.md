# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test82-V0097-source.zip`
   - SHA256: `c87275dea792073d8c8228235aed2f2d611d7e84e822c3b66cfdae7e9403ee47`
3. Materialize/extract that snapshot and continue from **Test83 / V0.098**. Do not resume from older checkpoints or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use preserved `program.bin` and original research data. If behaviour is not demonstrated, mark it provisional or registered-but-not-implemented instead of inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

Test66 fixed the persistent source manifest after earlier incomplete snapshots. Tests67-74 were reapplied on canonical Test66 after an obsolete branch was detected and removed. Test82 continues directly from Test74 and its checkpoint manifest includes the new pair-state sources.

## Immediate continuation

Tests75-82 add exact raw ground/paired-state behavior:
- prone categories `$05/$06/$07` are classified from raw actor fields and resolved through the exact 12×21×3 action matrix;
- Sleeper `$1C` and Restrain `$1D` callback entry writes are recovered;
- full behind category `$10` raw predicate is recovered;
- category `$08`: existing `+$26` link + attacker raw state `$0B` + clear `+$44` bits5/6; sets target `+$44` bit6 and universally selects action `$29`;
- category `$09`: existing link + attacker raw state `$0C` + `+$45=1`; clears attacker `+$44`, mapping P/K/P+K to `$15/$17/$15`;
- direct raw-state `$0B` audit finds initial producer `$F726/$F72C` and action-$29 reentry `$165E4/$165EA` only;
- action `$29` callback `$16582` exact tail is modeled: first terminal completion returns both actors to `$0B`, second leaves attacker `$0C` and target `$FF`.

Do **not** yet call `$F5E4->$F726` a specific double-team. Its raw pair state machine is demonstrated, but its caller/actor-role semantics still need proof.

Next priority for Test83: trace `$F574..$F77E` callers and actor roles far enough to prove or reject a Restrain/double-team relationship. If that cannot be established rigorously, switch to another evidence-backed TAG/AI priority rather than inventing semantics.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
