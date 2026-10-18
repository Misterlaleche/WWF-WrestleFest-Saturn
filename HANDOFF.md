> RUNTIME-17 R02 (2026-10-18): see CURRENT-STATE-LATEST.md for this checkpoint and its tested scope. Historical hardware results below belong to earlier separate builds; they do not certify this gameplay checkpoint.

# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, `docs/RUNTIME-AUDIO-RECONCILIATION.md`, and `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md` before doing any work.

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Current immediate work

1. Revalidate BLOCK-ROSTER-01 BGFIX on real Saturn.
2. Close BLOCK-MODE-01 TAG visual effect from indexed ROM provenance.
3. Continue no-game frontend blocks before full gameplay integration.

4. Audio:
   - transport is HARDWARE-PASS;
   - original command-byte dispatcher is present;
   - full producer matrix contains 173 ROM-verified rows;
   - owner map R01 classifies 14 rows `INTEGRATION_READY`, 4 `OWNER_SCOPE_ONLY`, 155 `UNRESOLVED`;
   - use only the 14 ready rows for direct runtime insertion;
   - low-stamina is producer `0x009088`, JSR `0x00908C`, command `0x0B`;
   - `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`;
   - ring-out/count-out ready rows belong to WP08;
   - finisher ready rows belong primarily to WP01 with WP22/WP10 support;
   - exact `0x1F` fade and production cue lifetimes remain open.

5. Gameplay runtime now exists on branch `gameplay-runtime-r01`.
   - Do not import WIP160/175/176 as the base.
   - Insert audio hooks only when the canonical CLEAN-R01 equivalent state/callback exists.
   - `OWNER_SCOPE_ONLY` rows are blocked until their semantic trigger is proven.

## What other chats must preserve

- No BIN/CUE concatenation; integrate source modules.
- One shared Saturn platform layer.
- No guessed timings/palette/input/audio semantics.
- No placeholder/fake music presented as final.
- No finite diagnostic PCM capture relabelled as an arcade loop.
- Do not restore TEST-03 fixed-time refill.
- Do not restore historical `wf_audio_observer.c` slot-0 semantics as ROM truth.
- Real Saturn + MODE remains the hardware gate.

## Gameplay handoff

Gameplay owner branch: `gameplay-runtime-r01`.

Continue from checkpoint `BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`. Source integration commit: `8589e9ef796c2ed6ed00fb9a9f07c8eda3c96f24`. RUNTIME-11 remains the frozen reference base; RUNTIME-12/13/14/15/16 extend it cumulatively.

Do not restart gameplay from WIP160/WIP175/WIP176.

Current gates: ROM audit PASS, host PASS, SH-2 cross compile PASS, full Saturn ELF link PASS (41 objects), unresolved symbols 0. This is still CANDIDATE. Real WP28 branch-A integration reaches `$514E -> $8B3A -> $2088E -> $20958 -> $26936 -> $2983C -> $F560 -> $F18A -> $1C150` and stops fail-closed at mandatory unbound `$1F914`. `$21B4` remains blocked until exact live register context is proved. Direct-match and attract gameplay bootstraps are not synthesized.

Read `docs/GAMEPLAY-RUNTIME-R01.md` before changing gameplay.
