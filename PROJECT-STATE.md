# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test171 / V0.186** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test171-V0186-source.zip`
- Source SHA256: `0dda9d84fc51c5d71b69344502548e54aec566f0785c058f6cfd924dc80d996e`
- Complete host regression at Test171: **163/163 PASS** (parallel execution of the same source matrix as `tools/check_host.py`).
- Tests164-171 targeted ASan/UBSan: **PASS**.
- Tests164-171 are ROM-audit/test/documentation-only and do not modify runtime C/SH-2 code; **Test147 / V0.162** remains the latest SH-2/Yaul compile+link PASS.
- No new BIN/CUE was generated after Test144.

## Hardware image status

Test144/V0.159 remains the current hardware-validation image and is pending physical validation on Saturn. Test100/V0.115 remains the latest image confirmed working on real NTSC-U Saturn + Terraonion MODE until Test144 is physically tested.

## Canonical lineage

Test102 → Test114 → Test118 → Test119 → Test130 → Test133 → Test137 → Tests138-143 reconciled WIP → Test144 → Test145 → Test146 → Test147 → Test148 → Test149 → Test150 → Test160 → Test163 → **Test171**.

The separate historical Test140/V0.155 line and merged-from-Test140 Test143 WIP remain non-canonical research archives. Useful evidence may be revalidated under new future Test numbers; canonical numbering must not be overwritten.

## Tests138-144 — stamina/raw-state closure

- **Test138 ROM-EXACT:** stamina `+$66` → bucket `+$BD`: `>=128→0`, `101..127→1`, `80..100→2`, `64..79→3`, `37..63→4`, `1..36→5`, `0→8`.
- **Test139 ROM-EXACT:** `$1E45C-$1E464` compares raw `+$109 == +$BD`; semantics of `+$109` remain unclaimed.
- **Test140 ROM-EXACT audit:** explicit `+$109` read `$1E45C`, increment `$2006C`, compare sites around `$21282/$2128A`.
- **Test141 ROM-EXACT:** `ST_RISE` uses `$181FC/$181EC` for 13+13 ticks then `$11E36` for 17 ticks.
- **Test142:** TAG/Royal winner/loser ownership exact from match result; Saturn staging coordinates/speeds remain **PROVISIONAL**.
- **Test143:** centre result plate removed so ring/post-match actors remain visible; exact arcade result typography/layout **REGISTERED BUT NOT IMPLEMENTED**.
- **Test144 ROM-EXACT:** when `+$109 == +$BD`, recovered branch clears raw `+$20` low byte and writes raw `+$60=$004B` for `+$61==$4A/$64`, otherwise `$0056`; mismatch leaves tested outputs unchanged. This proves a stamina-derived raw-state consequence but not a fatigue/aggression/cadence interpretation.

## Tests145-160 — Signature / Super Throws fidelity and +$109 producer

- **Test145 ROM-EXACT:** Sgt. Slaughter callback `$17ECE` and Ted DiBiase callback `$179C0` each write `#$0005` to victim `+$68` immediately before `JSR $111C8`; exact per-pulse operand only.
- **Test146 fidelity guard:** Earthquake's old `$23/$15CE0` candidate is contradictory because `$23->$15CE0` is independently proven STOMP; candidate is gated off.
- **Test147 ROM-EXACT route:** Earthquake selection character 8, prone category `$07`, fresh physical button 1 resolves raw action `$47`, descriptor `$14440`, callback `$14458`, first recovered pose 132. Human-readable move identity remains CANDIDATE.
- **Test148 ROM-EXACT:** callback prelude checks actor `+$60 bit7` / `+$1C bit7`; otherwise clears `+$01`, writes `+$5C=$0600`, `+$5E=$0048`, selects D0=5 only for target `+$21==$05 && +$61==$61`, else D0=2, then calls `$26AE`.
- **Test149 ROM-EXACT:** `$26AE` consumes D0 as a four-byte table selector, not damage.
- **Test150 ROM-EXACT:** selector 2 resolves `{$0038,$FFF8}`, selector 5 resolves `{$0040,$FFFF}`.
- **Test151 ROM-EXACT:** `$26AE` indexes table `$275C` by `D0<<2`; target `+$2E bit7` negates horizontal table offset D4.
- **Test152 ROM-EXACT:** `$26AE` derives the raw trajectory divisor from unsigned actor `+$5C / +$5E`, masks to 16 bits, doubles it, and exits if zero.
- **Test153 ROM-EXACT:** actor `+$58/+5A` are computed from `((target coordinate + table offset - actor coordinate)<<8)/divisor` for X/Y.
- **Test154 ROM-EXACT:** raw velocity clamps are X `[-$0200,+$0200]`, Y `[-$00C0,+$00C0]`.
- **Test155 ROM-EXACT:** action `$47` writes actor `+$2E` from the inverted sign bit of computed `+$58` immediately after `$26AE`.
- **Test156 ROM-EXACT:** first resolved action-$47 contact branch locks actor to target Y-1, sets actor `+$60 bit7`, target `+$20=4`, decrements target `+$64` by 2, increments target `+$C7`, and writes target `+$68=10`.
- **Test157 ROM-EXACT:** alternate resolved contact branch locks actor to target Y-1, writes actor `+$1B=$0C`, target `+$20=5,+$60=$77,+$68=10`.
- **Test158 ROM-EXACT:** fallback writes actor `+$20=4,+$64=5,+$68=3`, triggers raw effect/sound `$32`, writes global `$1C15D2=$0F1A`, sets actor `+$26 bit7` and flips facing `+$2E bit7`.
- **Test159 ROM-EXACT:** recovered `+$109` producer state has one-time `+$1C bit7` init: clears raw global `$1C169A/+24`, seeds `+$54=$3162`, copies linked facing, sets `+$23=$14,+$05=4`.
- **Test160 ROM-EXACT:** producer countdown `+$23` expiry increments phase `+$25` and target `+$109` together; phase 7 exits through terminal raw-state branch at `$2012E`.

## Tests161-163 — parent/substate context for +$109

- **Test161 ROM-EXACT:** `$1FB04-$1FB18` indexes an 11-entry pointer table at `$1FB1A` directly from actor byte `+$1D`; table entry **6** is `$20022`. Therefore the recovered `+$109` producer is raw substate handler `+$1D==6`.
- **Test162 ROM-EXACT:** actor processing first dispatches raw `+$21` through the primary table at `$1F952`, then unconditionally calls `$1FAF0`, the secondary `+$1D` dispatcher.
- **Test163 ROM-EXACT:** before the `+$1D` dispatch, `$1FAF0` tests actor `+$20 bit7`; when set, it clears that bit and copies raw `+$20` into raw `+$1C`, then continues to the secondary dispatcher.

## Tests164-171 — linked-actor and mode context for +$109

- **Test164 ROM-EXACT:** `$20058` resolves `A1=A0+$56`, then `A2=A1+$26`; producer ticks local countdown on A0 while the incremented `+$109` belongs to second linked actor A2.
- **Test165 ROM-EXACT:** on expiry local phase `+$25` and linked `+$109` advance together; phase parity selects a distinct branch, and local state 5 promotes to state 6 with countdown `$38`.
- **Test166 ROM-EXACT:** phase 6 adds `$10` to countdown then splits on global mode bit `$1C0161`; the two branches are structurally distinct.
- **Test167 ROM-EXACT:** terminal handling is mode-dependent: clear `$1C0161` writes actor `+$20=$8009` and linked `+$4A=4`; set mode bit takes the alternate helper and writes actor `+$20=$8005`.
- **Test168 ROM-EXACT:** even producer phases increment global `$1C169A` and derive helper input from actor `+$24>>1`; `$1C169A` is independently recovered as ring-out/count storage.
- **Test169 ROM-EXACT:** clear-`$1C0161` (TAG) phase-6 path calls `$21358` on `+$56`, increments linked `+$C4`, follows `+$26`, and sets bit4 of that actor's `+$32`.
- **Test170 ROM-EXACT:** set-`$1C0161` (Royal Rumble) phase-6 path writes raw roles `$4000/$4001` across the recovered four-actor linked set, proving explicit role reassignment.
- **Test171 ROM-EXACT:** Royal phase-6 cleanup clears `$1C169E`, calls `$90D6`, then returns; `$90D6` is independently recovered as the clear-stage stamina restore routine.

The `+$109` route is now structurally tied to secondary actor state, a second linked actor, TAG/Royal mode branches, ring-out/count storage interaction, Royal role reassignment, and stamina-restore cleanup. This still does **not** prove that `+$109` itself means fatigue, aggression, recovery stage, cadence or another named mechanic; no live stamina-tier CPU behavior is promoted from it yet.

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
- Completion/proof of all Super Throws / Signature Finisher Moves, including Legion of Doom's team finisher.
- Exact TAG apron placement/entry/exit lifecycle.
- Remaining Royal entrant artwork/path/timing, full Hawk/Animal asset integration and additional elimination forms.
- Mode-select glow, records, attract/promo scenes and audio.

## Next priority

Continue from **Test172 / V0.187**. Primary fronts:

1. Continue tracing the linked actors and helper calls around `$2007A-$20154` until the gameplay meaning of `+$109` is actually demonstrated; only then reconcile it with `+$BD` for live stamina-tier CPU behavior.
2. Continue Earthquake action `$47` through contact/completion and independently identify the trademark move before promotion to gameplay-ready.
3. Recover exact TAG/Royal victory, defeat and winner-clap descriptors/states/coordinates/timing.
4. Complete/prove remaining Signature / Super Throws entry conditions, callbacks, raw effect/damage operands and compositions, including LOD.
5. Continue exact TAG apron/entry/exit, Royal assets/eliminations, records, attract/promo, presentation and audio work as evidence permits.

Continue concrete, non-empty checkpoints toward **Test200**. Do not claim arcade-exact behavior without ROM/manual evidence.