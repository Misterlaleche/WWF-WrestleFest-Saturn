# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test130 / V0.145**
- Full host regression: **126/126 binaries PASS**, executed in bounded batches because the monolithic runner exceeds the environment time budget.
- Tests127-130 targeted ASan/UBSan: **PASS**.
- SH-2/Yaul ELF compile+link: **PASS** (`wrestlefest-saturn-test130-v0145.elf`).
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test130-V0145-source.zip`
- Snapshot SHA256: `ab2b4c89d8a70d7121293e7be4c2023fff6e4881c494857021786d1208132b7a`
- Snapshot contains 416 source/data/report files and no disc images.
- Test100 remains the latest Saturn BIN/CUE and is confirmed booting/running on real NTSC-U Saturn + Terraonion MODE. No newer BIN/CUE was generated.

## Canonical lineage

Test130 continues from canonical Test119/V0.134 and preserves the validated Test102→Test114→Test118→Test119 line. Earlier divergent Test105/Test115/Test122 working snapshots are not canonical.

## Tests120-126 — Royal Rumble exact rules + live elimination route

- **Test120:** recovers the exact six raw Royal Rumble opening anchors at ROM `$10ACC`.
- **Test121:** reproduces the six-word reverse free-slot scan (`5 -> 0`) used by `$10AF0/$10B26`.
- **Test122:** recovers the exact post-elimination wait at `$BB30`: `$0126` = **294 ticks** before servicing a pending entrant.
- **Test123:** recovers the `$BB80/$BC00` next-entrant selection structure: two wrestler pools (IDs 0-5 / 6-11), up to four random attempts, then reverse fallback scan. Exact RNG source/order remains isolated where not yet recovered.
- **Test124:** integrates the exact 294-tick wait and six-slot ownership into the canonical replacement lifecycle while preserving Test119 `NEW CHALLENGER`, queue ownership, human-loss semantics and final-survivor victory.
- **Test125:** human headlock composition can move as one unit, enabling the documented rope setup and future headlock-to-corner/tag routes.
- **Test126:** Royal Rumble headlock + rope + Punch Body Slam elimination is live for wrestlers whose arcade repertoire contains BODY SLAM; Mr. Perfect correctly lacks this shortcut. Saturn rope threshold/coordinate translation remains PROVISIONAL.

## Tests127-130 — Difficulty and AI audit

- **Test127:** exact operator packing proves Difficulty comes from `$140022` bits 8-9 and becomes `$1C0066` bits 8-9 after inversion.
- **Test128:** complete direct-consumer audit finds the `$0300` Difficulty field extracted once at `$107F8` for CPU stamina setup; there are no direct `BTST #8/#9,$1C0066` AI cadence/aggression consumers in the preserved program ROM. Do not invent a Hard=faster-AI multiplier.
- **Test129:** reproduces the original `$24CC` weighted byte-table selector after its accepted random value is normalized to 1..99. The native random source remains separate when not yet recovered.
- **Test130:** recovers two exact 12-wrestler weighted distance-gate tables used by the state-0 CPU branch around `$1F318`:
  - raw `$C8..$DF`: `8,14,5,7,16,10,3,12,7,8,10,12`
  - raw `>= $E0`: `20,30,15,20,35,25,10,25,12,18,25,22`
  These prove wrestler-specific AI policy. Raw-distance-to-Saturn coordinate mapping and the semantic/native mapping of the selected raw state are not yet claimed exact.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal animation mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, visible entrant callout, human elimination and final-survivor resolution.

## Highest-priority open work

- Continue ROM-side AI semantic recovery and connect wrestler-specific policy only where raw state/action meaning and coordinate translation are demonstrated.
- Player stamina complaint from Test100: preserve exact difficulty->CPU-stamina behavior and audit damage/recovery/state consumers before changing values globally.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation.
- Missing roll/get-up animation fidelity after knockdown.
- Royal exact entrant artwork/path/timing, full 12-wrestler assets including Hawk/Animal, and additional elimination forms.
- TAG and Royal victory/defeat staging and animations as in arcade.
- Mode-select glow, records, portrait/HUD palette fidelity and additional attract/promo scenes.
- Ring-out/ringside hardware validation and audio.
- Future title Start options screen remains separate until user-supplied art is ready.

## Provisional

- Native RNG/entropy where the original arcade source is not yet recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw→Saturn coordinate/scale transform, including reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Earlier documented Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test131 / V0.146**. First prove the semantic meaning of the Test130 weighted state transition and the raw state-2 handler before wiring those percentages into live Saturn CPU movement. Then continue CPU damage/stamina/recovery auditing and post-match victory/defeat presentation. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
