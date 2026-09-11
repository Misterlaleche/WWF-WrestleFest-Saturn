# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, `docs/SATURN-PIPELINE.md`, `docs/AUDIO-HARDWARE-VALIDATION.md`, and `docs/RUNTIME-AUDIO-RECONCILIATION.md` before doing any work.

## Absolute project rule

`wwfwfest.zip` ROM > audited ROM-EXACT FINAL packages > Saturn/Yaul documentation > current Saturn code.

Never invent missing behavior.

## Current immediate work

1. Revalidate BLOCK-ROSTER-01 BGFIX on real Saturn:
   - fixed background visible;
   - roster visible;
   - navigation works one ROM-equivalent directional event at a time;
   - page transition visibly scrolls 208 px as 26 x 8 px;
   - no return to full-frame DMA.

2. Close BLOCK-MODE-01:
   - use indexed ROM-derived palette/pen provenance;
   - match TAG visual selection effect to arcade;
   - do not disturb already-good ROYAL behavior without ROM evidence.

3. Continue block architecture for full no-game frontend before gameplay:
   - TITLE/INTRO
   - credits display / coin frontend
   - rankings TAG and ROYAL
   - attract presentation/interstitials
   - attract master controller
   - playable demo later

4. Audio transport is hardware-proven and the runtime-facing raw hook layer now exists:
   - use WP24 slot-31 / 64 KiB ring / CA-driven transport under `src/audio/wp24/`;
   - use `wf24_command_dispatch()` with original arcade command bytes;
   - use `wf24_runtime_audio.*` for only ROM-backed producer/call-site bindings;
   - stage music uses raw ROM words `0x3104...`;
   - low-stamina producer is anchored to ROM PC `0x009088` (`JSR` at `0x00908C`), not a guessed fighter slot;
   - ring-out uses raw `+$54 = 0x3165..0x3168`;\n   - `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED` and must not be reused as low-stamina;
   - only proven finisher/heavy call-sites are admitted;
   - exact `0x1F` fade and final production cue lifetime remain open.

5. The current canonical gameplay runtime source is still absent from this repository.
   - Do not import WIP160/175/176 as the base merely to make audio hooks appear integrated.
   - When the CLEAN-R01 runtime source is reconciled, insert audio side effects at the equivalent proven producer/callback branches without adding a scheduler phase.

## What other chats must preserve

- No BIN/CUE concatenation; integrate source modules.
- One shared Saturn platform layer.
- No guessed timings.
- No guessed palette effects.
- No guessed input repeat.
- No placeholder/fake music presented as final.
- No finite diagnostic PCM capture relabelled as an arcade loop.
- Do not restore TEST-03's fixed-time music refill scheduler.
- Do not restore the historical `wf_audio_observer.c` slot-0 semantic adaptation as ROM truth.
- WIP160/175/176 are not canonical bases.
- Real Saturn + MODE is the hardware gate for visual/audio closure.
