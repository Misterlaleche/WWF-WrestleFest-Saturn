# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, `docs/RUNTIME-AUDIO-RECONCILIATION.md`, and `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md` before doing any work.

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Current immediate work

1. Treat **MODE + ROSTER TEST03** as the canonical hardware-proven combined checkpoint for these two frontend sections.
2. Import its proven behavior into the master/runtime without replacing the canonical CLEAN-R01 main-loop/state ownership.
3. After import, revalidate on real Saturn both routes: `TAG -> ROSTER -> confirm` and `ROYAL -> ROSTER -> confirm`, preserving the no-flash handoff and localized green confirmation patch.
4. Continue the next proven frontend/prematch integration boundary; do not invent the post-ROSTER transition if its owner/evidence is not yet bound.

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

5. Current canonical gameplay runtime source is still absent from this repository.
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
- MODE + ROSTER TEST03 is hardware-proven; do not regress to the separate candidate harnesses or reintroduce their freeze/timeouts.
- Do not clear/rebuild the full ROSTER NBG0 on confirmation; use the resident foreground and localized 96x96 VBLANK-IN update.
- Preserve MODE -> ROSTER display blank/synchronization before changing ROSTER CRAM/VRAM.
- See `docs/MODE-ROSTER-HARDWARE-VALIDATION.md`.
