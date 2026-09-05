# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and the persistent ChatGPT Library as source of truth.

## Current start point

1. Read `PROJECT-STATE.md` first.
2. Current validated canonical source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test160-V0175-source.zip`
   - SHA256 `4c3089d899f319c92afedd10ac9b8223656659f5269bb420801d68c8483ababb`
3. Continue from **Test161 / V0.176**. Do not resume from Test150 or older snapshots.
4. Complete host regression at Test160: **152/152 PASS**.
5. Targeted ASan/UBSan for Tests151-160: **PASS**.
6. Tests151-160 are ROM-audit/test/documentation-only; Test147/V0.162 remains the latest SH-2/Yaul compile+link PASS.
7. No new BIN/CUE was produced for Tests145-160.

## Hardware state

Test144/V0.159 remains the current pending hardware-validation image. Test100/V0.115 remains the latest image physically confirmed on NTSC-U Saturn + Terraonion MODE until the user tests Test144.

## Critical new evidence through Test160

- Earthquake selection character 8, prone category `$07`, fresh button1 resolves exact raw action `$47`, descriptor `$14440`, callback `$14458`; move identity remains CANDIDATE.
- `$14458` uses `$26AE` trajectory helper with D0 table selectors 2/5, not damage operands.
- `$26AE` table base is `$275C`, stride four bytes; selector 2 is `{$0038,$FFF8}`, selector 5 `{$0040,$FFFF}`.
- `$26AE` computes raw X/Y velocities from target coordinate plus table offset, with actor `+$5C/+5E` duration divisor; raw clamps are X ±$0200 and Y ±$00C0.
- Action `$47` contact/fallback raw-state consequences are pinned in Tests155-158; do not promote their high-level semantics without independent evidence.
- `+$109` producer state is now proven to initialize once via `+$1C bit7`, then increment target `+$109` in lockstep with its own phase `+$25`; phase 7 reaches a terminal branch. A gameplay-semantic label for `+$109` is still intentionally unclaimed.

## Fidelity rules

Every Test must implement or rigorously validate a real function. Preserve evidence labels: **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED**. Never promote a move, damage value, AI behavior, geometry, timing or presentation detail to arcade-exact from visual plausibility or memory.

## Immediate continuation

1. Trace the seven-phase `+$109` producer around `$20060-$20154` and its callers until its actor/target gameplay context is demonstrated; then reconcile it with the stamina bucket `+$BD` before wiring stamina-tier CPU behavior.
2. Continue Earthquake action `$47` through contact/completion and independently identify its trademark move before promotion.
3. Recover exact TAG/Royal victory, defeat and winner-clap/celebration descriptors/states/coordinates/timing.
4. Complete/prove all Super Throws / Signature Finisher Moves, respecting per-record evidence flags.
5. Revalidate useful evidence from the historical non-canonical Test140 research archive only under new future Test numbers.
6. Continue exact TAG apron/entry/exit, Royal assets/eliminations, records, attract/promo, presentation and audio work as evidence permits.

## Long target

Continue concrete checkpoints toward Test200. At genuine Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
