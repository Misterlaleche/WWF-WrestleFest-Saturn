# WIP97 CHECKPOINT — Front-grapple submissions through raw-$29 ownership

## Scope
Migrates the front-grapple submission regression from the obsolete instant-HOLD fixture to the current exact grapple entry path.

## Changes
- `tools/test_submissions.c` now wins the exact raw `$29` struggle before testing a submission.
- The test requires recovered ownership shadow `$0C/$FF` before committing the action.
- The live category `$09` P path must emit raw `$15` before entering the existing submission semantic bridge.
- Covers all three front-grapple submission identities currently present in the native move rota:
  - Sgt. Slaughter — Cobra Clutch
  - Earthquake — Bear Hug
  - Ted DiBiase — Million Dollar Dream
- Verifies zero-stamina submission victory and deterministic CPU-victim escape behavior.
- `tools/check_host.py` gives the `submissions` target the modern `arcade_action_map.c` + `arcade_pair_state.c` dependencies required by the exact grapple path.

## Fidelity boundary
- ROM-EXACT: grapple struggle raw `$29`, `$0C/$FF` ownership terminal, category `$09` P -> raw `$15` input identity.
- PROVISIONAL: semantic bridge raw `$15` -> current four-move grapple rota and the 18-tick submission drain cadence / escape thresholds.
- No claim is made that raw `$15` itself has been semantically identified as a specific submission callback.

## Validation
PASS:
- `submissions`
- `test377_grapple_cycle_effort`
- `test378_grapple_raw29_ownership_runtime`
- `test379_grapple_category9_live_input`
- `test380_grapple_reversal_raw_validity`
- `test381_grapple_raw15_raw17_profiles`
- `test382_grapple_raw15_raw17_producer_audit`

Source ZIP SHA256: `c75d54426a76b3fcbfc0c59d787fa36feaf84c8b3611316d1774bbfac0605874`

No BIN/CUE generated.
