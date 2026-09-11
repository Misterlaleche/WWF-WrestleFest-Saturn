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

Implemented master-side candidates:
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
- R13 Campaign Interstitials.

These candidates do not automatically inherit HARDWARE-PASS.

## Current hard dependencies

Gameplay remains on `gameplay-runtime-r01`.
Observed master dependency:
`BLOCK-GAMEPLAY-MASTER-01-RUNTIME-13` @ `91f4c697bec0b04678cc1e811b4ee60e606d7131`.

It still fails closed at WP28 phase `0x02088E`; shared `$21B4` RNG/order and attract CPU override `$1F7F0` remain gameplay-owned.

INTRO/PRESENTATION is developed separately on `intro-presentation-r01`.

## Important integration rules

- never replace CLEAN-R01/master `main.c` with an isolated harness;
- MODE+ROSTER imports TEST03 behavior, not its architecture;
- WP24 is the only audio backend;
- attract demo uses ordinary gameplay/CPU, never synthetic joystick/button autoplay;
- no WIP160/WIP175/WIP176 as canonical gameplay base;
- no synthetic ten-match campaign ladder;
- no BIN/CUE unless explicitly requested.

## Mandatory reading

See `HANDOFF.md` for the complete ordered integration reading list.
