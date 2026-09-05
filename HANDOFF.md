# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test60-V0075-source.zip`
   - SHA256: `a91ceea8c3a4a366a93a158c524345bff471f7003446cb6ee2810a3fbefdd0da`
3. Materialize/extract that snapshot and continue from **Test61 / V0.076**. Do not resume from Test50, Test51, Test55, Test58, or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

The old Test52 persistent ZIP omitted required parent sources and cannot compile by itself. The line was deterministically recovered from Test51 and subsequently validated through Test60. Test60 is now the unique source of truth.

## Immediate continuation

Test56-60 decoded and integrated more of the ringside-object lifecycle:
- `$FA14`: both available objects reset to state 3 with raw `+$0E=$0100`.
- `$FDEA` dispatches object state 2 to `$FE82`; its tail applies `+$38/+$3A/+$3C` to X/Y/Z and collision flag bit 4 advances to state 4.
- `$FF54` state-4 clock reloads `$10` and returns the object to state 3 at phase 4 (49 handler invocations from fresh entry).
- Test59 corrects the old velocity interpretation: in `$FE82`, `$280DC` clears `+$38/+$3A/+$3C` and generates contact-correction deltas. `$FE82` probes with `+$3E=$20`, retries with `-$20` after a miss, and performs no XYZ integration after two misses. There is no demonstrated independent persistent launch-velocity producer on this path.
- Test60 wires the exact state-4 reavailability clock into the native autonomous-object runtime.

Next priority for Test61 is to decode enough of shared resolver `$280DC` to reproduce the object world-boundary/collision path safely. Both ringside objects are initialized by `$FFD2` with `+$32=$0004`, which selects the bit-2 branch of `$280DC`; dispatch is also affected by low bits of global `$1C007E`. Do not guess the global-mode mapping. If that path cannot be bounded rigorously, move to another gameplay priority instead of inventing geometry.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
