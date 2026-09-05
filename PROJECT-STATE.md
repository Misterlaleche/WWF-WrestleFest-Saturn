# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test119 / V0.134**
- Prior full host regression at Test118: **114/114 binaries PASS**.
- Test119 targeted Rumble regression (Tests115-119): **PASS**.
- Test119 targeted ASan/UBSan: **PASS**.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test119-V0134-source.zip`
- Snapshot SHA256: `2ede6f3368ac76ed0443de3c3dddb6f7255e6349e8e1f03857cced40086f8148`
- Test100 remains the latest Saturn BIN/CUE and is confirmed booting/running on real NTSC-U Saturn + Terraonion MODE. No newer BIN/CUE was generated.

The complete host runner was attempted twice after Test119 but exceeded the execution-time budget before completion; no regression failure was observed before timeout. The changed Rumble block and its four predecessor lifecycle tests pass, and Test119 itself passes under ASan/UBSan.

## Canonical lineage

Test119 continues directly from validated Test118/V0.133, itself continuing from Test114/V0.129 and canonical Test102/V0.117. Obsolete divergent Test105/Test115 snapshots remain invalid.

## Tests115-119 — Royal Rumble lifecycle/presentation

- **Test115:** Rumble session constructs a complete unique ten-wrestler entrant order with the human selection first. The post-human ordering source remains **PROVISIONAL** until arcade RNG recovery.
- **Test116:** six live actor slots are separated from the overall entrant queue. CPU elimination clears active ownership once, increments elimination count once, consumes the next queued wrestler, exposes an announcement phase and then activates the replacement through an entry phase.
- **Test117:** human over-rope elimination ends the Rumble immediately and never consumes a queued replacement entrant.
- **Test118:** player victory resolves only after every other live actor is eliminated, no replacement is pending and the entrant queue is exhausted.
- **Test119:** the live Saturn renderer is now connected to the validated replacement lifecycle. During the pre-entry phase it displays `NEW CHALLENGER` plus the exact queued replacement identity from the single authoritative queue/slot state; the eliminated slot remains inactive until the announcement expires.

The Test119 overlay is a real runtime presentation connection, but exact arcade artwork, coordinates and the existing 90-tick announcement duration remain **PROVISIONAL** pending ROM/video measurement. Entry duration, entry geometry and queue ordering entropy also remain provisional.

See `docs/TEST119-V0134.md` inside the source checkpoint.

## Preserved state from Test103-118

TAG reserve HUD/stamina, reserve render correction, rope traversal animation mapping, staged TAG presentation, referee tracking, pin indicator, full-pool CPU selection, cached selection pages, recovery prompt, rope deflection, six-position Rumble opening + 1P marker, complete Rumble queue ownership, replacement lifecycle, human elimination semantics and final-survivor resolution remain integrated.

## Open hardware-feedback / fidelity work

- CPU difficulty/player stamina behavior still needs ROM-side scheduler/damage recovery; do not solve by arbitrary global weakening.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation remain incomplete.
- Royal Rumble exact entrant announcement artwork/text, exact entry path/timing and ROM RNG/order remain incomplete.
- Post-match winner/loser staging, mode-select glow, records, portrait/HUD palette fidelity and additional attract/promo scenes remain incomplete.
- Ring-out/ringside behavior still needs later hardware validation.
- Audio remains pending.
- Future title Start options screen remains separate until user-supplied art is ready.

## Provisional

- Native RNG/entropy where original arcade source is not yet recovered, including Rumble entrant order.
- Rumble announcement artwork/layout/timing and native entry geometry/timing.
- Exact raw→Saturn coordinate/scale transform, including reserve visual correction.
- Referee tracking trajectory.
- TAG presentation timing values.
- Rope deflection curve.
- Dedicated rope-entry animation mapping.
- Earlier documented Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test120 / V0.135**. First target: refine replacement entry ownership so an entrant can be visible in its entry animation without becoming a legal combat participant until the entry phase completes; preserve six-slot rendering/ownership and final-survivor semantics. In parallel continue ROM recovery for CPU decision cadence and stamina/damage so the Test100 difficulty complaint is corrected from evidence rather than arbitrary tuning. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
