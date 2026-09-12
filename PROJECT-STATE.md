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
- **INTRO ROM-EXACT FINAL: HARDWARE-PASS** on real Saturn NTSC-U + MODE from package `WF-SATURN-INTRO-ROMEXACT-FINAL-HWTEST(1).zip`.
- INTRO package SHA256: `0fb488514ae233c429058834b8ced73c757b0224d819d6ba4dc3015042f81c98`.
- TITLE.ANM contains **451 stored asset frames; 451 is NOT the native arcade intro duration**.
- INTRO owns only boot/title-build visuals. `CREDIT:x`, `INSERT COIN`, START prompt and title/attract/mode continuation are **FRONTEND-owned**; authoritative credit state/prompt selection remains WP19/R11.
- The hardware-test harness infinite final-frame hold is inspection-only and must not enter the master.

### R17 — INTRO Hardware-Pass handoff
Accepted and audited in `docs/INTRO-HARDWARE-PASS-R17.md`.
The exact package/BIN hashes are frozen in `manifests/intro-hardware-pass-r17.json`.
Master will transplant proven behavior, not the isolated harness `main.c`.

## AUDIO

Status: **RUNTIME AUDIO HANDOFF R02 RE-AUDITED ACCEPTED / TRANSPORT HARDWARE-PASS / PRODUCER MATRIX ROM-AUDITED / OWNER MAP R01 CLOSED / SOURCE INSERTION PARTIAL-OPEN.**

Canonical external handoff:
`WWF-RUNTIME-AUDIO-HANDOFF-R02-REAUDITED.zip`

SHA256:
`49335637b8ba6d26b361c88e3886f620e9f52c076df3d4bfb44accf3701ccb5c`

R02 **supersedes R01**. R01 SHA256 `cc1c9ed44edcde0237770521123072c3bbec6927bf37184c157e7b092858db65` is historical only.

R02 re-audit result:
- top-level package SHA256 manifest: 31/31 PASS;
- all nested ZIPs: PASS;
- complete WP24 source/test snapshot included;
- 12/12 WP24 source/test Git blobs are already byte-identical in `integration-master-r01`; no source overwrite required;
- 173 ROM-verified producer rows;
- 14 `INTEGRATION_READY`;
- 4 `OWNER_SCOPE_ONLY`;
- 155 `UNRESOLVED`;
- low-stamina = `0x009088 -> 0x0B`, JSR `0x00908C`;
- `0x0263CC -> 0x0B` remains `REGISTERED_NOT_IDENTIFIED`;
- no new WP28 audio phase;
- no semantic observer;
- exact `0x1F` lifecycle remains OPEN and must fail closed;
- final cue lifetime/termination and integrated gameplay audio hardware validation remain OPEN.

See `docs/AUDIO-RUNTIME-HANDOFF-R02.md`.

## INTEGRATION MASTER

Branch: `integration-master-r01`.

### R02 — CDFS runtime loader
Host PASS; SH-2 core + Yaul adapter compile PASS.

### R03 — machine-readable memory manifest
Structural validator PASS.
Final-fit remains BLOCKED by VDP1 maxima, final ELF/link map, HWRAM scene peaks, final CDFS root count and hardware transfer-window measurements.

### R04 — WP30 ownership resolution
Master: Records, Credits, Attract/Bio root, Scene Flow root.
Gameplay: CPU-controller remainder, WP31 objects/effects/props.

### R05 — Scene Flow Master
Fail-closed evidence-tagged root router implemented.

### R06 — Records / TOP RANKING
Pure ranking model + Saturn save serializer implemented. Host + SH-2 PASS.
OPEN: runtime hook, renderer, Backup RAM hardware adapter.

### R07 — Ending Credits
Nine-page + staff-scroll controller implemented. Host + SH-2 PASS.
OPEN: renderer/audio/entry binding and hardware validation.

### R08 — Attract Biography
12-character BIO controller/pack contract implemented. Host + SH-2 PASS.
Exact shared RNG ordering remains `BLOCKED_BY_RUNTIME_OWNER`.

### R09 — Attract Demo root contract
14 state/mode/lineup/threshold table + exact 24+33+33 exit cadence implemented.
A01 correction: the first 24-VBlank stage is ROM JSR $008F50 stripe-mask close, not generic palette fade.
No synthetic physical-input autoplay.
Gameplay CPU override `$1F7F0` and shared `$21B4` RNG remain gameplay-owned.

### R10 — Continue / Game Over
Exact WP14 countdown/session controller implemented. Host tests revalidated PASS.

### R11 — Operator / Credits / Payment
Exact WP19 operator/coin/payment core implemented. Host tests revalidated PASS.

### R12 — Continue / payment bridge
Continue uses the same exact WP19 payment service; no private credit model. Host integration test revalidated PASS.

### R13 — Campaign interstitials
WP13 post-win controller implemented.
A01 correction closes the ROM-demonstrated pre-interstitial progression==9 final branch to $1B4E, the config-bit7 extra progression increment at post-increment values 3/8, and the progression==5 table-preparation owner contract.
- B608/BDA6 gates;
- action/card expiry;
- raw `0x312A` event;
- 142 synchronized BDA6 iterations;
- 24-VBlank fade;
- progression increment only after interstitial return;
- no synthetic ten-match ladder.
Host + SH-2 compile revalidated PASS.

### R14 — Frontend Transition Engine
WP15 pure transition primitives implemented:
- exact common palette fade 7->0, 3 VBlank/level, 24 total;
- exact 16x8 nibble fade table;
- stripe close/open `$4BE..$4C5`;
- exact 17/33/257 wait primitive;
- exact 26x8 selector slide between `$0140/$0210`.
Host + SH-2 PASS.
No generic fade substitution for direct-clear or full-mask routes.

### R19 — Instructional hand prompt / WP16 R02
External checkpoint `WP16-HAND-PROMPT-R02.zip` accepted after independent ROM re-audit.
- package SHA256 `ffa8a1104023db142ba0f379c9f79a77c393ad5e79f8769b5f7ecde0a0877642`;
- visual payloads remain byte-identical to WP16 FINAL;
- request lifecycle, fresh-edge mechanic, pose cadence and one-pass clear are ROM-proven;
- prompt presentation is global/player-independent and mode-agnostic;
- no WP16-specific audio event is proven;
- pictured button is NOT proven to mean specifically A, B or A+B;
- Gameplay/WP27 owns producer/input/mash-counter semantics;
- Saturn renderer binding and hardware validation remain OPEN.
Host + SH-2 PASS. R15 visual-only contract is superseded.

### R16 — Gameplay HUD core
WP18 pure core implemented:
- four top-HUD anchors;
- player-indicator tile bases;
- raw portrait descriptors;
- queued/accumulated/displayed stamina convergence;
- exact six-column stamina quantization;
- exact BCD 30:01 seed -> first visible 30:00;
- decrement every four eligible loops;
- 05-minute event command `0x0B`;
- time-up clamp 00:00 and transition gate 3.
Host + SH-2 PASS.

Still OPEN in HUD:
- exact C710 live selector integration;
- gameplay credit numeric compositor/cache binding;
- referee/outside count renderer binding;
- renderer writes under exact gameplay camera/scroll;
- no wrestler names, generic score or Royal elimination counter may be invented.

## Gameplay runtime

Branch: `gameplay-runtime-r01`.

Current named dependency observed by master:
**BLOCK-GAMEPLAY-MASTER-01-RUNTIME-15**
source checkpoint `72de727a77603c0a81f552fa059c0d2072cd249b`; gameplay branch documentation HEAD `7f40fde30cb7af32cf95cd4d9469884a342da8b8`.

RUNTIME-15 host tests PASS, SH-2 cross-compile PASS (39 translation units), full Saturn ELF link PASS and unresolved symbols = 0. It closes the ROM-derived WP28 target/relation phase `0x020958` plus already-audited WP21 camera phases `0x026936/0x02983C`. Branch-A now reaches `0x514E -> 0x8B3A -> 0x2088E -> 0x20958 -> 0x26936 -> 0x2983C` and fails closed at the next mandatory phase `0x00F560`.
Direct-match/attract bootstrap, remaining WP28 owners and canonical shared `$21B4` RNG service/order remain OPEN.

WIP160/WIP175/WIP176 are not canonical gameplay bases.

## Integration rule

Consume gameplay, MODE+ROSTER, INTRO and audio only by named audited checkpoint/contract.

WP24 remains the sole audio backend owner.
MODE+ROSTER TEST03 behavior is transplanted, not its architecture.
Unknown behavior stays OPEN/BLOCKED.
No BIN/CUE unless explicitly requested.


## Integration Self-Audit A01

Status: **PASS WITH TWO CORRECTED FINDINGS**.

- R09 incorrect palette-fade event label corrected to exact ROM $008F50 stripe-mask close; 24+33+33 timing unchanged.
- R13 incomplete campaign control path corrected: progression==9 final gate, bit7 progression skip at 3/8, and progression==5 preparation wait.
- host/authority test matrix PASS;
- unified SH-2 relocatable audit PASS with zero duplicate globals;
- no integrated hardware PASS claimed.

See `docs/INTEGRATION-SELF-AUDIT-A01.md`.


## WP12 ENTRANCES/PREMATCH — Audit R3 master decision

**WP12 R01: REJECTED / NOT INTEGRABLE.**

Authority:
`WP12-R01-AUDIT-REPORT-R3(1).txt`
SHA256 `fb051c4e527db74514cacf4944deef5bbd129ae6429890697f75a63a1ab31091`.

Audit R3 supersedes R1/R2 for the R01 integration decision.

Current status:
- TAG: BLOCKED
- ROYAL: BLOCKED
- PRESENTADORA: PARTIAL
- ÁRBITRO PREMATCH: MISSING
- HANDOFF A GAMEPLAY: BLOCKED
- HOST COMPILE: PASS
- SH-2 COMPILE: PASS
- HARDWARE: NOT TESTED

The R01 branch `entrances-prematch-r01` at `820ea42af3fce45591eace60cba22a94d9ad3fba` must not be consumed by master. Delivered ZIP and recorded GitHub HEAD are not reproducible as one source state.

Next required checkpoint: **WP12 R02**, reconstructed directly from `wwfwfest.zip` for all affected semantics and satisfying every mandatory R3 gate before it can be called CANDIDATE.

Do not touch GAMEPLAY/WP28, CPU, collision, TAG/Royal gameplay or WP24 backend. No BIN/CUE.
See `docs/WP12-R01-REJECTION-R3.md`.


### R18 — Frontend TITLE presentation bridge

Implemented master-side source contract binding TITLE credit/prompt presentation to authoritative WP19/R11 state.

Closed in R18:
- Frontend receives live `credits` and `partial` from `wf_credit_state_t`;
- exact WP19 prompt ID is used via `wf_title_prompt_id()`;
- exact WP19 title blink phase is used via `wf_title_prompt_blink_phase()`;
- bridge is read-only and cannot mutate credit state;
- INTRO private credit counter / hard-coded CREDIT:0 remain forbidden.

Validation:
- host `-Wall -Wextra -Werror -pedantic`: PASS;
- SH-2 `-Wall -Wextra -Werror`: PASS;
- relocatable SH-2 link with WP19: PASS;
- no new project-owner unresolved symbols.

Still OPEN:
- exact frontend glyph/coordinate/palette renderer binding for CREDIT/INSERT COIN;
- integrated Saturn hardware validation;
- exact title continuation into attract/mode assembly.

See `docs/FRONTEND-TITLE-R18.md`.
