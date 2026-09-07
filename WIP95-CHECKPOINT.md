# WIP95 — grapple regression fixtures migrated to exact struggle ownership

## Test-harness closure
- `grapple_rota` no longer assumes GRAB -> immediate HOLD. It now traverses raw `$29` struggle twice, verifies valid `$0C/FF` ownership, then presses P (exact category `$09` -> raw `$15`) before testing the still-PROVISIONAL four-move semantic rota.
- `grapple_counters` now lets the CPU win the real struggle before testing native reversal/escape windows, and lets P1 win the real struggle before testing the provisional CPU reversal path.
- `power_grapple` keeps unilateral Power Up takeover behavior, but the equal-Power case now correctly traverses raw `$29` instead of expecting obsolete immediate ownership.

## Runtime
- No gameplay change in this checkpoint.
- This restores meaningful regression coverage around WIP89–WIP94 rather than weakening the new runtime to satisfy stale fixtures.

## Tests
- `grapple_rota` PASS.
- `grapple_counters` PASS.
- `power_grapple` PASS.
- Tests379–382 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP95-source.zip`
- SHA256 `c3ea7be798e7094bc9ad38ccf6cdddfded03a0207f108a8778757e078e65dfdb`

## Next
- Continue front-grapple/throw closure. Exact raw `$15/$17` callback semantics remain blocked; advance to independently proven paired throw/finisher identities and remove generic presentation/animation only where raw-action identity is established.
