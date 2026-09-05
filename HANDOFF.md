# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test55-V0070-source.zip`
   - SHA256: `f36eea7aaae6c9ebf5bdb4c44b6f6890cbf27908b1a244df98afb64df822c243`
3. Materialize/extract that snapshot and continue from **Test56 / V0.071**. Do not resume from Test50, Test51, or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Important recovery note

The old Test52 persistent ZIP was discovered to omit required parent sources and cannot compile by itself. Test55 contains the deterministically recovered Test52 object-release work plus Tests53-55 and is the only current recoverable source of truth.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Immediate continuation after Test55: decode and port the subsequent ringside object state-2 trajectory/collision/reavailability lifecycle from ROM evidence where demonstrable; otherwise continue another priority block without inventing exact values.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
