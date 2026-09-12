# Frontend TITLE Presentation Bridge R18

Authority:
- original wwfwfest.zip;
- WP19/R11 Operator/Credits;
- accepted INTRO R17 hardware-pass handoff;
- MODE+ROSTER TEST03 hardware-pass handoff;
- Integration Master ownership policy.

## Purpose

R18 closes one ownership ambiguity before integrated frontend assembly:

INTRO does not own CREDIT/INSERT COIN/START prompt logic.

R18 supplies a pure master-side TITLE presentation view derived from the authoritative WP19/R11 credit state. It does not duplicate credit logic and does not mutate credit state.

## Contract

Input:
- wf_credit_state_t from WP19/R11;
- packed operator config;
- existing TITLE prompt counter;
- dedicated-2P second-prompt phase flag.

Output:
- credits value for FRONTEND CREDIT:x rendering;
- partial-credit value;
- exact WP19 prompt ID;
- exact WP19 blink phase bit;
- dedicated-2P phase echo.

The bridge intentionally does not assign glyphs, coordinates, palette, timing, or renderer writes that have not yet been bound from frontend graphics authority.

## Ownership

- INTRO R17: boot/title-build visual sequence only.
- WP19/R11: credit state, coin conversion, payment and prompt selection.
- R18 Frontend: presents the already-authoritative credit/prompt state.
- MODE/ROSTER: remains the existing TEST03 hardware-proven behavior/contract.
- WP12: blocked; R01 rejected by R3.

Forbidden:
- private INTRO credit counter;
- hard-coded CREDIT:0;
- local coin model;
- test-harness START->MODE shortcut;
- assuming TITLE.ANM 451 stored frames are native arcade duration.

## Validation

Host:
PASS with -std=c11 -Wall -Wextra -Werror -pedantic.

SH-2:
PASS with -Wall -Wextra -Werror -ffreestanding -fno-builtin -m2.

Relocatable aggregate with WP19:
PASS.

Only unresolved symbols are libc-level memcpy/memset from WP19/R18; no new project-owner unresolved symbol is introduced.

No BIN/CUE.
Hardware PASS is not claimed for R18 until integrated build is tested on real Saturn.
