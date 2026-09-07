# Test255 WIP37 — callback-held prone release-gate map

## ROM-EXACT
- Every reachable prone `$FF00` occurrence is now mapped to the exact `BTST #4,+$37(A0)` gate that controls that held phase.
- Raw `$0E` has three distinct held phases/gates: `$13DF8`, `$13EF2`, `$13FF2`.
- Single held gates: `$0B->$13AA8`, `$10->$142D8`, `$13->$14A72`, `$14->$14C1A`, `$23->$15DFE`, `$35->$1780A`, `$46/$47->$144F0`.
- Probe-writing held gates carry the exact callback `+$42` literals: raw `$0E` phase2 `-$18`, raw `$10` `-$20`, raw `$14` `+$20`, raw `$35` `+$20`.
- By Test300 update order, these callback writes cannot affect the contact bit tested in that same callback invocation; they arm the NEXT resolver pass.

## Code
- Added `arcade_prone_hold_gate(raw, hold_phase, ...)`.
- Added `fighter_t.prone_probe_armed` scaffold for next-tick probe semantics.
- Added Tests314-316 to the host-test registry.

## Tests
- Test316 pins every held gate and every held-gate probe literal byte-for-byte against `program.bin`.
- Test290, Test314 and Test315 remain passing from WIP36.

## Still open before live release
- Do not yet release `$FF00` merely because contact37 is true: the native victim/attacker raw-Z lifecycle outside the held window must be mapped so the contact predicate receives the same inherited Z as the arcade.
- Damage/callback side effects and PICK UP victim handoff remain provisional.

No ROM image included. No BIN/CUE generated.
