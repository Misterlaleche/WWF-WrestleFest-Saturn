# WWF WrestleFest arcade -> Sega Saturn

Private development repository for a native ROM-EXACT Sega Saturn reimplementation of Technos WWF WrestleFest (1991).

## Authority

1. original `wwfwfest.zip`;
2. audited FINAL ROM-EXACT packages;
3. Saturn/Yaul technical authority;
4. current Saturn source/tests.

Unknown behavior remains OPEN/BLOCKED.

## Hardware-proven blocks

- MODE+ROSTER TEST03 — HARDWARE-PASS on Saturn NTSC-U + MODE.
- WP24 audio transport — HARDWARE-PASS.
- INTRO ROM-EXACT FINAL — HARDWARE-PASS on Saturn NTSC-U + MODE; package SHA256 `0fb488514ae233c429058834b8ced73c757b0224d819d6ba4dc3015042f81c98`.

## Integration master

Branch: `integration-master-r01`.

Implemented master-side candidate family:
- R02 CDFS runtime loader;
- R03 final-memory manifest/validator;
- R04 WP30 ownership resolution;
- R05 Scene Flow Master;
- R06 Records/TOP RANKING;
- R07 Ending Credits;
- R08 Attract Biography;
- R09 Attract Demo root contract;
- R10 Continue/Game Over;
- R11 Operator/Credits/Payment;
- R12 Continue/Payment bridge;
- R13 Campaign Interstitials;
- R14 Frontend Transition Engine;
- R15 historical Instructional Hand Prompt visual contract (superseded);
- R19 WP16 R02 runtime presentation contract (superseded visually);
- R20 WP16 R06 corrective ROM-exact presentation contract;
- R16 historical Gameplay HUD core;
- WP18 HUD Integration R03 active renderer-facing HUD authority;
- R17 accepted INTRO hardware-pass handoff;
- R18 Frontend TITLE credit/prompt presentation bridge.

R10-R13 host validation has been rechecked; R13 also cross-compiles for SH-2.
R14-R16 host + SH-2 compile/test gates PASS.

Self-audit A01: PASS WITH TWO CORRECTED FINDINGS. R09 transition semantics and R13 campaign control flow were corrected against original ROM authority before the integrated hardware build.

R18 host + SH-2 gates PASS and introduces no private credit model. Frontend glyph/coordinate renderer binding and integrated hardware validation remain open.

These candidates do not automatically inherit HARDWARE-PASS.

## Audio authority

Canonical runtime-audio handoff is now:
**WWF-RUNTIME-AUDIO-HANDOFF-R02-REAUDITED.zip**

SHA256:
`49335637b8ba6d26b361c88e3886f620e9f52c076df3d4bfb44accf3701ccb5c`

R02 supersedes R01. Producer semantics remain 173 total / 14 integration-ready / 4 owner-scope-only / 155 unresolved. R02 fixes handoff provenance/completeness and contains the full byte-identical WP24 source/test snapshot.

## WP12 ENTRANCES/PREMATCH

Canonical active source checkpoint is now **WP12 R02 FINAL AUDITED**:
- branch `entrances-prematch-r02`
- HEAD `99d316037af7508a896f0535d44dfa18775d0c47`
- ZIP SHA256 `966ac936f2a3a71b05673675b7ef041668fd99f2cf51431ccf9290dbb146f2fa`

R01 remains rejected/superseded.

R02 passes ROM/source-table/HOST/adversarial/sanitizer/static-analysis/SH-2 gates with zero SH-2 unresolved symbols and is imported byte-exact under `wp12-r02/`.

WP12 remains deliberately PARTIAL: TAG and Royal are partial; presenter/referee structural contracts are closed; the exact handoff boundary reaches `$F8C` but execution thereafter remains Gameplay-owned. The literal wrestler-name/red-band/WWF-logo join and final runtime camera/raster transform remain OPEN.

See `docs/WP12-R02-ACCEPTANCE.md`.

## Current hard dependencies

Gameplay remains on `gameplay-runtime-r01`.

Current accepted dependency:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-16`

Source integration commit:
`8589e9ef796c2ed6ed00fb9a9f07c8eda3c96f24`

Gameplay branch HEAD:
`1853f5564b8a00ddd5883a68169993305a40cfae`

RUNTIME-16 advances through `0x00F560 -> 0x00F18A -> 0x01C150` and fail-closes at `0x01F914` fixed referee scheduler. Full normal frame remains incomplete. Shared RNG/order and unresolved helper/bootstrap paths remain Gameplay-owned.

INTRO/PRESENTATION visual sequence is now accepted as HARDWARE-PASS. Its `TITLE.ANM` has 451 stored asset frames; this is not a claim that the native arcade sequence lasts 451 frames. Frontend still owns CREDIT/INSERT COIN/START presentation and the continuation into attract/mode.

## Important integration rules

- never replace CLEAN-R01/master `main.c` with an isolated harness;
- MODE+ROSTER imports TEST03 behavior, not its architecture;
- WP24 is the only audio backend;
- R02 runtime-audio handoff supersedes R01;
- only the 14 R02 `INTEGRATION_READY` producers may be wired semantically;
- `0x1F` remains fail-closed until exact lifecycle is proven;
- attract demo uses ordinary gameplay/CPU, never synthetic joystick/button autoplay;
- no WIP160/WIP175/WIP176 as canonical gameplay base;
- no synthetic ten-match campaign ladder;
- WP16 R06 is active: trigger/presentation cadence and fresh-edge behavior remain closed, corrected bbox is [10,4,71,94], and the pictured button is still not proven to mean specifically A, B or A+B;
- WP18 HUD must not invent wrestler names, generic score or Royal elimination counter;
- INTRO must never own a private credit counter or hard-code `CREDIT:0`;
- `TITLE_COUNT=451` is asset representation only, never arcade-native duration;
- no BIN/CUE unless explicitly requested.

## Mandatory reading

See `HANDOFF.md` for the complete ordered integration reading list, `docs/AUDIO-RUNTIME-HANDOFF-R02.md` for current audio authority, `docs/INTEGRATION-SELF-AUDIT-A01.md` for the master self-audit, and `docs/WP12-R01-REJECTION-R3.md` for the authoritative WP12 R01 rejection/R02 gate.


## WP16 authority

Current WP16 checkpoint: `WP16-HAND-PROMPT-R06.zip`
SHA256 `c0d5fe5a135cb1c89e1a79023351a6e90f774bd0e2f0bf687e413eb736b1b3c8`.

R06 supersedes R19/R02 visually after direct ROM evidence proved the final TEXT row was missing from the old derived assets. Corrected pose hashes are `352072fb...` and `36ca81e2...`, bbox is `[10,4,71,94]`, and the exact fixed TEXT transform maps to VDP2 NBG2 priority 4.

Runtime semantics remain the proven fresh-edge/global-request contract. Gameplay producer insertion and real-hardware validation remain separate.


## WP20 authority

Current WP20 source authority:
**WP20-SELECT-JOININ-R03-AUDITED**

SHA256:
`c83e4f8ef0697c7b2e42f143bf35c458e42413b4f53656940afe5f0534235c09`

R03 closes the previously open timeout/AUTO/terminal64 policy, late joins, selector IDs 8/9 and simultaneous occupancy arbitration, and corrects R02 control-flow defects. Selector slots 8/9 resolve through `$64CC` to internal `0A/0B` = Demolition Smash/Crush.

The source contract is ROM-EXACT closed with no known structural WP20 OPEN, but newly corrected R03 logic is not yet hardware-pass. TEST03 visual behavior remains the hardware-proven renderer contract.


## WP18 HUD authority

Current checkpoint:
`WP18-HUD-INTEGRATION-R03-AUDITED(1).zip`
SHA256 `8c8b46fa3b1f60f8e76fab2f76e6692177f9171e93712f9a946a0b15f148f19f`.

R03 supersedes R01/R02 and fixes the confirmed red/blue inversion by treating arcade colors as xBGR444 (`0x0BGR`) before VDP2 CRAM mode 2 RGB888 conversion.

## WP06 TAG exchange visual authority

Current checkpoint:
`WP06-TAG-EXCHANGE-VISUAL-R04-AUDITED(1).zip`
SHA256 `bcb5c35c1d404b88b4bafc3124dd6797b62487d6edffe4ec0073c13932b7807b`.

R04 closes the ROM-exact visual contract for `$4C/$4D/$4E/$66`. Warrior `$0062/$0063` are complete 23-piece interaction compositions; no synthetic arm patch is allowed. Gameplay must consume the contract instead of the old WIP160 synthetic TAG path.


## Ring / ropes authority

Active side-rope checkpoint:
**RING-ROPES-CAMERA-R02-ROM-PERFECT-AUDITED**

SHA256:
`20a54f5994de27d1e0114c62c9748e94d3b2a8138551eb4aaa8d687f7b19c44c`

The four fixed side-rope records at `$1C1134/$1C1164/$1C1194/$1C11C4` use exact ROM selector/timer animation. Historical synthetic pixel-offset rope deformation is forbidden.

This closes the side-rope consumer/cadence/restore contract, not the complete ring/gameplay subsystem. Gameplay producer integration and real-hardware validation remain pending.
