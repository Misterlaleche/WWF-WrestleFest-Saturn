# WWF WrestleFest Saturn — private development repository

Private working state for the native Sega Saturn port/reimplementation.

## Current validated checkpoint

- **Test102 / V0.117**
- Host regression: **98/98 host binaries PASS** (split into bounded chunks to avoid runner timeout).
- ASan/UBSan: new Test101/Test102 regressions PASS.
- Persistent source snapshot: `/WWF-Saturn/WWF-Saturn-Test102-V0117-source.zip`
- Source snapshot SHA256: `5576a4a22bb8ee957f2fa4ff80124d285f910abe0a5a6147affd55bee3ea539f`
- Source manifest: 377 files plus `SOURCE-SHA256.txt` verification.
- Test100 remains the latest Saturn hardware BIN/CUE and is confirmed booting/running on Sega Saturn NTSC-U + Terraonion MODE.

## Tests101-102

### Test101 / V0.116 — live Rumble stamina recovery active gate

ROM `$8EE0` first executes `BTST #7,+$00(A0)` before natural stamina recovery. The native Rumble recovery loop now uses `active_mask` as the live equivalent of that active/eliminated ownership bit: eliminated actors clear the recovery accumulator and cannot regain stamina. The already recovered raw `$1D/$61` exclusions remain preserved separately; native states without a proven raw-state mapping are not fabricated. See `docs/TEST101-V0116.md`.

### Test102 / V0.117 — original two-button manual TAG

Test100 hardware exposed that manual TAG was effectively unavailable because the native path depended on Saturn C. The arcade cabinet has only Punch/Kick; evidence documents normal corner tagging with Kick. Arcade Mode now accepts a fresh **Kick** edge at the player partner corner as the standard legal-wrestler swap. Saturn C remains an optional convenience alias, and a KO reserve cannot be tagged in. The special headlock-tag double-team is registered but not claimed implemented. Current native corner coordinate conversion remains provisional. See `docs/TEST102-V0117.md`.

## Test100 Saturn hardware baseline

The Test100 / V0.115 BIN/CUE built with Yaul is confirmed to boot and run on real Sega Saturn NTSC-U + Terraonion MODE. Full hardware observations remain canonical in `docs/HARDWARE-TEST100-FEEDBACK.md`.

## Major integrated state through Test102

- Arcade HUD/stamina model, CPU stamina progression, ten-stage TAG campaign, Continue flow, persistent player stamina, CPU/apron partner logic, Power Up, grapple rota, submissions, ground actions and exact 20-count count-out are integrated to the demonstrated level.
- Ringside pickup/use, CPU weapon seeking/use, ROM-backed release geometry, state-2 collision work and state-4 reavailability lifecycle are integrated to the demonstrated level.
- Two-button physical action layer, contextual action matrix, running counters, behind actions, contextual pin and mapped trademark finishers are integrated; some non-grapple opportunity frequency remains provisional.
- Raw paired-state/category evidence through Test94, live Sleeper/Restrain behavior through Test95, and first Restrain partner-assist lifecycle through Test99 remain integrated with previously documented provisional timing/damage limits.
- Test101 closes the proven ROM active-actor exclusion in live Rumble natural stamina recovery.
- Test102 makes standard manual TAG usable through the original two-button control model instead of requiring a Saturn-only third button.

## Highest-priority confirmed gaps / defects

- CPU is still disproportionately difficult and player stamina still drains too fast on Test100 hardware; exact damage/decision-frequency rules require further ROM recovery rather than arbitrary global weakening.
- Reserve/apron placement and pin-assist entry/exit remain visually inaccurate.
- TAG pre-match presentation timing/poses/staging, referee tracking, rope deformation, pin 1/2/3 indicator, post-match staging, mode-select glow, instant character-select page transition, TAG HUD/palettes/portrait corrections and broader opponent selection remain incomplete.
- Ring-out/ringside gameplay remains hardware-validation pending.
- Royal Rumble formation, player marker, richer eliminations, timed entrants, ring entry and entrant announcement remain incomplete.
- Records and additional attract/presentation scenes including Legion of Doom material remain missing.

## Provisional

- Native deterministic RNG feed used by the Saturn CPU weapon branch; ROM-proven bounds/weighting are exact.
- Exact initial ringside-object coordinate conversion into the Saturn world model.
- CPU priority/frequency for non-grapple trademark opportunities.
- Translation of some raw paired actor states into native Saturn states.
- Restrain native timeout/release behavior where callback semantics remain incomplete.
- Test96-99 partner-assist trajectory/timing and current 8-point assist damage.
- Absolute Saturn placement/scale corresponding to recovered raw active/reserve coordinates.
- Native corner-coordinate transform used by the standard TAG proximity gate.

## Registered but not yet exact/complete

- Exact ROM-side entry/lifecycle for all three documented TAG double-team forms, including the headlock-tag form.
- Exact semantic/action callback mapping for all paired category `$09` actions `$15/$17`.
- Full live mapping of raw pair fields/states.
- Remaining LOD assets/behaviour, records, full Continue/Buy-In/Regain Power presentation, stages/cage, richer Rumble eliminations, attract/presentation and audio.
- Remaining Test100 hardware-feedback backlog.

## Recovery / lineage note

Historical incomplete/obsolete snapshots remain superseded. Test100 was rebuilt from its complete persistent source. Test102 is now the canonical persistent source checkpoint and includes the updated source manifest and regression additions.

## Next priority

Continue from **Test103 / V0.118**. Recover CPU damage/decision/stamina rules from ROM to address the Test100 difficulty complaint without inventing a rebalance, then continue core TAG active/reserve lifecycle and presentation/referee/rope/knockdown/HUD fidelity, followed by Royal Rumble progression. Preserve evidence labels and concrete non-empty checkpoints toward Test200. Do not generate another hardware BIN/CUE unless a Saturn-only blocker appears or the long integration checkpoint is reached.
