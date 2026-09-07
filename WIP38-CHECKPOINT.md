# Test255 WIP38 — exact floor correction applied to native raw-Z shadow

## ROM-EXACT
- Test317 pins all four audited `+$37 bit4` floor-producer sequences through the final `ADD.W +$42,+$3C` operation.
- The contact gate is evaluated from the 16-bit candidate `+$0E + +$42`.
- On the contact side the ROM algebra reduces to `+$3C = threshold - +$0E`, because it first subtracts the probed candidate then adds `+$42` back.
- The following main fighter update applies `+$3C` to `+$0E` before the action callback, so the corrected raw-Z lands exactly on the producer threshold (`$140` or `$100`).

## Native integration
- Added `arcade_floor_resolution_t`, `arcade_floor_resolve()` and `arcade_floor_resolve_base_context()`.
- `fighter_t` now retains `contact37_shadow` from the latest resolver pass.
- `ground_attack_update()` now applies the exact `+$3C` correction to `raw_z_shadow` before ticking the prone action callback clock.
- `$FF00` release remains deliberately disabled: correction/contact are now live, but callback-local `+$42` arm/clear ordering still has to be modeled before a held frame may advance.

## Tests
- Test317: byte-pinned floor probe/correction algebra and boundary cases.
- Test315 continues to pass.
- Test290 re-run passes after the native raw-Z correction integration.

## Still open before `ground timing provisional = 0`
- Model exact callback `+$42` arm-on-current-callback / affect-next-resolver / clear-on-consumption semantics.
- Resolve non-probe-writing held gates and inherited raw-Z/probe state for all nine callback-held prone raws.
- Remove the legacy 32-tick fallback only after every held phase has an exact physical release path.
- Raw damage/callback side effects and PICK UP victim handoff remain provisional.

No ROM image included. No BIN/CUE generated.
