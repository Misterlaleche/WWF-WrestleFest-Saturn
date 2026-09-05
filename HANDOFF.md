# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test64-V0079-source.zip`
   - SHA256: `a9c204df4078dda34f97dd9a1e90ea2b223e529e0850f542b1eca2f0e753e6f7`
3. Materialize/extract that snapshot and continue from **Test65 / V0.080**. Do not resume from older checkpoints or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the preserved `program.bin` 68000 image and the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

The old Test52 persistent ZIP omitted required parent sources and cannot compile by itself. The line was deterministically recovered from Test51 and subsequently validated through Test64. Test64 is now the unique current source of truth.

## Immediate continuation

Test56-64 decoded and integrated more of the ringside-object lifecycle:
- `$FA14`: both available objects reset to state 3 with raw `+$0E=$0100`.
- `$FDEA` dispatches object state 2 to `$FE82`; its tail applies resolver-generated `+$38/+$3A/+$3C` contact corrections to X/Y/Z and collision flag bit 4 advances to state 4.
- `$FF54` state-4 clock reloads `$10` and returns the object to state 3 at phase 4 (49 handler invocations from fresh entry).
- `$280DC` clears correction fields, probes with `+$3E=$20`, retries with `-$20` after a miss, and performs no XYZ integration after two misses.
- `$28128` indexes table `$28154` by `($1C007E & 7) * 8 + (+$32 & 4)`. Both `$FFD2` objects have `+$32=$0004`. Safely bounded global modes 0..6 route to `{$28288, RTS, $2851E, RTS, RTS, $28288, $2851E}`.
- Test62 implements exact `$2851E` correction/contact behavior: Y bounds `$110/$140`, Z floor `$100` plus object `+$42`, X bounds `$1C8/$470`, contact-code-4 window `$294 <= X < $38C`.
- Test63 reproducibly audits all absolute-long `$1C007E` references. All 13 direct `MOVE.w #imm,$001C007E` producers write only 3 or 4. This does **not** exclude indirect/copied producers.
- Test64 implements exact `$28288..$2831A` behavior: Z floor `$100` plus object `+$42`, sloped left/right X walls derived with the original `LSL.l #8` and signed `DIVS.w #$02E0`, strict coarse gates `$01C7/$0360`, unsigned boundary tests, correction flags and contact code 3.

Next priority for Test65 is to continue dataflow analysis around `$1C007E` and determine whether modes 0/2/5/6 can occur for live thrown ringside objects. Both non-RTS resolver families are now exact, so do not wire them into runtime based on guessed global-mode semantics. If the producer path cannot be bounded rigorously, switch to another real gameplay priority.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
