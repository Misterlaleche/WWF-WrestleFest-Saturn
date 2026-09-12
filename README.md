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
- R15 Instructional Hand Prompt visual contract;
- R16 Gameplay HUD core;
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

WP12 R01 is **REJECTED / NOT INTEGRABLE** by third independent audit R3 against original ROM authority.

Correct state: TAG BLOCKED, ROYAL BLOCKED, presenter PARTIAL, prematch referee MISSING, gameplay handoff BLOCKED. Host and SH-2 compilation PASS do not make the implementation ROM-EXACT.

Next checkpoint must be WP12 R02 rebuilt from `wwfwfest.zip` and satisfying all R3 gates. See `docs/WP12-R01-REJECTION-R3.md`.

## Current hard dependencies

Gameplay remains on `gameplay-runtime-r01`.
Observed dependency:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-15` — source `72de727a77603c0a81f552fa059c0d2072cd249b`, gameplay branch documentation HEAD `7f40fde30cb7af32cf95cd4d9469884a342da8b8`.

RUNTIME-15 closes WP28 `0x020958` and binds the audited WP21 camera phases `0x026936/0x02983C`; the next fail-closed point is `0x00F560`. Shared `$21B4` RNG/order and attract CPU override `$1F7F0` remain gameplay-owned.

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
- WP16 prompt has no timing/trigger/button semantics until proven;
- WP18 HUD must not invent wrestler names, generic score or Royal elimination counter;
- INTRO must never own a private credit counter or hard-code `CREDIT:0`;
- `TITLE_COUNT=451` is asset representation only, never arcade-native duration;
- no BIN/CUE unless explicitly requested.

## Mandatory reading

See `HANDOFF.md` for the complete ordered integration reading list, `docs/AUDIO-RUNTIME-HANDOFF-R02.md` for current audio authority, `docs/INTEGRATION-SELF-AUDIT-A01.md` for the master self-audit, and `docs/WP12-R01-REJECTION-R3.md` for the authoritative WP12 R01 rejection/R02 gate.
