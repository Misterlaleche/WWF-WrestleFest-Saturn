# WIP96 — grapple-finisher regressions migrated to raw-$29 ownership

## Test-harness / registry closure
- Grapple-condition finisher fixtures now win the real raw `$29` struggle and enter valid `$0C/FF` HOLD before probing/starting the finisher.
- CPU Jake DDT regression now waits through the actual CPU struggle instead of assuming immediate HOLD.
- Finisher registry expectations for Smash/Crush are updated to the later exact mappings:
  - Smash raw `$43`, descriptor index 77, ROM `$18518`, 7 poses, exact damage 24.
  - Crush raw `$44`, descriptor index 78, ROM `$1863C`, 6 poses, exact damage 24.
- Exact-damage registry expectation is synchronized with the canonical finisher records.

## Runtime
- No gameplay change in this checkpoint.
- Existing high-confidence finisher descriptor/callback identities remain unchanged.
- Earthquake candidate remains gated.

## Tests
- `finisher_registry` PASS.
- `finishers` PASS.
- `cpu_finishers` PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP96-source.zip`
- SHA256 `624250901df4725f600c1ffb7762e6cc2d0a987fe6ac279331fe84368ee214e5`

## Next
- Audit paired throw/finisher animation coverage after the exact HOLD path; remove generic visuals only for raw identities with proven descriptors/compositions.
