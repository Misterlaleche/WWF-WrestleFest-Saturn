# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test85-V0100-source.zip`
   - SHA256: `0c42b3367b612cddf9e6a67e3a68a32839a2d3838dfcf642950dd4f011c50f14`
3. Materialize/extract that snapshot and continue from **Test86 / V0.101**. Do not resume from older checkpoints or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use preserved `program.bin` and original research data. If behaviour is not demonstrated, mark it provisional or registered-but-not-implemented instead of inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Immediate continuation

Tests83-85 correct and narrow the double-team investigation:
- `$F560` is called in both main frame-loop branches and immediately invokes `$F574`; the raw `$0B/$0C/$29` pair state machine is globally serviced and is not a Restrain-only callback path.
- Restrain's calls to `$10D3A` with D0 `$08/$09` are effect/entity descriptor IDs (pool `$1C1258`, table `$10DDA`, index `D0*6`), not contextual action categories. Do not reconnect them to categories `$08/$09`.
- Restrain sets `+$33 bit6` on attacker and target. That exact bit is consumed by multiple later AI/control branches. Historical move-guide evidence says Restrain lets the tag partner enter and strike, but the exact apron-partner consumer branch remains unproven.

Next priority for **Test86**: identify actor roles around `+$33 bit6` consumers, especially `$1CA00/$1CBB4/$1CE06/$1E2DE`, and prove which belongs to the apron/tag-partner controller. If proven, implement the first Restrain-dependent double-team entry. If not, switch to the next documented double-team path: headlock-to-corner tag or prone-feet partner ground attack.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
