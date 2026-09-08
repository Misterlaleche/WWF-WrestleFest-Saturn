# WIP100 CHECKPOINT — Finisher grapple context after exact ownership

## Changes
- Migrated the grapple helper in `test_finisher_runtime.c` from instant HOLD to the exact raw `$29` struggle and `$0C/$FF` ownership path.
- Grapple-context finisher probes can no longer pass by bypassing ownership.
- Updated `check_host.py` dependencies for the finisher runtime fixture.
- Corrected stale graphics expectation: Crush-vs-Smash is now READY because the current finisher asset table contains the recovered partner-specific frames.
- Jake-vs-Jake remains deliberately MISSING_GRAPHICS because same-character role tagging is still unsafe.

## Validation
PASS: finisher_runtime, finishers, Test383, grapple_rota, submissions, irish_whip.

Source ZIP SHA256: `5791751bd3a7dfd00f1be21f43e8e4c4a00690e28beefd482c9d904c5b519eec`

No BIN/CUE generated.
