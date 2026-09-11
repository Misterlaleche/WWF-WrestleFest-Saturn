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
- R16 Gameplay HUD core.

R10-R13 host validation has been rechecked; R13 also cross-compiles for SH-2.
R14-R16 host + SH-2 compile/test gates PASS.

These candidates do not automatically inherit HARDWARE-PASS.

## Current hard dependencies

Gameplay remains on `gameplay-runtime-r01`.
Observed dependency:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-14` @ `9f2ad43b843570653a7292a45ee86b7740e98fa7`.

RUNTIME-14 closes WP28 phase `0x02088E`; the next fail-closed point is `0x020958`. Shared `$21B4` RNG/order and attract CPU override `$1F7F0` remain gameplay-owned.

INTRO/PRESENTATION is developed separately on `intro-presentation-r01`.

## Important integration rules

- never replace CLEAN-R01/master `main.c` with an isolated harness;
- MODE+ROSTER imports TEST03 behavior, not its architecture;
- WP24 is the only audio backend;
- attract demo uses ordinary gameplay/CPU, never synthetic joystick/button autoplay;
- no WIP160/WIP175/WIP176 as canonical gameplay base;
- no synthetic ten-match campaign ladder;
- WP16 prompt has no timing/trigger/button semantics until proven;
- WP18 HUD must not invent wrestler names, generic score or Royal elimination counter;
- no BIN/CUE unless explicitly requested.

## Mandatory reading

See `HANDOFF.md` for the complete ordered integration reading list.
