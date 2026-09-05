# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test137 / V0.152**
- Full host regression: **133/133 binaries PASS**, executed in bounded batches to avoid runner timeout.
- Tests122-137 targeted ASan/UBSan: **PASS**.
- SH-2 ELF compile/link with Yaul: **PASS** (`wrestlefest-saturn-test137-v0152.elf`). The later ISO step was intentionally not pursued; Test100 remains the latest hardware BIN/CUE.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test137-V0152-source.zip`
- Snapshot SHA256: `461912913a2edbf2b4be532c8378d81f74abbe3d088ff481647fda5283b59261`
- Clean extraction targeted regression: **11/11 PASS**.
- Source package includes `arcade_ai.c/.h`; `tools/package_release.py` was corrected so future snapshots cannot omit them.

## Canonical lineage

Test137 continues the canonical Test102→Test114→Test118→Test119→Test130→Test133 line. Earlier divergent working snapshots remain non-canonical.

## Tests120-126 — Royal Rumble exact rules + live elimination route

- Test120: exact six raw opening anchors at ROM `$10ACC`.
- Test121: exact six-word reverse free-slot scan (`5 -> 0`) used by `$10AF0/$10B26`.
- Test122: exact post-elimination wait at `$BB30`: `$0126` = 294 ticks.
- Test123: `$BB80/$BC00` two-pool next-entrant structure, four random attempts then reverse fallback scan; exact RNG source/order remains isolated.
- Test124: exact 294-tick wait and six-slot ownership integrated into the canonical replacement lifecycle while preserving the visible `NEW CHALLENGER` phase from Test119.
- Test125: human headlock composition can move as one unit.
- Test126: live headlock + rope + Punch Body Slam elimination for wrestlers whose arcade repertoire contains BODY SLAM; Saturn rope threshold remains provisional.

## Tests127-133 — Difficulty and wrestler-specific AI audit

- Test127: Difficulty originates from `$140022` bits 8-9 and becomes `$1C0066` bits 8-9 after inversion.
- Test128: the direct Difficulty field `$0300` is consumed at `$107F8` for CPU stamina setup; no direct Difficulty-bit aggression/cadence consumer was found.
- Test129: exact `$24CC` weighted selector recovered.
- Test130: exact two 12-wrestler probability tables at `$1F3A6/$1F3BE`.
- Test131: raw target-distance `+$B0 < $C8` uses table 1; `$C8..$DF` is a deadband; `>= $E0` uses table 2.
- Test132: selected bucket-0 transition writes raw actor state `+$20=$0002` and updates recovered flag fields; far branch has additional flag changes.
- Test133: dispatcher `$1C1F4` maps raw state 2 to `$1D7B6`; handler is target-relative. Precise semantic naming remains deliberately unclaimed where ROM evidence is incomplete.

## Tests134-137 — combat feedback and visible recovery of arcade behavior

- **Test134:** arcade stationary Punch action `$00` callback writes exact **3-point stamina damage**, replacing the old provisional 20-point native value for arcade-motion fighters. This directly addresses part of the Test100 hardware report that player stamina drained much too quickly.
- **Test135:** player-victim submission now exposes the live rapid-button prompt; fresh Punch/Kick edges feed the existing escape counter. Exact hold-specific timing/drain remains provisional where callbacks are not yet recovered.
- **Test136:** `ST_GROUND_ATTACK` now renders the exact catalogued ground-drop action descriptors instead of losing stamina with no visible CPU move: STOMP `$23->$15CE0`, ELBOW DROP `$24->$1603A`, KNEE DROP `$25->$16116`, FIST DROP `$26->$1623C`.
- **Test137:** CPU ground policy can choose arcade action `$22` PICK UP / descriptor `$15B6C`, returning a prone live opponent toward the rise path instead of only repeatedly damaging/pinning. Native completion tick/state translation remains provisional pending callback audit.

Two stale regression expectations were corrected during the Test137 close: the long combat simulation now accepts the newer valid fighter states through `ST_GROUND_PICKUP`, and ringside punch tests now use the recovered 3-point arcade Punch damage instead of the obsolete provisional 20-point value.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, entrant callout, human elimination and final-survivor resolution.

## Highest-priority open work

- Continue CPU behavior recovery from ROM without inventing Difficulty speed/aggression multipliers. Exact ground-choice probabilities and remaining raw state-2 semantics are still incomplete.
- Recover fatigue/tired-walk and stationary exhausted/stunned presentation seen in the arcade.
- Complete roll/get-up animation fidelity after knockdown and refine prompt artwork/palette/cadence.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation.
- Royal exact entrant artwork/path/timing, full 12-wrestler assets including Hawk/Animal, and additional elimination forms.
- TAG and Royal victory/defeat staging and animations.
- Mode-select glow, records, portrait/HUD palette fidelity, attract/promo scenes and audio.
- Ring-out/ringside hardware validation in the next major Saturn build.

## Provisional

- Native RNG/entropy where the original arcade source is not yet recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw→Saturn coordinate/scale transform, including reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Ground PICK UP completion timing/native state translation.
- Exact per-hold submission drain/escape timing.
- Earlier Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test138 / V0.153**. First recover fatigue/exhaustion presentation and remaining ground-choice probabilities from ROM, then continue victory/defeat staging and exact TAG/apron work. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
