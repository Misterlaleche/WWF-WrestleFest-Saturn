# WP12 R02 — independent self-audit

## Verdict

**SOURCE CHECKPOINT: PASS WITH EXPLICIT EXTERNAL CONTRACTS / OVERALL WP12 PARTIAL / HARDWARE NOT TESTED.**

This verdict means the source model and ordered integration contract pass the
current ROM/FINAL evidence.  It does not claim that unresolved graphics/camera
joins are closed or that real Saturn hardware has been tested.

## Audit passes

- fresh `wwfwfest.zip` main program reconstruction:
  SHA1 `a726c46f350723ea73158bc78055d17d418b14e2`;
- direct byte-signature audit of `$BD2`, `$C08`, `$A654`, `$A814`, `$AAEA`,
  `$ACB6/$AD66`, `$10718`, `$1086C`, `$A778`, `$8EB6`, `$8AE6`, `$DEE`;
- source-table audit against ROM for `$10708`, `$10ACC/$10AE4`, `$AC4A/$AC56/$AC9E`,
  `$A7B4`, `$AD6C/$AD9C` programs and `$DEE`;
- natural-duration regression: TAG baseline `1441` updates for IDs `0,1,2,3`; Royal `64`;
- host compile `-std=c11 -Wall -Wextra -Werror -pedantic`;
- host adversarial tests;
- ASan/UBSan host tests;
- GCC `-fanalyzer` PASS; Clang warning/static-analyzer PASS (unsafe-buffer heuristic suppressed after explicit bounds audit);
- SH-2 GCC 13.2.0 compile `-Wall -Wextra -Werror -ffreestanding -fno-builtin -m2`;
- SH-2 relocatable link;
- zero unresolved symbols in the combined SH-2 module;
- ownership scan: no `main.c`, no gameplay scheduler, no collision/CPU/audio backend;
- renderer audit: source exports machine coordinates only; no synthetic camera math.

## R3 gates

All R3 implementation gates are either implemented or represented as an explicit
fail-closed external contract.  In particular, `$C98`, `$21B4`, `$98BA`, canonical
renderer and gameplay `$F8C` ownership are not replaced by provisional substitutes.

## New R02 audit finding

The direct ROM pass additionally captured the one-time `$A654` referee X adjustment
when `$1C0163==0`.  This behavior is implemented and tested.

## Still OPEN

See `docs/OPEN-BLOCKED.md`.  These OPEN items are intentionally not hidden by dummy
assets, guessed camera positions, or invented timings.
