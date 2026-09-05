# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test160 / V0.175** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test160-V0175-source.zip`
- Source SHA256: `4c3089d899f319c92afedd10ac9b8223656659f5269bb420801d68c8483ababb`
- Complete host regression at Test160: **152/152 PASS** (parallel execution of the same source matrix as `tools/check_host.py`).
- Tests151-160 targeted ASan/UBSan: **PASS**.
- Tests151-160 are ROM-audit/test/documentation-only and do not modify runtime C/SH-2 code; Test147/V0.162 remains the latest SH-2/Yaul compile+link **PASS**.
- No new BIN/CUE was generated for Tests145-160.

## Hardware image status

Test144/V0.159 remains the current hardware-validation image and is still pending physical validation on Saturn. Test100/V0.115 remains the latest image confirmed working on NTSC-U Saturn + Terraonion MODE until the user physically tests Test144.

## Canonical lineage

Test102 → Test114 → Test118 → Test119 → Test130 → Test133 → Test137 → Tests138-143 reconciled WIP → Test144 → Test145 → Test146 → Test147 → Test148 → Test149 → Test150 → **Test160**.

The separate historical Test140/V0.155 line and later merged-from-Test140 Test143 WIP remain non-canonical research archives. Useful evidence may be revalidated under new future Test numbers; canonical numbering must not be overwritten.

## Tests138-144 — stamina/raw-state closure

- Test138 ROM-EXACT: stamina `+$66` → bucket `+$BD`: `>=128→0`, `101..127→1`, `80..100→2`, `64..79→3`, `37..63→4`, `1..36→5`, `0→8`.
- Test139 ROM-EXACT: `$1E45C-$1E464` compares raw `+$109 == +$BD`; semantics of `+$109` remain unclaimed.
- Test140 ROM-EXACT audit: explicit `+$109` read `$1E45C`, increment `$2006C`, compare sites around `$21282/$2128A`.
- Test141 ROM-EXACT: `ST_RISE` uses `$181FC/$181EC` for 13+13 ticks then `$11E36` for 17 ticks.
- Test142: TAG/Royal winner/loser ownership exact from match result; Saturn staging coordinates/speeds remain PROVISIONAL.
- Test143: centre result plate removed so ring/post-match actors remain visible; exact arcade result typography/layout REGISTERED BUT NOT IMPLEMENTED.
- Test144 ROM-EXACT: when `+$109 == +$BD`, recovered branch clears raw `+$20` low byte and writes raw `+$60=$004B` for `+$61==$4A/$64`, otherwise `$0056`; mismatch leaves tested outputs unchanged. This proves a stamina-derived raw-state consequence but not a fatigue/aggression/cadence interpretation.

## Tests145-160 — Signature / Super Throws fidelity

- Test145 ROM-EXACT operand: Sgt. Slaughter callback `$17ECE` and Ted DiBiase callback `$179C0` each write `#$0005` to victim `+$68` immediately before `JSR $111C8`; exact per-pulse operand only.
- Test146 fidelity guard: Earthquake's old `$23/$15CE0` candidate is contradictory because `$23->$15CE0` is independently proven STOMP; candidate is gated off.
- Test147 ROM-EXACT route: Earthquake selection character 8, prone category `$07`, fresh physical button 1 resolves raw action `$47`, descriptor `$14440`, callback `$14458`, first recovered pose 132. Human-readable move identity remains CANDIDATE.
- Test148 ROM-EXACT callback prelude: actor `+$60 bit7` or `+$1C bit7` early-branches; otherwise clear `+$01`, write `+$5C=$0600`, `+$5E=$0048`, choose D0=5 only for target `+$21==$05 && +$61==$61`, else D0=2, then `JSR $26AE`.
- Test149 ROM-EXACT: `$26AE` consumes D0 as a four-byte table selector, not damage.
- Test150 ROM-EXACT: selector 2 resolves `{$0038,$FFF8}`, selector 5 resolves `{$0040,$FFFF}`.
- **Test151 ROM-EXACT:** `$26AE` indexes table `$275C` by `D0<<2`; target `+$2E bit7` negates horizontal table offset D4.
- **Test152 ROM-EXACT:** `$26AE` derives its raw trajectory divisor from unsigned actor `+$5C / +$5E`, masks to 16 bits, doubles it, and exits if zero.
- **Test153 ROM-EXACT:** actor `+$58/+5A` are computed from `((target coordinate + table offset - actor coordinate)<<8)/divisor` for X/Y.
- **Test154 ROM-EXACT:** raw velocity clamps are X `[-$0200,+$0200]`, Y `[-$00C0,+$00C0]`.
- **Test155 ROM-EXACT:** action `$47` writes actor `+$2E` from the inverted sign bit of computed `+$58` immediately after `$26AE`.
- **Test156 ROM-EXACT:** first resolved action-$47 contact branch locks actor to target Y-1, sets actor `+$60 bit7`, target `+$20=4`, decrements target `+$64` by 2, increments target `+$C7`, and writes target `+$68=10`.
- **Test157 ROM-EXACT:** alternate resolved contact branch locks actor to target Y-1, writes actor `+$1B=$0C`, target `+$20=5,+$60=$77,+$68=10`.
- **Test158 ROM-EXACT:** fallback writes actor `+$20=4,+$64=5,+$68=3`, triggers raw effect/sound `$32`, writes global `$1C15D2=$0F1A`, sets actor `+$26 bit7` and flips facing `+$2E bit7`.
- **Test159 ROM-EXACT:** recovered `+$109` producer state has one-time `+$1C bit7` init: clears raw global `$1C169A/+24`, seeds `+$54=$3162`, copies linked facing, sets `+$23=$14,+$05=4`.
- **Test160 ROM-EXACT:** producer countdown `+$23` expiry increments phase `+$25` and target `+$109` together; phase 7 exits through terminal raw-state branch at `$2012E`.

The action-$47 composition/effect path is now substantially tighter, but its exact trademark-move name and complete semantic interpretation remain unclaimed. `+$109` is now proven to advance in lockstep with a seven-phase producer state, but a high-level gameplay label is still not assigned.

## Preserved implemented systems

- Arcade HUD/stamina model, CPU stamina progression, ten-stage TAG campaign.
- Victory/defeat/Continue flow, identity-persistent stamina.
- CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out.
- Manual TAG using original two-button Kick route; TAG reserve HUD/stamina and staged four-wrestler presentation.
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

Continue from **Test161 / V0.176**. Primary fronts:

1. Trace the seven-phase `+$109` producer around `$20060-$20154` and its callers until the actor/target gameplay context is demonstrated, then reconcile it with `+$BD` before any live stamina-tier CPU integration.
2. Continue Earthquake action `$47` through its contact/completion branches and independently identify the trademark move before promotion to gameplay-ready.
3. Recover exact TAG/Royal victory, defeat and winner-clap descriptors/states/coordinates/timing.
4. Complete/prove remaining Signature / Super Throws entry conditions, callbacks, raw effect/damage operands and compositions.
5. Revalidate useful evidence from the non-canonical Test140 research archive under new future Test numbers before integration.

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence.