# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test58-V0073-source.zip`
   - SHA256: `3a412eed3d54009cce11c2a5bf057073e9e127462983291f12775226fc55c328`
3. Materialize/extract that snapshot and continue from **Test59 / V0.074**. Do not resume from Test50, Test51, Test55, or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

The old Test52 persistent ZIP omitted required parent sources and cannot compile by itself. The line was deterministically recovered from Test51 and subsequently validated through Test58. Test58 is now the unique source of truth.

## Immediate continuation

Test56-58 decoded additional ringside-object lifecycle from ROM:
- `$FA14`: both available objects reset to state 3 with raw `+$0E=$0100`.
- `$FDEA` dispatches object state 2 to `$FE82`; its tail integrates velocity fields `+$38/+$3A/+$3C` into X/Y/Z and collision flag bit 4 advances to state 4.
- `$FF54` state-4 clock reloads `$10` and returns the object to state 3 at phase 4 (49 handler invocations from fresh entry).

Next priority for Test59 is to identify the producer of state-2 velocity fields `+$38/+$3A/+$3C` and/or the exact collision/damage semantics. If those cannot be demonstrated safely, move to another listed gameplay priority rather than inventing values.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
