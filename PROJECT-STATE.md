# PROJECT STATE

## Current development policy

Native ROM-EXACT reimplementation of WWF WrestleFest (1991) for Sega Saturn NTSC-U. Supreme authority: original `wwfwfest.zip` ROM.

## Hardware-proven foundation

Real Saturn + Terraonion MODE has validated:
- boot/VDP foundation;
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
- ROSTER: transparent NBG0 moving foreground over fixed NBG2 background — HARDWARE-PASS.
- Confirmation: bright green localized 96x96 patch at VBLANK-IN with cursor preserved — HARDWARE-PASS.
- TEST03 `main.c` remains an isolated harness and must not replace CLEAN-R01/master `main.c`.
- TITLE / INTRO is owned by separate `intro-presentation-r01`; master consumes only its audited checkpoint.

## AUDIO

Status: **RUNTIME AUDIO HANDOFF R01 ACCEPTED / TRANSPORT HARDWARE-PASS / PRODUCER MATRIX ROM-AUDITED / OWNER MAP R01 CLOSED / SOURCE INSERTION PARTIAL-OPEN.**

External handoff SHA256:
`cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65`.

- 173 ROM-verified producer rows.
- 14 `INTEGRATION_READY`.
- 4 `OWNER_SCOPE_ONLY`.
- 155 `UNRESOLVED`.
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`.
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`.
- no new WP28 audio phase;
- no semantic observer;
- exact `0x1F` lifecycle remains OPEN;
- final cue lifetime/termination and integrated gameplay audio hardware validation remain OPEN.

## INTEGRATION MASTER

Branch: `integration-master-r01`.

### R02 — CDFS runtime loader
- HWRAM staging / LWRAM bulk classes;
- no direct CDFS writes into VDP1/VDP2/CRAM/SCSP;
- transaction preflight / overlap rejection / scene publication gate;
- host PASS;
- SH-2 core + Yaul adapter compile PASS.

### R03 — machine-readable memory manifest
- VDP2/CRAM/SCSP known partitions represented;
- structural validator PASS;
- final-fit remains BLOCKED by VDP1 maxima, final ELF/link map, HWRAM scene peaks, final CDFS root count and hardware transfer-window measurements.

### R04 — WP30 orphan ownership resolution
Master owns:
- O30-01 Records/ranking/initials;
- O30-02 Ending Credits;
- O30-03 Attract/Bio root orchestration;
- O30-06 Scene Flow Master root.

Gameplay owns:
- O30-04 CPU AI controller remainder;
- O30-05 WP31 objects/effects/props.

### R05 — Scene Flow Master
Fail-closed root router exists in `src/runtime/wf_scene_flow_master.[ch]`.
Only proven evidence-tagged root transitions are accepted.

### R06 — Records / TOP RANKING
Pure ranking model + Saturn-specific save serializer implemented.
Host + SH-2 PASS.
OPEN: runtime hook, renderer, Backup RAM adapter/hardware persistence.

### R07 — Ending Credits
Nine-page + staff-scroll controller implemented.
Host + SH-2 PASS.
OPEN: renderer/audio binding/entry binding/hardware.

### R08 — Attract Biography
12-character BIO controller/pack contract implemented.
Host + SH-2 PASS.
Exact shared RNG call ordering remains `BLOCKED_BY_RUNTIME_OWNER`.

### R09 — Attract Demo root contract
14 state/mode/lineup/threshold table and exact 24+33+33 post-demo transition implemented.
No synthetic joystick/button autoplay.
Ordinary gameplay/CPU remains gameplay-owned.
`$1F7F0` override and shared `$21B4` RNG remain gameplay dependencies.

### R10 — Continue / Game Over
WP14 countdown/session controller implemented:
- exact pair order;
- 9..0, 64 iterations per digit, 640 total;
- live credit availability;
- current START level;
- final-iteration accept-before-expire ordering;
- exact normal/Royal Game Over wait constants.
Host + SH-2 PASS.

### R11 — Operator / Credits / Payment
WP19 core implemented:
- operator config packing;
- coinage conversion;
- pulse filter;
- payment modes 0..5;
- affordability preview;
- title prompt IDs;
- demonstrated DIP consumers.
Unknown physical semantics remain fail-closed.
Host + SH-2 PASS.

### R12 — Continue ↔ payment bridge
R10 Continue now binds to the exact R11 WP19 payment service.
No private Continue credit model.
Host + SH-2 PASS.

### R13 — Campaign interstitial contract
WP13 pure post-win controller implemented:
- B608 gate;
- BDA6 selector;
- exact action/card expiry semantics;
- raw `0x312A` emission point;
- 142 synchronized BDA6 iterations;
- 24-VBlank fade;
- progression increments only after both interstitial calls return;
- no synthetic ten-match ladder.
Host + SH-2 compile validation pending final checkpoint packaging.

## Gameplay runtime

Branch: `gameplay-runtime-r01`.

Current named dependency observed by master:
**BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13**
commit `91f4c697bec0b04678cc1e811b4ee60e606d7131`.

Its host tests, SH-2 compile, full Saturn ELF link and 0 unresolved-symbol gate PASS, but normal branch-A still fails closed at mandatory unbound phase `0x02088E`.
Direct gameplay/attract bootstrap, remaining WP28 owners and canonical shared `$21B4` RNG service/order remain OPEN.

WIP160/WIP175/WIP176 are not canonical gameplay bases.

## Integration rule

Consume gameplay, MODE+ROSTER, INTRO and audio only by named audited checkpoint/contract.

WP24 remains the sole audio backend owner.
MODE+ROSTER TEST03 behavior is transplanted, not its architecture.
Unknown behavior stays OPEN/BLOCKED.
No BIN/CUE unless explicitly requested.
