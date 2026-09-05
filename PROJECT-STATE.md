# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test200 / V0.215** is the canonical validated source checkpoint.
- Canonical source snapshot: `/WWF-Saturn/WWF-Saturn-Test200-V0215-source.zip`
- Source SHA256: `d4e55dc0ddc388b5842118ed459aa5cd35841888e219679b06cf9c530e59b9b2`
- Complete host regression: **192/192 PASS**.
- Targeted ASan/UBSan for Tests172-200: **29/29 PASS**.
- Tests172-199 are ROM-audit/test/documentation-only. Test200 rigorously reconciles the existing native clear-stage stamina runtime with newly pinned ROM `$90D6` semantics; it does not add an invented gameplay semantic.
- **Test147 / V0.162** remains the latest SH-2/Yaul compile+link PASS.
- No new BIN/CUE was generated after Test144.

## Hardware image status

Test144/V0.159 remains the current pending hardware-validation image. Test100/V0.115 remains the latest image physically confirmed working on real NTSC-U Saturn + Terraonion MODE until Test144 is physically tested.

## Test172-180 — linked actor / TAG / Royal cleanup

- **Test172 ROM-EXACT:** `$21358` preserves registers then sets actor `+$B6 bit7`.
- **Test173 ROM-EXACT:** a non-null `+$7E` link has `+$B5 bit7` cleared and `+$B6 bit7` set.
- **Test174 ROM-EXACT:** actor `+$61==$53` follows `+$26` and sets that actor `+$B6 bit7`.
- **Test175 ROM-EXACT:** `$206FE` begins a four-entry cleanup at `$0C0744`, zeroing four longwords per entry.
- **Test176 ROM-EXACT:** `$20156` selects raw effect `$3109` only for global `$1C0162==4/9`, otherwise `$3105`.
- **Test177 ROM-EXACT:** `$213A6` splits on the TAG/Royal mode bit; TAG follows `+$26` and clears linked `+$34 bit0`.
- **Test178 ROM-EXACT:** TAG cleanup tests linked `+$33 bit1` and clears `+$56 bit6` when set.
- **Test179 ROM-EXACT:** TAG cleanup follows `+$86`; when `+$33 bit1` is clear it clears `+$B5 bit7` and sets global `$1C1697 bit0`.
- **Test180 ROM-EXACT:** Royal cleanup follows non-null `+$7E` and clears its `+$B5 bit7`.

These facts materially tighten the linked-actor/role cleanup around the `+$109` producer. They still do **not** prove a human-readable semantic for `+$109`.

## Test181-188 — Earthquake raw action `$47`

- **Test181 ROM-EXACT:** phase 0 with zero `+$22` writes actor `+$01=2`.
- **Test182 ROM-EXACT:** action `$47` uses dedicated phase-1 sprite offsets `$F8/$F0`, distinct from neighbouring `$F0/$28`.
- **Test183 ROM-EXACT:** contact resolution requires actor `+$37 bit4`, sets `+$18 bit7`, then calls `$1110E/$11152`.
- **Test184 ROM-EXACT:** contact branch A writes victim state 4, decrements `+$64` by 2, increments `+$C7`, writes `+$68=10`, emits effects `$2B/$32`.
- **Test185 ROM-EXACT:** branch A increments victim `+$E0` only for neighbouring raw action `$46`; raw action `$47` takes `+$E2`.
- **Test186 ROM-EXACT:** contact branch B writes victim state 5, `+$60=$77`, `+$68=10`, increments `+$C7`.
- **Test187 ROM-EXACT:** phase `$FE` completes into actor state 7, sets `+$26 bit7`, calls `$10BD0` with `D0=$18,D1=-1,D2=0`.
- **Test188 ROM-EXACT:** the fallback extra `$FFD0` impulse is exclusive to neighbouring raw action `$46`, not `$47`.

Action `$47` is now mechanically bounded much further, but its trademark move identity remains **CANDIDATE** until independently demonstrated. It is not promoted to Vertical Splash from inference.

## Test189-195 — Signature / Super Throw callback operands

- **Test189 ROM-EXACT:** Warrior Gorilla Press has two impact paths and both write victim `+$68=$16` (22).
- **Test190 ROM-EXACT:** Jake DDT writes victim `+$64=$23`, sets bits5/3 and writes exact `+$68=$1C` (28).
- **Test191 ROM-EXACT:** Boss Man Slam writes victim `+$64=$2A`, increments `+$C7` by 5, writes exact `+$68=$16` (22), then sets bits3/5.
- **Test192 ROM-EXACT:** Smash Suplex writes victim `+$64=5`, exact `+$68=$12` (18), then sets `+$64 bit5` and `+$32 bit3`.
- **Test193 ROM-EXACT:** Perfect Plex contains two exact `+$68=$16` (22) victim writes; alternate branch also writes `+$60=$5E`.
- **Test194 ROM-EXACT:** Slaughter Cobra Clutch pulse is exact `+$68=5 -> $111C8`; alternate release writes victim `+$60=$62`, sets flags and invokes `$215B6`.
- **Test195 ROM-EXACT:** DiBiase Million Dollar Dream pulse is exact `+$68=5 -> $111C8`; alternate release writes victim `+$60=$63`, sets flags and invokes `$215B6`.

Hold pulse operands remain distinct from total hold damage/cadence.

## Test196-200 — exact clear-stage stamina restore

- **Test196 ROM-EXACT:** `$90D6` iterates exactly four actor records from `$1C05B0` at stride `$10C`, skipping records with `+$FF bit0` set.
- **Test197 ROM-EXACT:** restore amount comes from global config `$1C0066 bits $6000` through exact table `{24,32,12,0}` at `$912A`.
- **Test198 ROM-EXACT:** the same amount is added to live stamina `+$66` and queued stamina delta `+$6A`.
- **Test199 ROM-EXACT:** live `+$66` is clamped to actor maximum `+$72`; the separately queued `+$6A` addition is preserved.
- **Test200 VALIDATED RUNTIME:** native TAG `game_clear_stage_restore()` is regression-validated against those exact semantics: human-role exclusion, exact table, max clamp and queued stamina/HUD delta behavior.

## IMPLEMENTADO

- Native Saturn combat/runtime architecture with VDP1/VDP2-oriented source path and validated SH-2/Yaul build lineage through Test147.
- Arcade HUD/stamina model, exact human maxima, CPU stage/difficulty stamina progression, exact clear-stage restore table and Test200 runtime alignment.
- Ten-stage TAG campaign, opponent selection structure, victory/defeat/Continue flow and identity-persistent player stamina.
- CPU/apron partner logic, Power Up, grapple rota/counters, Irish whip/running paths, submissions, ground actions, rise/recovery prompts and exact 20-count count-out.
- Manual TAG on the original two-button Kick route; reserve HUD/stamina and four-wrestler TAG presentation framework.
- Ringside object pickup/use, nearest-object CPU seeking and opponent-distance gate, pickup/use clocks, state-2/state-4 lifecycle evidence and native object runtime.
- Royal Rumble opening/active-slot framework, replacement lifecycle, entrant queue/callout, human elimination and final-survivor resolution.
- Difficulty source and wrestler-specific weighted AI tables.
- Native finisher state-machine/overlay framework for the ten selectable wrestlers, with evidence gating; high-confidence routes remain playable while unsupported identities are blocked rather than fabricated.
- Exact callback impact operands now pinned for Warrior, Jake, Boss Man, Smash, Perfect, Slaughter and DiBiase as detailed above.
- Exact Rumble-only stamina recovery gate/exclusions already preserved from earlier checkpoints; clear-stage `$90D6` role exclusion/table/queue semantics are now separately closed through Test200.

## REGISTRADO PERO NO IMPLEMENTADO

- Human-readable gameplay semantic of raw `+$109/+$60/+$61` and therefore live CPU stamina-tier behavior based on that still-unproven semantic.
- Independent proof of Earthquake action `$47` as his trademark Vertical Splash; the route is mechanically recovered but identity remains CANDIDATE.
- Completion/proof of every Super Throw / Signature Finisher entry condition, callback composition and total hold cadence, especially Legion of Doom's team finisher.
- Full Hawk/Animal graphical/behavior integration.
- Exact TAG/Royal victory clap, winner pose, defeated/sad pose, coordinates and timing.
- Exact TAG apron placement/entry/exit lifecycle and all evidenced double-team forms.
- Remaining Royal entrant artwork/path/timing and additional elimination forms.
- Exact arcade result typography/layout, records presentation, mode-select glow, attract/promo scenes and full audio implementation.
- Remaining stage/scenario/cage completeness beyond the current ring/runtime framework.

## PROVISIONAL

- Native RNG/entropy where original arcade source has not yet been recovered.
- Rumble callout artwork/layout, native entry geometry and portions of entrant ordering.
- Raw-to-Saturn coordinate/scale transform, reserve visual correction and Rumble Body Slam rope threshold.
- Referee trajectory, TAG presentation timing, rope deflection curve and dedicated rope-entry mapping.
- Ground PICK UP completion timing/native state translation.
- Exact per-hold submission drain/escape timing and total hold damage cadence.
- Post-match staging coordinates/speed.
- Earlier Restrain native timing/assist path/damage and other pre-Test100 explicitly provisional items.

## PENDIENTE DE BUILD SATURN

- Test144/V0.159 is still the newest generated hardware image and awaits physical validation.
- Test100/V0.115 remains the latest image confirmed on real NTSC-U Saturn + Terraonion MODE.
- Tests145-200 have **not** produced a new BIN/CUE. A new Saturn integration image should be generated only after the user requests the post-Test200 hardware phase or when a Saturn-only blocker justifies it.

## Validation artifact

Detailed Tests172-200 evidence is preserved in `docs/TEST172-200-CLOSURE.md` inside the canonical Test200 source snapshot.