# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test140 / V0.155** remains the canonical validated checkpoint.
- Full host regression at canonical validation: **136/136 binaries PASS**.
- Tests138-140 targeted ASan/UBSan: **PASS**.
- Latest hardware BIN/CUE remains **Test100 / V0.115**; confirmed working on real NTSC-U Saturn + Terraonion MODE.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test140-V0155-source.zip`
- Canonical snapshot SHA256: `4f30e2d92545a263f206f72564562c301b5eaebd55d4d297f84d5326ca6aa8bf`

## Current continuation WIP

A clean merged continuation now exists on top of Test140:

- `/WWF-Saturn/WWF-Saturn-Test143-MERGED-WIP-from-Test140.zip`
- SHA256 `f8513c20569f909fcd2b4cc56de52d0a1709dc2554a62cc60d6afe17cc831e1f`
- Supersedes the older divergent `WWF-Saturn-Test143-WIP-from-Test137.zip`.
- Targeted normal and ASan/UBSan builds PASS for Test141/142 and for canonical Test138-140 regression guards.
- Full host-suite promotion is still pending because the monolithic suite exceeded this runtime execution window; SH-2/Yaul compile/link is also pending because that toolchain is not installed in the current runtime.
- Therefore Test143 is **WIP, not canonical**.

## Canonical lineage

Test140 continues the canonical Test102→Test114→Test118→Test119→Test130→Test133→Test137→Test140 line. Earlier divergent working snapshots remain non-canonical.

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

## Tests134-137 — combat feedback and visible recovery

- Test134: arcade stationary Punch action `$00` callback writes exact **3-point stamina damage**, replacing the old provisional 20-point value for arcade-motion fighters.
- Test135: player-victim submission exposes the live rapid-button prompt; fresh Punch/Kick edges feed the existing escape counter.
- Test136: `ST_GROUND_ATTACK` renders exact catalogued ground-drop descriptors: STOMP `$23->$15CE0`, ELBOW DROP `$24->$1603A`, KNEE DROP `$25->$16116`, FIST DROP `$26->$1623C`.
- Test137: CPU ground policy can choose arcade action `$22` PICK UP / descriptor `$15B6C`, returning a prone live opponent toward the rise path.

## Tests138-140 — ROM-exact stamina-tier AI data and low-stamina boundary

- **Test138:** branch `$1D870` uses wrestler-indexed pointer table `$23856`, then actor `+$70 * 2`, then weighted selector `$24CC`. All **12 × 3** rows verified directly from ROM. First-bucket weights are **50 / 30 / 10** for tiers 0/1/2; every two-way row sums to 100. Surrounding branch semantic remains intentionally unnamed.
- **Test139:** branch `$1EDE0` selects one of ten pointers at `$1EE38`, then actor `+$70 * 2`, then `$24CC`. All **10 × 3** rows verified; every two-way row sums to 100. Native helpers expose the exact weights without inventing an aggression/cadence interpretation.
- **Test140:** ROM `$906E-$9085` locks the low-stamina warning boundary: active actor required, warning only when `+$66 < $0018`; stamina **24 is not low**, 23 and below are. The following global `$1C0166` latch test confirms alert gating.

## Test141-143 merged WIP

- **Test141:** `ST_RISE` renderer maps role-specific `$181FC/$181EC` for the first 26 ticks, then `$11E36` for the final 17 ticks, matching recovered 13+13+17 rise timing. Targeted normal + ASan/UBSan PASS.
- **Test142:** first native TAG/Rumble post-result staging uses the already-resolved match winner/loser ownership. Ownership is exact; Saturn positions/speed remain **PROVISIONAL** pending arcade post-match coordinate recovery. Targeted normal + ASan/UBSan PASS.
- **Test143:** result rendering no longer covers the centre of the ring during post-match staging; result text moves to a compact top strip. Exact arcade typography/layout remains **PROVISIONAL**.
- Merge integrity: canonical Test138-140 implementations were retained. The older divergent WIP stamina-bucket helper was not allowed to replace the validated `$23856/$1EE38` table work.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, entrant callout, human elimination and final-survivor resolution.

## Highest-priority open work

- Close Test141-143 WIP with full host regression in bounded batches and SH-2/Yaul compile/link before canonical promotion.
- Recover exact arcade victory/defeat/winner clap/celebration descriptors, positions and timing for TAG and Royal.
- Continue decoding the semantics around `$23856` and `$1EE38` so exact stamina-tier tables affect live CPU behavior only where proven.
- Recover exact ground-choice probabilities and remaining raw state-2 semantics.
- Complete roll/get-up fidelity and prompt artwork/palette/cadence.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation.
- Royal exact entrant artwork/path/timing, full 12-wrestler assets including Hawk/Animal, additional elimination forms.
- Mode-select glow, records, portrait/HUD palette fidelity, attract/promo scenes and audio.
- Ring-out/ringside hardware validation in the next major Saturn build.

## Provisional

- Native RNG/entropy where original arcade source is not yet recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw→Saturn coordinate/scale transform, including reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Ground PICK UP completion timing/native state translation.
- Exact per-hold submission drain/escape timing.
- Test142 post-match coordinates/speed and Test143 typography/layout.
- Earlier Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from the **merged Test143 WIP**, but do not promote it until full host + SH-2 validation succeeds. Then proceed to Test144+ with arcade victory/defeat animation recovery and the stamina-tier AI semantic route. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
