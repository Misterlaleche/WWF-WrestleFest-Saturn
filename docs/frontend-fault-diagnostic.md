# Frontend failure diagnostics

An adapter error can return without setting `frontend.blocked_pc` or
`frontend.blocked_dependency`. Previously `frontend_step()` then called
`fail(0, 0)`: gameplay stopped, but both public fault fields looked clear.
The stock B post-attract capture demonstrated this state. Its actual driver
was fatal at PC `$52BE` with dependency `$83A4`; the last observed phase 3
predated that failure. The final CPU stack was ordinary SDK music service,
and the examined heap block headers were valid.

Both run and flush failures now preserve an existing adapter owner and
dependency. An absent owner falls back to the current driver PC. An absent
dependency falls back to the driver's dependency only when the driver is
fatal. Remaining unknown values use explicit tags outside the 24-bit arcade
address space:

| Value | Meaning |
| --- | --- |
| `0x46524E54` (`FRNT`) | Neither adapter nor driver supplied an owner |
| `0x5354ssss` (`ST`) | Low 16 bits contain the adapter return status; this is not a ROM address |

For example, a bare `WF_BAD_STATE` at `$52BE` reports owner `000052BE` and
data `53540002`. A genuine dependency remains unchanged. Flush returning
`WF_BLOCKED` without an adapter dependency remains a retry, not a fatal
error. No game state, input, timing, or diagnostic structure layout changes.

The existing `fail()` path is retained: drain pending publication, disable
the game presenter, restore the SDK NBG0 font/map/palette, then print the
diagnostic. Saved VDP2 state from the stock B failure contained a complete
legible diagnostic plane with `Owner: 00000000` and `Dato: 00000000`; this
change repairs its information, not its graphics.

Run `python3 tests/test_main_frontend_fault.py` in the Linux validation
environment. It compiles the actual `frontend_step()` and `fail()` bodies
with the real frontend types, controlling only their runtime/graphics
boundaries. Eleven ASan/UBSan cases cover run/flush errors, existing errors,
driver fatal state, stale nonfatal dependencies, missing PC, retry, successful
handoff, unchanged game state, and graphics restoration before printing.
The baseline at commit `f5b22d4` fails the first missing-owner assertion.

The isolated Royal B link with only this change adds 64 resident bytes:
`_end=0x060DBFA4` becomes `0x060DBFE4`, with 28 bytes before the unchanged
SDK work-area limit. This includes BSS and is not runtime heap or stack
headroom. The simultaneous attract continuation correction and R02C require
their own clean link and target validation. No audio change is included.
