# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test140 / V0.155**
- Full host regression: **136/136 binaries PASS**, rerun in bounded parallel batches.
- Tests138-140 targeted ASan/UBSan: **PASS**.
- Latest hardware BIN/CUE remains **Test100 / V0.115**; no new disc image was generated.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test140-V0155-source.zip`
- Snapshot SHA256: `4f30e2d92545a263f206f72564562c301b5eaebd55d4d297f84d5326ca6aa8bf`
- Test140 package: 431 source/data/report files, CRC-checked ZIP, no executable/disc images.

## Canonical lineage

Test140 continues the canonical Test102→Test114→Test118→Test119→Test130→Test133→Test137 line. Earlier divergent working snapshots remain non-canonical.

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

- Test134: arcade stationary Punch action `$00` callback writes exact **3-point stamina damage**, replacing the old provisional 20-point native value for arcade-motion fighters.
- Test135: player-victim submission exposes the live rapid-button prompt; fresh Punch/Kick edges feed the existing escape counter.
- Test136: `ST_GROUND_ATTACK` renders exact catalogued ground-drop descriptors: STOMP `$23->$15CE0`, ELBOW DROP `$24->$1603A`, KNEE DROP `$25->$16116`, FIST DROP `$26->$1623C`.
- Test137: CPU ground policy can choose arcade action `$22` PICK UP / descriptor `$15B6C`, returning a prone live opponent toward the rise path.

## Tests138-140 — ROM-exact stamina-tier AI data and low-stamina boundary

- **Test138:** branch `$1D870` uses wrestler-indexed pointer table `$23856`, then actor `+$70 * 2`, then weighted selector `$24CC`. All **12 × 3** rows were verified directly from ROM. First-bucket weights are exactly **50 / 30 / 10** for stamina tiers 0/1/2; every two-way row sums to 100. The surrounding branch semantic remains deliberately unnamed until fully decoded.
- **Test139:** branch `$1EDE0` selects one of ten pointers at `$1EE38`, then actor `+$70 * 2`, then `$24CC`. All **10 × 3** rows were verified and every two-way row sums to 100. Native helpers expose the exact weights without inventing an aggression/cadence interpretation.
- **Test140:** ROM `$906E-$9085` locks the low-stamina warning boundary: active actor required, warning only when `+$66 < $0018`; stamina **24 is not low**, 23 and below are. The following global `$1C0166` latch test confirms the alert is gated rather than emitted every frame.

These checkpoints replace more guessed stamina-tier assumptions with exact ROM data while intentionally refusing to map a recovered probability table to a gameplay semantic until its surrounding handler is sufficiently decoded.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, entrant callout, human elimination and final-survivor resolution.

## Highest-priority open work

- Recover the exact fatigue/tired-walk and stationary exhausted/stunned presentation. No animation descriptor will be labelled fatigue purely by visual plausibility.
- Continue decoding the surrounding semantics of the `$23856` and `$1EE38` stamina-tier weighted branches so exact tables can replace provisional live CPU choices safely.
- Recover exact ground-choice probabilities and remaining raw state-2 semantics.
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

Continue from **Test141 / V0.156**. First finish fatigue/exhaustion descriptor/state recovery and surrounding semantics of the two recovered stamina-tier AI tables, then continue exact ground policy, victory/defeat staging and TAG/apron work. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
