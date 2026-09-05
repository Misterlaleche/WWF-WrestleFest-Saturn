# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test118 / V0.133**
- Full host regression: **114/114 binaries PASS**.
- Tests115-118 targeted ASan/UBSan: **PASS**.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test118-V0133-source.zip`
- Snapshot SHA256: `a3b7b197828548c51fe0beda249e603c959c949b2d6f75146fe846418c93cff3`
- Test100 remains the latest Saturn BIN/CUE and is confirmed booting/running on real NTSC-U Saturn + Terraonion MODE. No newer BIN/CUE was generated.

## Canonical lineage

Test118 continues directly from validated Test114/V0.129, itself rebased on canonical Test102/V0.117. Obsolete divergent Test105/Test115 snapshots remain invalid.

## Tests115-118 — Royal Rumble lifecycle

- **Test115:** Rumble session now constructs a complete unique ten-wrestler entrant order with the human selection first. The post-human ordering source remains **PROVISIONAL** until arcade RNG recovery.
- **Test116:** six live actor slots are separated from the overall entrant queue. CPU elimination clears active ownership once, increments elimination count once, consumes the next queued wrestler, exposes an announcement phase and then activates the replacement through an entry phase.
- **Test117:** human over-rope elimination ends the Rumble immediately and never consumes a replacement entrant.
- **Test118:** player victory resolves only after every other live actor is eliminated, no replacement is pending and the entrant queue is exhausted.

Announcement duration, entry duration, entry geometry and queue ordering entropy are **PROVISIONAL** pending direct ROM/video measurement; the native implementation does not claim these values are arcade exact.

See `docs/TEST115-118-RUMBLE-LIFECYCLE.md` inside the source checkpoint.

## Preserved state from Test103-114

TAG reserve HUD/stamina, reserve render correction, rope traversal animation mapping, staged TAG presentation, referee tracking, pin indicator, full-pool CPU selection, cached selection pages, recovery prompt, rope deflection and six-position Rumble opening + 1P marker remain integrated.

## Open hardware-feedback / fidelity work

- CPU difficulty/player stamina behavior still needs ROM-side scheduler/damage recovery; do not solve by arbitrary global weakening.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation remain incomplete.
- Royal Rumble entrant announcement artwork/text, exact entry path/timing and ROM RNG/order remain incomplete.
- Post-match winner/loser staging, mode-select glow, records, portrait/HUD palette fidelity and additional attract/promo scenes remain incomplete.
- Ring-out/ringside behavior still needs later hardware validation.
- Audio remains pending.
- Future title Start options screen remains separate until user-supplied art is ready.

## Provisional

- Native RNG/entropy where original arcade source is not yet recovered, including Rumble entrant order.
- Rumble announcement/entry timing and native entry geometry.
- Exact raw→Saturn coordinate/scale transform, including reserve visual correction.
- Referee tracking trajectory.
- TAG presentation timing values.
- Rope deflection curve.
- Dedicated rope-entry animation mapping.
- Earlier documented Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test119 / V0.134**. First target: connect the Rumble replacement lifecycle to visible entrant announcement/presentation and refine entry ownership without breaking six-active-slot combat. In parallel continue ROM recovery for CPU decision cadence and stamina/damage so the Test100 difficulty complaint is corrected from evidence rather than arbitrary tuning. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
