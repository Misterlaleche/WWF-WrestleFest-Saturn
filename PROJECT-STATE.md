# PROJECT STATE

## Current development policy

Native ROM-EXACT reimplementation of WWF WrestleFest (1991) for Sega Saturn NTSC-U. Supreme authority: original `wwfwfest.zip` ROM.

## Hardware-proven foundation

Real Saturn + Terraonion MODE has validated boot/VDP foundation plus:
- authentic SCSP PCM playback;
- master volume;
- SFX and bank-1 presenter/ring-announcer voices;
- WP24 slot-31 PCM16BE streaming with 64 KiB double buffer;
- CA-driven refill;
- long continuous TEST-05 music playback without skips/premature cuts.

## Current frontend status

- **MODE + ROSTER TEST03: HARDWARE-PASS** on real Saturn NTSC-U + Terraonion MODE (2026-09-11).
- Combined route validated: `MODE -> TAG/ROYAL confirm -> ROSTER -> wrestler confirm`.
- MODE -> ROSTER yellow/orange flash: CLOSED by display blank/sync before ROSTER CRAM/VRAM reconfiguration.
- ROSTER composition: transparent NBG0 moving foreground over fixed NBG2 background — HARDWARE-PASS.
- Confirmation: bright green 96x96 localized patch at VBLANK-IN, cursor preserved — HARDWARE-PASS.
- Full NBG0 confirmation rebuild is forbidden; TEST03 eliminated that regression.
- Master import is still pending and must preserve CLEAN-R01 ownership; TEST03 `main.c` is a harness only.
- TITLE / INTRO: PARTIAL.
- RANKINGS / ATTRACT: NOT INTEGRATED.

## AUDIO

Status: **TRANSPORT HARDWARE-PASS / PRODUCER MATRIX ROM-AUDITED / OWNER MAP R01 CLOSED / GAMEPLAY SOURCE INSERTION OPEN.**

- 173 total ROM-verified producer rows.
- 14 rows: `INTEGRATION_READY`.
- 4 rows: `OWNER_SCOPE_ONLY`.
- 155 rows: `UNRESOLVED`.
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`.
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.

## INTEGRATION MASTER / WP32-WP33

Branch: `integration-master-r01`.

Status: **MEMORY/CDFS R02 CANDIDATE**.

Closed/defined at current authority:
- HWRAM/LWRAM/VDP1/VDP2/CRAM/SCSP ownership boundaries;
- LWRAM forbidden as SCU DMA endpoint;
- CDFS sector = 2048 bytes;
- current CDFS file-list capacity = 384;
- WP24 full SCSP ownership including music ring `0x60000..0x6FFFF`;
- CRAM mode 2 with four 256-entry windows from WP26 FINAL;
- VDP2 capacity map sufficient for full active FG/BG/TEXT caches without dropping source tiles;
- six-wrestler LWRAM demonstrated peak policy retained;
- runtime CDFS loader with exact root lookup;
- HWRAM staging / LWRAM bulk load classes;
- no direct CDFS writes into VDP1/VDP2/CRAM/SCSP;
- transaction-wide preflight and destination-overlap rejection;
- scene publication remains false until every requested read succeeds.

Validation:
- memory-contract host tests PASS;
- CDFS-loader host tests PASS with `-Wall -Wextra -Werror`;
- SH-2 memory/core compile PASS;
- SH-2 Yaul adapter compile PASS;
- no BIN/CUE generated;
- no new hardware PASS claimed.

Still BLOCKED/OPEN:
- source-level import of MODE+ROSTER TEST03 behavior into master runtime;
- final VDP1 VRAM partition until maximum simultaneous visible command/unique-texture reachability is demonstrated;
- exact asset byte manifests for remaining scenes;
- final HWRAM free budget until the integrated ELF/map exists;
- full Saturn link after gameplay/frontend merge;
- measured real-hardware loading/transfer timings.

## Gameplay runtime

Gameplay is developed independently on branch `gameplay-runtime-r01` from the audited RUNTIME-11 lineage. This integration branch must not duplicate gameplay ownership.

WIP160/175/176 must not be promoted as canonical gameplay bases.

## Integration rule

Consume gameplay and frontend blocks by named audited checkpoint/commit. WP24 remains the sole audio backend owner.

MODE+ROSTER TEST03 must be transplanted behaviorally, not architecturally: preserve the validated blank/sync, layer model, resident foreground and VBLANK-IN 96x96 confirmation patch, while keeping the canonical master main loop/state ownership.
