# New-chat handoff — WWF WrestleFest Saturn

Use this repository and the persistent ChatGPT Library as the source of truth when continuing development in a new conversation.

## Start here

1. Read `PROJECT-STATE.md` from this private repository.
2. Use the latest validated persistent source snapshot named there. Current validated source:
   - Library path: `/WWF-Saturn/WWF-Saturn-Test74-V0089-source.zip`
   - SHA256: `5f3b52220fb842fb23199dfb8519bd6264c72f93d4ad6bab4def9d13a195bd93`
3. Materialize/extract that snapshot and continue from **Test75 / V0.090**. Do not resume from older checkpoints or the incomplete Test52 ZIP.
4. Each Test must be a real implementation or rigorous validation checkpoint; no empty version-number increments.
5. Preserve arcade fidelity. Use the preserved `program.bin` 68000 image and the user's original ROM/research data where available. If an arcade behaviour is not demonstrated, mark it provisional or registered-but-not-implemented rather than inventing exactness.
6. Run host regression/sanitizer tests as appropriate and persist major validated checkpoints back to Library. Keep `PROJECT-STATE.md` updated.

## Recovery note

Test66 is the parent of the current line and fixed the persistent checkpoint manifest after earlier incomplete snapshots. During continuation an obsolete Test58-derived branch was detected before repo state was overwritten; its conflicting Library ZIP was removed. Tests67-74 were then reapplied on top of canonical Test66 and Test74 was validated from a clean extraction of its own persistent ZIP.

## Immediate continuation

Tests67-74 recover and integrate the original two-button input/action layer:
- `$64EA..$6532`: active-low byte -> four directions + two physical action buttons + fresh edges.
- Human object pickup: either fresh physical action button can select action `$70` in the valid contact context.
- Held-object human use: category `$11` resolves button1/button2/both to action `$1E` in all 12 tables.
- Complete `$E4FE` contextual matrix: 756 bytes = 12 internal wrestlers × 21 categories × 3 button combinations, byte-verified.
- Running-opponent category `$12` action resolver recovered.
- Behind category `$10`: same orientation, `|dx|<$50`, `|dy|<$10`; button1 `$1C` Sleeper, button2 `$1D` Restrain, both `$FF`.
- Stationary category 0: button1 `$00`, button2 `$72`, P+K `$FF`; the previous normal Saturn A+B-finisher and neutral P+K-Big-Boot shortcuts are removed.
- Prone categories `$05/$06/$07`: button2 is always action `$48`; live ground K now enters pin and Saturn R no longer maps synthetic PIN.

Next priority for **Test75**: decode the exact raw predicates that distinguish categories `$05/$06/$07` (the `$E0B8/$E110` region) and implement a pure ROM-backed classifier before connecting more ground actions. Continue tracing action `$1C/$1D` callbacks for Sleeper/Restrain and the demonstrated Restrain-dependent double-team path, but do not invent lifecycle rules.

## Long target

Continue toward **Test200**, intended as an alpha-class integrated build candidate. Priorities include stamina/CPU progression, wrestler/partner AI, wrestler-specific moves, TAG/double-team/interference behaviour, ring-out/count-out/ringside objects, Royal Rumble, campaign/stages/cage, records, Continue/Buy-In/Regain Power, Game Over, attract/presentation, and other arcade systems not yet ported.

Do not generate a BIN/CUE on every test. The next hardware build is deferred until the long integration checkpoint is mature, unless a Saturn-only blocker requires one earlier.

At Test200, produce the requested final report divided into:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
