# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test87-V0102-source.zip`
   - SHA256: `e1f9bc4f742f3a9f6d38be131b439468bf053399104dae248c476881bf3d35c4`
3. Materialize/extract that snapshot and continue from **Test88 / V0.103**. Do not resume from older checkpoints or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use preserved `program.bin` and original research data. If behaviour is not demonstrated, mark it provisional or registered-but-not-implemented instead of inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Immediate continuation

Tests83-87 correct and narrow the double-team investigation:
- `$F560` is globally serviced; the raw `$0B/$0C/$29` pair machine is not Restrain-only.
- Restrain's D0 `$08/$09` calls to `$10D3A` are effect/entity descriptor IDs, not contextual action categories.
- Restrain sets `+$33 bit6` on attacker and target, but apron-partner ownership remains unproven.
- `$1E2DE/$1E30A` are generic per-action callback consumers reached from dispatcher `$1DFEA` via table `$1E018`: action `$72` uses `$1E2A8`, actions `$00/$21` use `$1E2F6`.
- Their restrained-target distance/range gates are exact and modeled: `$72` uses `$28`; `$00/$21` use `$30`.

Next priority for **Test88**: narrow actor roles around `$1CA00/$1CBB4/$1CE06/$1D9BC`. If apron ownership still cannot be proven, switch to the historically documented headlock-to-corner-tag or prone-feet-partner-ground double-team path and recover the ROM-side trigger without inventing semantics.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
