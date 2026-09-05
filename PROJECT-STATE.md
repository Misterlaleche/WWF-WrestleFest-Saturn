# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test114 / V0.129**
- Host regression: **110/110 host binaries PASS** (55/55 + 55/55).
- Tests103-114 targeted ASan/UBSan: **PASS**.
- `main.c`, `game.c`, `session.c`, `hud.c`: SH-2 compile check **PASS** with the pinned Yaul toolchain used for Test100.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test114-V0129-source.zip`
- Snapshot SHA256: `a58835923d3ad97da97c765ac652cd2c925bebc5a8d4f0edfd58e8204d32204d`
- Test100 remains the latest Saturn BIN/CUE and is confirmed booting/running on real NTSC-U Saturn + Terraonion MODE. No newer BIN/CUE was generated.

## Canonical lineage

Test114 is rebased on the validated Test102/V0.117 line. It preserves:
- Test101 ROM-backed Rumble active-actor stamina-recovery gate.
- Test102 original two-button standard corner TAG with Kick (Saturn C remains optional alias).

Obsolete divergent Test105/Test115 snapshots created from an earlier branch were removed from Library before this checkpoint was accepted.

## Tests103-114 — Test100 hardware-feedback block

- **Test103:** TAG HUD shows active/reserve stamina for both teams; reserve portraits are no longer forced grayscale.
- **Test104:** waiting reserve gets a render-only outward correction relative to the ropes. Gameplay coordinates remain unchanged. **PROVISIONAL** until next hardware comparison.
- **Test105:** ring enter/exit uses the verified common locomotion descriptor instead of a static idle slide. Dedicated transition descriptor remains unrecovered, so mapping is **PROVISIONAL**.
- **Test106:** TAG pre-match staging uses separate rear/front anchors and draw order based on the user's arcade references.
- **Test107:** TAG presentation is structured as four individual wrestler windows plus presenter exit (4×180 + 180 ticks). Timeline is ready for future audio; exact arcade timings remain **PROVISIONAL** pending direct measurement.
- **Test108:** referee follows live action instead of parking at centre. Exact referee pathing remains **PROVISIONAL**.
- **Test109:** live pin count exposes visible 1/2/3 indication.
- **Test110:** TAG CPU roster fill uses all ten selectable wrestlers instead of the previous fixed sequential choices. Entropy source remains **PROVISIONAL** pending arcade RNG recovery.
- **Test111:** both character-select pages are preloaded; switching page no longer performs a CD read.
- **Test112:** existing recovery-effort mechanic now exposes the rapid-button prompt after knockdown while preserving pin-escape prompt priority.
- **Test113:** local visual rope deflection appears during rebound. Exact arcade deformation curve remains **PROVISIONAL**.
- **Test114:** Royal opening uses six separated depth anchors and exposes a green `1P` marker for the human wrestler.

See `docs/TEST103-114-HARDWARE-FIDELITY.md` in the source checkpoint.

## Test100 hardware feedback still open

- CPU difficulty/player stamina behavior still needs ROM-side scheduler/damage recovery; do not solve by arbitrary global weakening.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation remain incomplete.
- Royal Rumble elimination/entrant queue, announcement box and ring-entry lifecycle remain incomplete.
- Post-match winner/loser staging, mode-select glow, records, portrait/HUD palette fidelity and additional attract/promo scenes remain incomplete.
- Ring-out/ringside behavior still needs later hardware validation.
- Audio remains pending; Test107 provides a structured TAG presentation timeline for eventual announcer sync.
- Future title Start options screen remains separate until user-supplied art is ready.

## Provisional

- Native RNG/entropy where original arcade source is not yet recovered.
- Exact raw→Saturn coordinate/scale transform, including reserve visual correction.
- Referee tracking trajectory.
- TAG presentation timing values.
- Rope deflection curve.
- Dedicated rope-entry animation mapping.
- Earlier documented Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test115 / V0.130**. First target: Royal Rumble lifecycle — prove/refactor elimination ownership, six-active-slot behavior, replacement entrant queue, entrant announcement and ring entry from ROM/reference evidence. In parallel continue ROM recovery for CPU decision cadence and stamina/damage so the Test100 difficulty complaint is corrected without arbitrary tuning. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
