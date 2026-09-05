# New-chat handoff — WWF WrestleFest Saturn

Use this private repository and the persistent ChatGPT Library as source of truth.

## Current start point

1. Read `PROJECT-STATE.md` first.
2. Current validated canonical source snapshot:
   - `/WWF-Saturn/WWF-Saturn-Test163-V0178-source.zip`
   - SHA256 `593c8e255281e5f88a87700a4f420d3201537e255ca9a0ee27ca1a85309fcb42`
3. Continue from **Test164 / V0.179**. Do not resume from Test160 or older snapshots.
4. Complete host regression at Test163: **155/155 PASS**.
5. Targeted ASan/UBSan for Tests161-163: **PASS**.
6. Tests161-163 are ROM-audit/test/documentation-only; Test147/V0.162 remains the latest SH-2/Yaul compile+link PASS.
7. No new BIN/CUE was produced after Test144.

## Hardware state

Test144/V0.159 remains the current pending hardware-validation image. Test100/V0.115 remains the latest image physically confirmed on NTSC-U Saturn + Terraonion MODE until the user tests Test144.

## Critical evidence through Test163

- Earthquake selection character 8, prone category `$07`, fresh button1 resolves exact raw action `$47`, descriptor `$14440`, callback `$14458`; move identity remains CANDIDATE.
- Action `$47` trajectory/contact/fallback consequences are pinned through Tests148-158; do not promote their high-level semantics without independent evidence.
- `+$109` producer state initializes once via `+$1C bit7`, then increments target `+$109` in lockstep with producer phase `+$25`; phase 7 reaches a terminal branch.
- **Test161 ROM-EXACT:** `$1FB04-$1FB18` indexes an 11-entry table at `$1FB1A` from actor byte `+$1D`; table entry 6 is exactly `$20022`, proving the `+$109` producer is raw substate 6.
- **Test162 ROM-EXACT:** actor processing first dispatches raw `+$21` through table `$1F952`, then unconditionally calls `$1FAF0`, the secondary `+$1D` dispatcher.
- **Test163 ROM-EXACT:** before `+$1D` dispatch, `$1FAF0` tests actor `+$20 bit7`; when set it clears that bit and copies raw `+$20` to `+$1C`.
- These relations materially tighten the actor-state context of `+$109`, but no fatigue/aggression/cadence semantic is yet justified.

## Fidelity rules

Every Test must implement or rigorously validate a real function. Preserve evidence labels: **ROM-EXACT**, **PROVISIONAL**, **REGISTERED BUT NOT IMPLEMENTED**. Never promote a move, damage value, AI behavior, geometry, timing or presentation detail to arcade-exact from visual plausibility or memory.

## Immediate continuation

1. Continue tracing raw `+$1D==6/$20022` callers, linked actor/target ownership and exit transitions until the gameplay context of `+$109` is demonstrated; only then reconcile it with stamina bucket `+$BD` for live CPU behavior.
2. Continue Earthquake action `$47` through contact/completion and independently identify its trademark move before promotion.
3. Recover exact TAG/Royal victory, defeat and winner-clap/celebration descriptors/states/coordinates/timing.
4. Complete/prove all Super Throws / Signature Finisher Moves, including LOD, respecting per-record evidence flags.
5. Continue exact TAG apron/entry/exit, Royal assets/eliminations, records, attract/promo, presentation and audio work as evidence permits.

## Long target

Continue concrete checkpoints toward Test200. At genuine Test200 report exactly:
- IMPLEMENTADO
- REGISTRADO PERO NO IMPLEMENTADO
- PROVISIONAL
- PENDIENTE DE BUILD SATURN
