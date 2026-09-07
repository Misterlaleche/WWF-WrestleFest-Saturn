# Test255 WIP39 — exact callback probe arm/consume/clear cycle

## ROM-EXACT
- Four reachable callback-held prone gates write an explicit `+$42` probe immediately before their `BTST #4,+$37(A0)` gate:
  - raw `$0E`, held phase 2: `-$18`, clear at `$14002`;
  - raw `$10`: `-$20`, clear at `$142E2`;
  - raw `$14`: `+$20`, clear at `$14C2A`;
  - raw `$35`: `+$20`, clear at `$17814`.
- By the already-pinned main update order, the callback write occurs after the current resolver pass. It therefore arms the NEXT resolver pass.
- If contact bit4 was already latched by that next resolver, the callback falls through the gate, clears `+$42`, and continues. If bit4 is clear, the callback branches away and leaves the probe armed.

## Code/tests
- Added `arcade_prone_probe_callback_step()` as a ROM-exact side-effect helper for these four probe-writing held phases.
- Test318 pins write, contact gate, and clear addresses byte-for-byte and verifies arm-vs-consume semantics.
- WIP38 floor correction remains active in native shadow state; `$FF00` release remains disabled in gameplay until non-probe-writing held gates are closed.

## Still open
- Determine exact physical source for held gates without an immediate probe write: raw `$0B`, raw `$0E` phases 0/1, raw `$13`, raw `$23`, raw `$46/$47`.
- Raw `$23` and `$46/$47` do have later `+$42=+$20` paths elsewhere in their callbacks; these must not be incorrectly assigned to their initial held gate.
- Only after every held phase has an exact release source may the native action clock receive release=true and the 32-tick fallback be removed.
- Damage/callback side effects and PICK UP handoff remain provisional.

No ROM image included. No BIN/CUE generated.
