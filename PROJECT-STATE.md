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

### R20 — Instructional hand prompt / WP16 R06 corrective checkpoint
External checkpoint `WP16-HAND-PROMPT-R06.zip` accepted after independent ROM/asset re-audit.
- package SHA256 `c0d5fe5a135cb1c89e1a79023351a6e90f774bd0e2f0bf687e413eb736b1b3c8`;
- R06 supersedes R19/R02 visually because direct ROM renderer evidence proves the final TEXT row was missing from the old derived payloads;
- corrected bbox `[10,4,71,94]`;
- corrected pose hashes:
  - pose0 `352072fb1ab6b2be33a42b469572189b9202ed36578b14289002261d8316bf57`
  - pose1 `36ca81e2ac88ddb113000775003e9f0db0e8f087f9b0bf75660ec73d01527a02`;
- runtime request/fresh-edge/cadence semantics remain unchanged;
- exact 13-row / 78-write TEXT transaction and fixed raster transform are closed;
- Saturn-equivalent plane NBG2, priority 4;
- pictured button remains NOT proven to mean specifically A, B or A+B;
- Gameplay/WP27 owns producer/input/mash-counter semantics;
- hardware validation remains OPEN.
Host PASS. Package SH-2 module compile PASS. R19/R02 visual payloads are superseded.

### WP18 HUD Integration R03 — active HUD authority
External checkpoint `WP18-HUD-INTEGRATION-R03-AUDITED(1).zip` accepted after independent ROM/build re-audit.
- ZIP SHA256 `8c8b46fa3b1f60f8e76fab2f76e6692177f9171e93712f9a946a0b15f148f19f`;
- bundle HEAD `6dfd3db09f7b45fc8b2011ec6f557aa2ab6aabcd`;
- package hashes 51/51 PASS;
- HOST HUD + timer PASS;
- SH-2 PASS;
- ROM portrait/palette audits PASS;
- exact preview reproduction PASS.

R03 closes the R02 red/blue inversion: arcade source words are **xBGR444 (0x0BGR)** and final Saturn representation is VDP2 CRAM mode 2 RGB888.
Integration Master now carries the exact portrait decode, selector-5 suppression, stamina/timer/count cells and xBGR444 conversion in `wf_hud_core.*`.

Still fail-closed/outside scope:
- wrestler-name labels;
- generic score;
- Royal elimination counter without proved consumer;
- final external renderer hookup where still pending;
- real hardware validation.

Historical R16 pure HUD core is extended/superseded by this R03 renderer-facing contract.

## Gameplay runtime

Branch: `gameplay-runtime-r01`.

Current accepted dependency:
**BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16**

- external ZIP SHA256 `0d8ddac0cd849b3c65a7c354aa6bab7606316b883e412380fc5b8fa3bce57ea9`;
- source integration commit `8589e9ef796c2ed6ed00fb9a9f07c8eda3c96f24`;
- gameplay branch HEAD `1853f5564b8a00ddd5883a68169993305a40cfae`;
- package hashes 278/278 PASS;
- canonical ROM audit PASS;
- independent strict HOST rerun PASS;
- SH-2 cross-compile PASS;
- full Saturn ELF link PASS;
- unresolved symbols = 0.

WP28 normal-frame chain now reaches:

`$00514E -> $008B3A -> $02088E -> $020958 -> $026936 -> $02983C -> $00F560 -> $00F18A -> $01C150 -> $01F914 BLOCKED`

RUNTIME-16 binds real ROM-derived owners `$00F560` and `$00F18A`.

Next mandatory owner:
**`$01F914` fixed referee scheduler**.

Still fail-closed:
- shared RNG `$21B4` path requiring exact inherited register context;
- active `$F98C` scene route;
- active `$A0E0` audio producer route;
- `$DCDE` human-controller path;
- `$10E86` movement/follow helper;
- `$1129E` state-2 random/helper path;
- direct-match and attract bootstraps.

Full normal frame: NOT COMPLETE.
Hardware: NOT TESTED.

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


## WP12 ENTRANCES/PREMATCH — R02 accepted source checkpoint

**R01 remains REJECTED / SUPERSEDED.**

Canonical active checkpoint:
- branch `entrances-prematch-r02`
- HEAD `99d316037af7508a896f0535d44dfa18775d0c47`
- ZIP SHA256 `966ac936f2a3a71b05673675b7ef041668fd99f2cf51431ccf9290dbb146f2fa`

Independent master acceptance:
- ZIP hash PASS;
- internal hashes 33/33 PASS;
- Git reproducibility 1 ahead / 0 behind PASS;
- local audited checkpoint vs GitHub 34/34 blobs PASS;
- ROM/source-table/HOST/adversarial/ASan-UBSan/static/SH-2 PASS;
- SH-2 unresolved symbols = 0.

Current WP12 status:
- TAG: PARTIAL
- ROYAL: PARTIAL
- PRESENTADORA: CLOSED structural/numeric
- ÁRBITRO PREMATCH: CLOSED structural
- HANDOFF A GAMEPLAY: CLOSED boundary / execution BLOCKED_BY_GAMEPLAY_OWNER
- HARDWARE: NOT TESTED

Mandatory external contracts remain `$C98`, canonical shared `$21B4` RNG, WP26 renderer/camera ownership and Gameplay after `$F8C`.

Still OPEN: literal wrestler-name/red-band/WWF-logo join beyond proven TAG `$98BA`; final runtime camera/raster transform; human semantic names not ROM-proven; actual gameplay execution beyond `$F8C`.

Exact checkpoint subtree imported at `wp12-r02/`.
See `docs/WP12-R02-ACCEPTANCE.md`.
Historical R01 rejection remains documented in `docs/WP12-R01-REJECTION-R3.md`.


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


## WP20 CHARACTER SELECT / JOIN-IN — R03 accepted

Canonical active checkpoint:
- external ZIP `WP20-SELECT-JOININ-R03-AUDITED.zip`
- SHA256 `c83e4f8ef0697c7b2e42f143bf35c458e42413b4f53656940afe5f0534235c09`
- bundle branch `wp20-select-joinin-r03`
- bundle HEAD `f814acaee8f7ebb9fbac153f2299314a0f0f9702`

Independent master acceptance:
- internal hashes 37/37 PASS;
- HOST PASS;
- ROM verifier PASS;
- SH-2 compile PASS;
- source contract closes all four prior structural OPEN items;
- WP22 identity cross-check closes selector 8/9 as Smash/Crush;
- exact handoff boundary reaches `$00000AC0`.

WP20 source contract status: **ROM-EXACT CLOSED / NO KNOWN STRUCTURAL OPEN**.

Still external:
- shared RNG `$21B4`;
- WP19 credits;
- WP27 input normalization/fresh edges;
- WP15/TEST03 renderer/transition/`$21E6`;
- WP12 after `$AC0`.

TEST03 visual behavior remains hardware-pass and must not be rewritten.
R03 new logic itself is **NOT HARDWARE-PASS** until integrated and tested on real Saturn.

See `docs/WP20-R03-ACCEPTANCE.md`.


## WP06 TAG EXCHANGE VISUAL — R04 accepted

Canonical active visual checkpoint:
- ZIP `WP06-TAG-EXCHANGE-VISUAL-R04-AUDITED(1).zip`
- SHA256 `bcb5c35c1d404b88b4bafc3124dd6797b62487d6edffe4ec0073c13932b7807b`
- embedded HEAD `6cb8de6721b9fff01ecb013ebc37287934bd7286`

Independent master audit:
- source/evidence hashes 11/11 PASS;
- preview hashes 4/4 PASS;
- canonical ROM identity PASS;
- HOST PASS;
- SH-2 PASS;
- ROM anchors PASS;
- WP25 metasprite audit PASS;
- preview regeneration PASS.

Visual contract for `$4C/$4D/$4E/$66`: **ROM-EXACT CLOSED**.

Critical:
- legal-role handoff owner is `$188BC`;
- `$4D` has a real callback-controlled HOLD; never replace by a guessed timeout;
- Warrior `$0062/$0063` are complete ROM interaction compositions with 23 pieces each;
- no synthetic arm patch;
- no global reverse-painter rule;
- the historical WIP160 ST_PUNCH/18-tick slap/48-tick crossing/late swap/±16 apron correction path is non-ROM-exact and must not be promoted.

Gameplay integration remains owner-dependent and hardware is NOT TESTED.
See `docs/WP06-TAG-EXCHANGE-VISUAL-R04-ACCEPTANCE.md`.