# RING / ROPES / CAMERA R02 — Integration Master acceptance

External checkpoint:
`RING-ROPES-CAMERA-R02-ROM-PERFECT-AUDITED.zip`

External ZIP SHA256:
`20a54f5994de27d1e0114c62c9748e94d3b2a8138551eb4aaa8d687f7b19c44c`

The external package explicitly was not pushed to a Git branch. Its `BRANCH-HEAD.txt` only records historical baseline `main@3287ac70c2010b3706aaf3a76003e56f7e1fa25b`. Integration Master therefore establishes the reproducible checkpoint by importing the audited consumer, test and ROM verifier directly.

## Independent re-audit

- canonical `wwfwfest.zip` main 68000 SHA1:
  `a726c46f350723ea73158bc78055d17d418b14e2`;
- external ZIP SHA256 computed independently: PASS;
- external file-sha256 manifest: 5/5 PASS;
- ROM auditor rerun against current `wwfwfest.zip`: PASS;
- independent init-byte inspection: PASS;
- HOST strict `-std=c11 -Wall -Wextra -Werror -pedantic`: PASS;
- SH-2 GCC 13.2.0 `-Wall -Wextra -Werror -ffreestanding -fno-builtin -m2`: PASS;
- WP21 camera/shake/HUD cross-check: PASS;
- WP26 source-routing/painter contract cross-check: PASS.

## Exact fixed side-rope identity

Four 0x30-byte records:
- `$1C1134` left primary;
- `$1C1164` left secondary;
- `$1C1194` right primary;
- `$1C11C4` right secondary.

ROM init:
- left X `$01C0`;
- right X `$0330`;
- Y `$0178`;
- Z `$0140`;
- gfx set `$000E`;
- class `$0001/$0003`;
- right orientation `$8000`.

Static selectors:
- primary `$02`;
- secondary `$0A`.

The world-geometry resolver independently proves:
- actor `+$37.bit1` = low-X / left boundary;
- actor `+$37.bit0` = high-X / right boundary.

This closes the previous side-alias ambiguity without using artwork guesses.

## Consumer / cadence

Scheduler `$10120` dispatches `WORD(+$1C)&$F` through `$10172`.

State 1:
- primary: 03/04/05/06/02/04, 6 ticks each;
- secondary: 07/08/0A/09/0A/08, 6 ticks each;
- 37 renders through terminal clear; static restore update 38.

State 2:
- first phase 16 ticks, remaining phases 6;
- 47 renders through terminal clear; restore update 48.

State 3:
- selectors 04/05/06 with 10/10/6;
- 27 renders through terminal clear; restore update 28.

State 4:
- selectors 06/02 with 14/12;
- 27 renders through terminal clear; restore update 28.

Terminal clear intentionally leaves the last selector visible for that render; state0 restores the static selector on the next scheduler update.

## Producer evidence

State 1:
- common producer `$10FC6`;
- 23 demonstrated callsites: 17 absolute JSR + 6 BSR;
- plus state4:22 direct writes.

State 2:
- helper `$11D56`.

State 3:
- raw action `$3F` callback `$1836E`;
- left/right primary selected by actor `+$06 < $0280`.

State 4:
- raw action `$72` callback `$19FDA`;
- primary side selected by actor `+$33.bit7`.

Human semantic names beyond what WP22 proves remain fail-closed.

## North / south horizontal ropes

No equivalent fixed state1..4 quartet is demonstrated for the low/high-Y boundary bits.

The ROM does demonstrate bank-14 rope material inside wrestler ring-transition compositions:
- raw `$69`, poses 32..38;
- raw `$6C`, poses 39..43;
- poses 32..41 contain bank 14.

Therefore horizontal crossing visuals must not be replaced with a synthetic global north/south rope displacement.

## Renderer / camera contract

- side-rope object material is SPRITES -> VDP1;
- fixed/background rope art remains in its original VDP2 world plane;
- preserve source command order; no renderer-side semantic depth heuristic;
- preserve WP21 capture-proven painter relation;
- world camera/shake moves world layers;
- HUD/FG0 stays screen-fixed;
- shake remains after base clamp and before scroll upload;
- do not re-clamp the shaken display offset in the same update.

## Superseded behavior

The old synthetic WIP/R01 deformation:
`d = 6 - (dx * 6 / 64)` and traversal clamp-style offsets
is **forbidden** as arcade authority.

Rope deformation is selector/timer animation of the four ROM records, not a synthetic per-piece pixel curve.

## Ownership

Gameplay/Collision owns:
- actor `+$36/+$37`;
- action/state selection;
- exact producer reachability.

Ring/Ropes owns:
- exact four-record consumer;
- selector/timer cadence;
- side identity and restore.

WP25 owns bank-14 pixels/palettes.
WP26 owns Saturn rendering.
WP21 owns camera/shake composition.

## Verdict

**Fixed side-rope producer/consumer/cadence/restore: ROM-EXACT CLOSED.**

This does NOT mean the entire ring/camera/gameplay subsystem is hardware-validated or globally closed.

Still external:
- Gameplay producer integration;
- final renderer hookup;
- real Saturn validation.

Hardware: **NOT TESTED**.
BIN/CUE: **NO**.
