# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test61-V0076-source.zip`
   - SHA256: `2953b7ccd38f1d84e8a3e46f94ac237a4ffa8ad8789a70f0a691a690158dc915`
3. Materialize/extract that snapshot and continue from **Test62 / V0.077**. Do not resume from Test50, Test51, Test55, Test58, Test60, or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

The old Test52 persistent ZIP omitted required parent sources and cannot compile by itself. The line was deterministically recovered from Test51 and subsequently validated through Test61. Test61 is now the unique source of truth.

## Immediate continuation

Test56-61 decoded and integrated more of the ringside-object lifecycle:
- `$FA14`: both available objects reset to state 3 with raw `+$0E=$0100`.
- `$FDEA` dispatches object state 2 to `$FE82`; its tail applies resolver-generated `+$38/+$3A/+$3C` contact corrections to X/Y/Z and collision flag bit 4 advances to state 4.
- `$FF54` state-4 clock reloads `$10` and returns the object to state 3 at phase 4 (49 handler invocations from fresh entry).
- `$280DC` clears correction fields, probes with `+$3E=$20`, retries with `-$20` after a miss, and performs no XYZ integration after two misses.
- Test60 wires the exact state-4 reavailability clock into the native autonomous-object runtime.
- Test61 recovers the bounded object resolver dispatch: `$28128` indexes table `$28154` by `($1C007E & 7) * 8 + (+$32 & 4)`. Both `$FFD2` objects have `+$32=$0004`. Safely bounded global modes 0..6 route to `{$28288, RTS, $2851E, RTS, RTS, $28288, $2851E}`. Mode 7 remains unknown rather than reading following opcodes as pointer data.

Next priority for Test62 is to decode enough of `$28288` / `$2851E` and the producers/writes of global `$1C007E` to reproduce the object world-boundary/collision path safely. Direct constant writes observed so far use values 3 and 4, but do not assume those are the only runtime values until indirect producers are bounded. If that path cannot be bounded rigorously, move to another gameplay priority instead of inventing geometry.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
