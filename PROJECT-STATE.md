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
- TITLE / INTRO: owned by separate intro-presentation block; master consumes its audited checkpoint.
- Records R06 pure ranking model: IMPLEMENTED / host+SH-2 PASS; runtime hook/renderer/Backup RAM adapter still OPEN.\n- Credits R07 controller: IMPLEMENTED / host+SH-2 PASS; renderer/audio binding/entry binding/hardware still OPEN.\n- Attract Bio R08 controller: IMPLEMENTED / host+SH-2 PASS; exact shared RNG ordering remains BLOCKED_BY_RUNTIME_OWNER.\n- Attract Demo R09 master contract: IMPLEMENTED / host+SH-2 PASS; ordinary gameplay/CPU override/shared RNG remain gameplay-owned.

## AUDIO

Status: **RUNTIME AUDIO HANDOFF R01 ACCEPTED / TRANSPORT HARDWARE-PASS / PRODUCER MATRIX ROM-AUDITED / OWNER MAP R01 CLOSED / SOURCE INSERTION PARTIAL-OPEN.**

External handoff SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`.

Producer ownership:
- 173 total ROM-verified producer rows;
- 14 `INTEGRATION_READY`;
- 4 `OWNER_SCOPE_ONLY`;
- 155 `UNRESOLVED`.

Critical correction:
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.

Rules:
- no new WP28 audio phase;
- no semantic observer;
- unresolved rows stay fail-closed;
- exact `0x1F` lifecycle OPEN;
- final production music lifetimes/termination OPEN;
- integrated simultaneous music/SFX/voice hardware validation OPEN.

See `docs/AUDIO-RUNTIME-HANDOFF-R01.md`.

## INTEGRATION MASTER / WP32-WP33

Branch: `integration-master-r01`.

Status: **MEMORY/CDFS R03 CANDIDATE**.

Closed/defined:
- HWRAM/LWRAM/VDP1/VDP2/CRAM/SCSP ownership boundaries;
- LWRAM forbidden as SCU DMA endpoint;
- CDFS sector = 2048 bytes;
- current CDFS file-list capacity = 384;
- WP24 full SCSP ownership;
- CRAM mode 2 with four 256-entry windows from WP26 FINAL;
- VDP2 fixed technical-equivalence partition;
- six-wrestler LWRAM demonstrated peak policy;
- runtime CDFS loader with HWRAM staging / LWRAM bulk classes;
- no direct CDFS writes into VDP1/VDP2/CRAM/SCSP;
- transaction preflight/overlap rejection/publication gate;
- machine-readable `manifests/final-memory-manifest.r03.json`;
- fail-closed validator `tools/validate_final_memory_manifest.py`.

R03 structural validation: PASS.
R03 final-memory-fit gate: intentionally BLOCKED until:
- final VDP1 command/texture maxima;
- integrated ELF/link map;
- HWRAM scene peaks;
- final CDFS root count;
- hardware transfer-window measurements.

No BIN/CUE generated. No new hardware PASS claimed.

## WP30 ownership closure

WP30 FINAL identified six project ownership gaps. R04 assigns every one explicitly without claiming implementation closure:

Master `integration-master-r01`:
- O30-01 Records/ranking/initials;
- O30-02 Ending Credits;
- O30-03 Attract/Bio root orchestration;
- O30-06 single frontend/attract Scene Flow Master root.

Gameplay `gameplay-runtime-r01`:
- O30-04 full CPU AI controller remainder beyond State5;
- O30-05 WP31 objects/effects/props actor engine.

Machine-readable authority:
`manifests/wp30-orphan-resolution-r04.json`.

Ownership validator/tests: PASS.

## Gameplay runtime

Gameplay is developed independently on branch `gameplay-runtime-r01` from the audited RUNTIME-11 lineage.\n\nCurrent named dependency observed by master: **BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13**, commit `91f4c697bec0b04678cc1e811b4ee60e606d7131`. Its host tests, SH-2 compile, full Saturn ELF link and 0-unresolved-symbol gate PASS, but normal branch-A frame still fails closed at mandatory unbound phase `0x02088E`; direct-match/attract bootstrap and canonical shared `$21B4` RNG service/order are not yet closed.

Its explicit scope includes CPU FINAL remainder and WP31 object/effect/prop engine in addition to the previously assigned gameplay owners.

WIP160/175/176 must not be promoted as canonical gameplay bases.

## Integration rule

Consume gameplay, frontend, intro, and audio by named audited checkpoint/contract. WP24 remains the sole audio backend owner.

MODE+ROSTER TEST03 must be transplanted behaviorally, not architecturally.

Audio producer hooks are inserted only where their exact CLEAN-R01 owner/callback exists; otherwise `BLOCKED_BY_RUNTIME_OWNER`.

Ownership assignment is not implementation closure. R06/R07/R08/R09 now provide integration-safe master-side controllers/contracts, but their renderer/runtime-owner/hardware gates remain explicit.

## Scene Flow Master R05

WP30 O30-06 now has a concrete fail-closed root router in `src/runtime/wf_scene_flow_master.[ch]`.

The router accepts only proven `(from,to,evidence)` handoffs exported in `manifests/scene-flow-proven-edges-r05.csv`. Unknown or evidence-mismatched transitions are rejected without changing current state.

Validation:
- host compile/test PASS with `-Wall -Wextra -Werror`;
- SH-2 freestanding object compile PASS;
- full Saturn ELF link not claimed until destination scene modules are merged;
- hardware PASS not claimed;
- no BIN/CUE/ISO generated.
