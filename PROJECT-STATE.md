# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test150 / V0.165** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test150-V0165-source.zip`
- Source SHA256: `34e3b712696b2bf7959bf33cc00d7c18987f3e4145bb7cd55406e8803025f4e0`
- Host regression at Test150: **144/144 PASS**.
- Tests148-150 targeted ASan/UBSan: **PASS**.
- SH-2/Yaul compile + link: **not rerun in the Test150 runtime**; Test147/V0.162 remained the latest SH-2/Yaul compile+link **PASS** before these header/test-only changes.
- No new BIN/CUE was generated for Tests145-150.

## Hardware image status

Test144/V0.159 remains the current hardware-validation image and is still pending physical validation on Saturn. Test100/V0.115 remains the latest image confirmed working on NTSC-U Saturn + Terraonion MODE until the user physically tests Test144.

## Canonical lineage

Test102 → Test114 → Test118 → Test119 → Test130 → Test133 → Test137 → Tests138-143 reconciled WIP → Test144 → Test145 → Test146 → Test147 → Test148 → Test149 → **Test150**.

The separate historical Test140/V0.155 line and later merged-from-Test140 Test143 WIP remain non-canonical research archives. Useful evidence may be revalidated under new future Test numbers; canonical numbering must not be overwritten.

## Tests138-144 — preserved closure

- Test138 ROM-EXACT: stamina `+$66` → bucket `+$BD`: `>=128→0`, `101..127→1`, `80..100→2`, `64..79→3`, `37..63→4`, `1..36→5`, `0→8`.
- Test139 ROM-EXACT: `$1E45C-$1E464` compares raw `+$109 == +$BD`; semantics of `+$109` remain unclaimed.
- Test140 ROM-EXACT audit: explicit `+$109` read `$1E45C`, increment `$2006C`, compare sites around `$21282/$2128A`.
- Test141 ROM-EXACT: `ST_RISE` uses `$181FC/$181EC` for 13+13 ticks then `$11E36` for 17 ticks.
- Test142: TAG/Royal winner/loser ownership exact from match result; Saturn staging coordinates/speeds remain PROVISIONAL.
- Test143: centre result plate removed so ring/post-match actors remain visible; exact arcade result typography/layout REGISTERED BUT NOT IMPLEMENTED.
- Test144 ROM-EXACT: when `+$109 == +$BD`, recovered branch clears raw `+$20` low byte and writes raw `+$60=$004B` for `+$61==$4A/$64`, otherwise `$0056`; mismatch leaves tested outputs unchanged. This proves a stamina-derived raw-state consequence but not a fatigue/aggression/cadence interpretation.

## Tests145-150 — Signature / Super Throws fidelity

- **Test145 ROM-EXACT operand:** Sgt. Slaughter callback `$17ECE` and Ted DiBiase callback `$179C0` each write `#$0005` to victim `+$68` immediately before `JSR $111C8`. This is an exact per-pulse operand, not proof of total hold damage or cadence.
- **Test146 fidelity guard:** Earthquake's old candidate used raw action `$23` / descriptor `$15CE0`, but Test136 independently proved `$23->$15CE0` is STOMP. Gameplay readiness is therefore restricted to HIGH_CONFIDENCE records; the contradictory Earthquake candidate is gated off rather than fabricated.
- **Test147 ROM-EXACT route:** Earthquake selection character 8, prone category `$07`, fresh physical button 1 resolves raw action `$47`, descriptor `$14440`, callback `$14458`, first recovered pose 132. Move name/complete semantics remain CANDIDATE.
- **Test148 ROM-EXACT callback prelude:** `$14458` first tests actor `+$60 bit7` and `+$1C bit7`. If neither early branch is taken, it clears actor `+$01`, writes `+$5C=$0600`, `+$5E=$0048`, and chooses D0=5 only for target `+$21==$05 && +$61==$61`, otherwise D0=2, before `JSR $26AE`.
- **Test149 ROM-EXACT semantic correction:** `$26AE` starts `LEA $275C,A1; LSL.w #2,D0; MOVE.w (A1,D0.w),D4; MOVE.w 2(A1,D0.w),D5`. Therefore D0 values 2/5 are **4-byte table selectors, not damage values**.
- **Test150 ROM-EXACT selected table entries:** selector 2 resolves raw pair `{$0038,$FFF8}` and selector 5 resolves `{$0040,$FFFF}`. These remain raw effect/vector evidence; no unsupported gameplay-semantic label is assigned.

## Signature / Super Throws status

The source contains overlay/state-machine routes for ten trademark moves represented in `finisher_data.h` / `finisher_assets.h`, but per-record evidence flags remain authoritative. Earthquake is deliberately not promoted to gameplay-ready until its action-$47 composition, callback consequence and move identity are closed without contradiction.

The target **All Super Throws / Signature Finisher Moves** remains **REGISTERED BUT NOT FULLY IMPLEMENTED/PROVEN**.

## Preserved implemented systems

- Arcade HUD/stamina model, CPU stamina progression, ten-stage TAG campaign.
- Victory/defeat/Continue flow, identity-persistent stamina.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out.
- Manual TAG using the original two-button Kick route; TAG reserve HUD/stamina and staged four-wrestler presentation.
- Ringside object pickup/use path, ROM nearest-object seeking and opponent-distance gate, exact pickup/use clocks, state-2/state-4 lifecycle evidence.
- Royal Rumble six-slot opening, replacement lifecycle, entrant queue/callout, human elimination and final-survivor resolution.
- Difficulty raw source and wrestler-specific weighted AI tables.
- Visible submission/recovery prompts and exact rise descriptor sequence.

## Provisional

- Native RNG/entropy where arcade source is not yet recovered.
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
- Completion/proof of all Super Throws / Signature Finisher Moves.
- Exact TAG apron placement/entry/exit lifecycle.
- Remaining Royal entrant artwork/path/timing, full Hawk/Animal asset integration and additional elimination forms.
- Mode-select glow, records, attract/promo scenes and audio.

## Next priority

Continue from **Test151 / V0.166**. Primary fronts:

1. Continue Earthquake action `$47` from callback `$14458` until composition and effect consequences are proven; do not infer damage from selector values.
2. Recover exact TAG/Royal victory, defeat and winner-clap descriptors/states/coordinates/timing.
3. Continue the `+$66 -> +$BD / +$109 -> +$20,+$60` route until gameplay semantics are demonstrated before wiring any CPU stamina-tier behavior.
4. Complete/prove all remaining Signature / Super Throws entry conditions, callbacks, damage/effect operands and compositions.
5. Revalidate useful evidence from the non-canonical Test140 research archive under new future Test numbers before integration.

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence.