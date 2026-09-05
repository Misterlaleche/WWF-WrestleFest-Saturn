# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test133 / V0.148**
- Tests129-133 targeted host validation: **PASS**.
- Tests131-133 targeted ASan/UBSan: **PASS**.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test133-V0148-source.zip`
- Snapshot SHA256: `0ad549b82dded29efcfe4de0ce8ea55cef3ea61ed67180df70d8eeea7fd7044f`
- Snapshot contains 423 source/data/report files and no generated Saturn ISO/BIN/CUE products.
- Test100 remains the latest Saturn BIN/CUE and is confirmed booting/running on real NTSC-U Saturn + Terraonion MODE. No newer disc image was generated.

## Canonical lineage

Test133 continues the canonical Test102→Test114→Test118→Test119→Test130 line. Earlier divergent Test105/Test115/Test122 working snapshots are not canonical.

## Tests120-126 — Royal Rumble exact rules + live elimination route

- Test120: exact six raw Royal Rumble opening anchors at ROM `$10ACC`.
- Test121: exact six-word reverse free-slot scan (`5 -> 0`) used by `$10AF0/$10B26`.
- Test122: exact post-elimination wait at `$BB30`: `$0126` = 294 ticks.
- Test123: `$BB80/$BC00` two-pool next-entrant structure, four random attempts then reverse fallback scan; exact RNG source/order remains isolated.
- Test124: exact 294-tick wait and six-slot ownership integrated into the replacement lifecycle.
- Test125: human headlock composition can move as one unit.
- Test126: live headlock + rope + Punch Body Slam elimination for wrestlers whose arcade repertoire contains BODY SLAM; Saturn rope threshold remains provisional.

## Tests127-133 — Difficulty and wrestler-specific AI audit

- Test127: exact operator packing proves Difficulty comes from `$140022` bits 8-9 and becomes `$1C0066` bits 8-9 after inversion.
- Test128: complete direct-consumer audit finds Difficulty extracted at `$107F8` for CPU stamina setup and no direct Difficulty-bit aggression/cadence consumers.
- Test129: exact `$24CC` weighted selector after accepted random value normalization to 1..99.
- Test130: exact two 12-wrestler probability tables at `$1F3A6/$1F3BE`.
- **Test131 corrects Test130 control-flow prose:** raw target-distance `+$B0 < $C8` uses the first table; `$C8..$DF` is a deadband that returns without the weighted state-2 transition; `>= $E0` uses the second table. The table bytes themselves from Test130 were correct.
- Test132: a bucket-0 win writes raw actor state `+$20=$0002`, clears `+$B7` bit 1, derives `+$2D` bit 7 through `$1D7A0`; the far branch additionally sets `+$32` bit 2 and toggles `+$2D/+$2E` bit 7.
- Test133: CPU dispatcher `$1C1F4` maps raw state 2 to `$1D7B6`; the handler is target-relative, dereferencing `+$7A`, comparing actor/target `+$2E`, and consuming `+$B2/+$B0` thresholds. Its precise semantic label is not yet claimed.

## Recovery integrity fix at Test133

The Test130 Library snapshot accidentally omitted `arcade_ai.c/.h` even though the Makefile/tests referenced them. Test133 reconstructs the two small helpers from their already validated Test129/130 behavior, adds them to `tools/package_release.py`, and persists a self-contained source checkpoint.

## Preserved hardware-feedback work

- Manual TAG through original two-button Kick route.
- TAG reserve HUD/stamina, reserve visual correction, rope traversal mapping, staged four-wrestler presentation, referee tracking and visible pin count.
- Cached character-select pages and broader CPU roster selection.
- Knockdown rapid-button prompt and provisional rope deflection.
- Six-position Royal opening, `1P` marker, queue/replacement lifecycle, entrant callout, human elimination and final-survivor resolution.

## Highest-priority open work

- Continue decoding `$1D7B6..$1DA65` before assigning a semantic/native Saturn movement name to raw state 2.
- Audit player/CPU damage and recovery consumers. Exact Rumble recovery exclusions are ROM-proven, but the live native path still passes zero placeholders for raw `+$1D/+$61` state bytes where semantic mappings are not yet demonstrated; do not invent those mappings.
- Exact reserve/apron placement and dedicated partner rope-entry/exit animation.
- Missing roll/get-up animation fidelity after knockdown.
- Royal exact entrant artwork/path/timing, full 12-wrestler assets including Hawk/Animal, and additional elimination forms.
- TAG and Royal victory/defeat staging and animations.
- Mode-select glow, records, portrait/HUD palette fidelity and additional attract/promo scenes.
- Ring-out/ringside hardware validation and audio.

## Provisional

- Native RNG/entropy where the original arcade source is not yet recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw→Saturn coordinate/scale transform, including reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Earlier Restrain native timing/assist path/damage and other pre-Test100 provisional items remain provisional.

## Next priority

Continue from **Test134 / V0.149**. Decode more of the raw state-2 handler and its producer/consumer flags, then continue CPU damage/stamina/recovery auditing. Do not wire raw-distance percentages into live Saturn movement until the state semantics and coordinate translation are demonstrated. Do not generate another BIN/CUE until a major integration checkpoint or Saturn-only blocker warrants it.
