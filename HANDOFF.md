# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test51-V0066-source.zip`
   - SHA256: `06f95c2faa2b1a28c6c8013df8cd361f1d1bd9912e99be63e4a6482b9b4a5ee6`
3. Materialize/extract that snapshot and continue from the next concrete test; do not rebuild from Test15/V0.025, V0.029, or Test50.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
