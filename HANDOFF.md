# HANDOFF

Read `README.md`, `PROJECT-STATE.md`, and `docs/SATURN-PIPELINE.md` before doing any work.

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

4. Audio is a separate diagnostic block. Exact audio source must come from the audited YM2151 + OKIM6295 pipeline; fail closed if source assets are absent.

## What other chats must preserve

- No BIN/CUE concatenation; integrate source modules.
- One shared Saturn platform layer.
- No guessed timings.
- No guessed palette effects.
- No guessed input repeat.
- No placeholder/fake music presented as final.
- Real Saturn + MODE is the hardware gate for visual/audio closure.
