# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test144 / V0.159** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test144-V0159-source.zip`
- Source SHA256: `6ea546f185d71951596ca19419c7fbd50a2def0d24e0346ec91ca36350960eee`
- Source package: 441 source/data/report files, no executable/disc image except the original research `program.bin` ROM image already used by the project.
- Full host regression: **139/139 PASS**.
- Tests138-144 targeted ASan/UBSan: **PASS**.
- SH-2/Yaul compile + link: **PASS**, using the pinned project toolchain lineage from Test100.
- Yaul ISO structural validation: **PASS** (Saturn header, USA area, V0.159/title, ISO9660, 35 CD files and exact `A.BIN`).

## Test144 hardware image

The user explicitly requested a BIN/CUE after the next completed Test, so Test144 has a hardware-validation image:

- `/WWF-Saturn/WWF-WrestleFest-Test144-V0159-BIN-CUE.zip`
- Package SHA256: `b61ad9a53a3d159dbac9c34e4111f60f2aa6b11af5b98706fcd9f5c83f42646e`
- Raw BIN SHA256: `7610c9dfc015baa0573742286f2381d5b7b63bd2ccccfcb61844bd3c5199ac46`
- CUE SHA256: `bf558f9489edcd0809da6f884beddb070b1d4896eb7de4164d65f02c3400b1cd`
- Final disc format: single-track **MODE1/2352** BIN/CUE.
- Sector writer verification: all 2,214 sectors regenerated and compared; payload round-trip exact.
- Independent libcdio CUE read + bitwise EDC verification: **PASS**.
- **Real Saturn validation: PENDING.** Test100/V0.115 remains the latest image confirmed working on NTSC-U Saturn + Terraonion MODE until the user physically tests Test144.

## Canonical lineage and reconciled divergence

The user explicitly selected the Test137/V0.152 checkpoint plus `/WWF-Saturn/WWF-Saturn-Test143-WIP-from-Test137.zip` as the continuation to reconcile. That line is now closed through Test144:

Test102 → Test114 → Test118 → Test119 → Test130 → Test133 → Test137 → Tests138-143 reconciled WIP → **Test144**.

A separate Test140/V0.155 snapshot and a later Test143 merged-from-Test140 WIP were produced on a divergent numbering line. They are **non-canonical research archives**, not the active source baseline. Their useful `$23856`, `$1EE38` and low-stamina-boundary findings are not discarded; they must be revalidated and assigned new future Test numbers before integration so canonical Test138-140 meanings are not overwritten.

## Tests120-126 — Royal Rumble exact rules + live elimination route

- Test120: exact six raw opening anchors at ROM `$10ACC`.
- Test121: exact six-word reverse free-slot scan (`5 -> 0`) used by `$10AF0/$10B26`.
- Test122: exact post-elimination wait at `$BB30`: `$0126` = 294 ticks.
- Test123: `$BB80/$BC00` two-pool next-entrant structure, four random attempts then reverse fallback scan; exact RNG source/order remains isolated.
- Test124: exact 294-tick wait and six-slot ownership integrated into the canonical replacement lifecycle while preserving the visible `NEW CHALLENGER` phase.
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

- Test134: arcade stationary Punch action `$00` callback writes exact **3-point stamina damage**.
- Test135: player-victim submission exposes the live rapid-button prompt; fresh Punch/Kick edges feed the existing escape counter.
- Test136: `ST_GROUND_ATTACK` renders exact catalogued ground-drop descriptors: STOMP `$23->$15CE0`, ELBOW DROP `$24->$1603A`, KNEE DROP `$25->$16116`, FIST DROP `$26->$1623C`.
- Test137: CPU ground policy can choose arcade action `$22` PICK UP / descriptor `$15B6C`, returning a prone live opponent toward the rise path; completion timing/native state translation remains provisional.

## Tests138-140 — ROM-EXACT raw stamina route

- **Test138:** `$1E48C-$1E4D8` classifies actor stamina `+$66` into raw bucket `+$BD`: `>=128 -> 0`, `101..127 -> 1`, `80..100 -> 2`, `64..79 -> 3`, `37..63 -> 4`, `1..36 -> 5`, `0 -> 8`.
- **Test139:** `$1E45C-$1E464` compares actor `+$109` for equality with `+$BD`. Exact predicate; no fatigue/aggression/cadence semantic is assigned.
- **Test140:** bounded ROM audit pins the explicit `+$109` read at `$1E45C`, byte increment at `$2006C`, and direct compare sites around `$21282/$2128A`. Producer/consumer flow is proven; semantic name remains deliberately withheld.

## Test141 — ROM-EXACT rise presentation

- `ST_RISE` uses recovered role-specific descriptors `$181FC/$181EC` for the first 26 ticks (13+13), then `$11E36` for the final 17 ticks.

## Tests142-143 — post-match staging and ring visibility

- **Test142:** TAG/Royal winner/loser ownership comes from exact resolved match state. Saturn staging coordinates and movement speeds are **PROVISIONAL** pending recovery of the arcade routine.
- **Test143:** the old large opaque centre result plate is removed; a shallow top banner keeps the ring/post-match actors visible. Exact arcade result typography/layout is **REGISTERED BUT NOT IMPLEMENTED**.
- Exact victory clap, exact winner pose and exact defeated/sad pose remain **REGISTERED BUT NOT IMPLEMENTED**.

## Test144 — ROM-EXACT stamina/raw-109 state consequence

The recovered `$1E45C-$1E48A` branch is exposed as a raw helper. If `+$109 != +$BD`, tested outputs remain unchanged. On equality it:

1. clears the low byte of raw state `+$20`;
2. tests `+$61`;
3. writes raw `+$60=$004B` for `+$61==$4A` or `$64`;
4. otherwise writes raw `+$60=$0056`.

This proves that the recovered stamina bucket participates in a concrete raw state transition. It still does **not** prove that `+$109`, `+$60` or `+$61` means fatigue, tired walk, aggression, cadence or any similar gameplay concept.

## Signature / Super Throws status

The current source already contains overlay/state-machine routes for the ten trademark moves represented in `finisher_data.h`/`finisher_assets.h`. Those routes are implemented source functionality, but not every entry condition, callback, damage value or graphical variant is fully ROM-exact; their per-record evidence flags remain authoritative, and Earthquake retains candidate status where documented.

The project requirement **All Super Throws / Signature Finisher Moves** is **REGISTERED BUT NOT FULLY IMPLEMENTED/PROVEN** as a completeness target. Future work must recover wrestler → trigger/condition → raw action/state → descriptor/composition → impact/effect before upgrading each remaining route. No finisher is to be invented from visual plausibility or memory.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, entrant callout, human elimination and final-survivor resolution.

## Provisional

- Native RNG/entropy where original arcade source is not yet recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw→Saturn coordinate/scale transform, reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Ground PICK UP completion timing/native state translation.
- Exact per-hold submission drain/escape timing.
- Test142 post-match coordinates/speed.
- Earlier Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Registered but not implemented / not yet proven

- Exact TAG/Royal victory clap, winner pose, defeated/sad pose, coordinates and timing.
- Exact arcade result typography/layout.
- Gameplay-semantic interpretation of raw `+$109/+$60/+$61` and live CPU integration based on that interpretation.
- Completion/proof of **All Super Throws / Signature Finisher Moves**.
- Exact TAG apron placement/entry/exit lifecycle.
- Remaining Royal entrant artwork/path/timing, full Hawk/Animal asset integration and additional elimination forms.
- Mode-select glow, records, portrait/HUD palette fidelity, attract/promo scenes and audio.

## Next priority

Continue from **Test145 / V0.160**. Primary fronts:

1. Recover exact TAG/Royal victory, defeat and winner-clap descriptors/states/coordinates/timing.
2. Continue the `+$66 -> +$BD / +$109 -> +$20,+$60` route until gameplay semantics are demonstrated, then and only then wire those semantics into live CPU behavior.
3. Complete/prove **All Super Throws / Signature Finisher Moves** without inventing controls, damage, callbacks or compositions.
4. Revalidate useful evidence from the non-canonical Test140 research archive under new future Test numbers before any integration.

Test144 BIN/CUE has already been generated by explicit user request. Do not treat it as hardware-confirmed until the user tests it on real Saturn.
