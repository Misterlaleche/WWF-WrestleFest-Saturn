# SATURN PIPELINE / BLOCK INTEGRATION RULES

This document is mandatory reading before changing any Saturn implementation.

## Authority order

1. `wwfwfest.zip` original arcade ROM — supreme authority.
2. Audited ROM-EXACT FINAL packages in the ChatGPT project sources.
3. Saturn / Yaul technical documentation.
4. Existing Saturn code and diagnostic builds.

If a lower layer disagrees with a higher layer, the higher layer wins.

## Fail-closed rule

Do not invent provisional mechanics, timings, transitions, RNG, input behavior, palette behavior, animation, audio, collision, AI, or frontend choreography merely to make a test look complete.

Unknown behavior stays OPEN.

## BLOCK method

Each feature is developed as an isolated BLOCK with its own hardware-test BIN/CUE harness when useful. The BIN/CUE files are diagnostic artifacts only. Final integration is from source modules and shared infrastructure; BIN/CUE images are never concatenated.

A BLOCK can be:
- CANDIDATE — compiles and is testable, but real Saturn validation is still pending.
- HW-PASS — observed working on real NTSC-U Saturn + MODE for the stated scope.
- ROM-EXACT CLOSED — evidence, implementation, and required hardware validation are all closed.

Do not promote CANDIDATE to FINAL merely because an emulator or host test passes.

## Shared Saturn infrastructure

All blocks must converge on one common platform layer:
- Saturn boot / CDFS
- VDP1 renderer
- VDP2 renderer
- SMPC input
- timing / VBlank scheduler
- SCSP audio transport
- asset loading / memory ownership
- VRAM / CRAM allocation policy

Do not create a private duplicate input loop, timer, renderer, or audio backend inside a block unless the block is explicitly a disposable diagnostic harness.

## Graphics pipeline — canonical rules

### General

- Source graphics must remain traceable to the arcade ROM or audited ROM-derived evidence.
- Preserve indexed-color provenance when the arcade effect depends on palette-bank or pen changes.
- Do **not** flatten dynamic indexed graphics to RGB and then attempt to recreate palette effects heuristically.
- Do not approximate an arcade transition with prerecorded video/bitmap playback when the ROM demonstrates an underlying state machine.
- Saturn adaptation may differ internally, but visible behavior and timing must follow ROM evidence.

### MODE SELECT

The current MODE work proved an important rule:
- TAG / ROYAL dynamic selection effects depend on indexed palette/descriptor behavior.
- The earlier RGB-flattened reconstruction was insufficient for TAG even when ROYAL looked convincing.
- Preserve per-pixel palette-bank/pen ownership from the ROM-derived source and reproduce descriptor/palette phase behavior from ROM evidence.
- No invented cursor is allowed when the arcade expresses selection through the option graphics themselves.

Current state: BLOCK-MODE-01 remains CANDIDATE until TAG visual behavior matches real arcade hardware capture closely enough and ROM evidence closes the remaining discrepancy.

### ROSTER SELECT

The arcade scroll rule currently established for the page transition is:
- total travel: 208 px
- 26 updates
- 8 px per update
- source positions: 0x0140 <-> 0x0210

Saturn implementation rule:
- foreground roster plane stays resident in VDP2 and is moved by hardware scroll;
- do not rebuild and DMA a full framebuffer every step;
- the fixed background must remain on a separate stable VDP2 layer;
- current candidate architecture uses movable roster content on NBG0 and a fixed background layer on NBG2;
- the NBG2 background fix is still CANDIDATE until real Saturn revalidation.

The earlier full-frame DMA implementation had correct spatial steps but ran about 3.8x too slowly on hardware; never compensate for renderer cost by falsifying ROM timing.

### TITLE / INTRO

Static VDP2 title-frame tests were diagnostic only. Streaming a pre-rendered TITLE.ANM frame sequence is not the final frontend architecture.

The final title/intro must execute the ROM-derived frontend state/timing logic and use ROM-derived assets. Do not use fake video playback to stand in for unresolved title animation.

## Input pipeline

Do not add modern debounce/autorepeat by intuition.

For roster selection, the audited arcade behavior around normalizer `$64E0` compares current direction against previous direction (`+$91`) and suppresses repeated movement while the same direction remains held. A new movement requires the ROM-equivalent new directional event.

Any Saturn input adapter must preserve arcade held/edge semantics instead of feeding raw held directions directly to every frontend update.

## Audio pipeline

Audio source authority is the audited FINAL audio packages plus the ROM.

Current canonical production target:
- combined YM2151 + OKIM6295 source rendering;
- 44.1 kHz mono signed PCM16 big-endian transport source;
- SCSP playback transport;
- no fake synth, no test tone presented as game audio, no provisional CDDA substitution;
- command 0x1F must keep its original fade lifecycle semantics.

Workpack 24 documents SCSP slot 31 and a 64 KiB double buffer at sound RAM `0x60000..0x6FFFF`.

A finite repeated PCM clip is a diagnostic source, not ROM-EXACT continuous synthesis.

## Hardware validation

Target hardware:
- Sega Saturn NTSC-U
- Terraonion MODE

Every hardware-sensitive BLOCK must record:
- exact artifact tested;
- what was visible/audible;
- what passed;
- what failed;
- whether the result is CANDIDATE or HW-PASS.

User video/capture comparisons are valid diagnostic evidence for visible discrepancies, but exact source values/timings must still come from ROM/audited evidence rather than estimating from camera color or CRT capture.

## Integration contract

Blocks must expose minimal state contracts rather than call each other's private implementation.

Examples:
- MODE SELECT outputs chosen mode and next frontend state.
- ROSTER SELECT receives mode and outputs selected wrestler(s) and next state.
- RANKING receives ranking mode and returns to the attract/frontend controller.
- TITLE queries the shared credits subsystem; it must not create a private incompatible credits model.

Before a new chat changes a closed block, it must read:
1. `README.md`
2. `PROJECT-STATE.md`
3. `HANDOFF.md`
4. this file
5. the relevant ROM-EXACT package(s)

If required evidence is unavailable, stop rather than infer.
