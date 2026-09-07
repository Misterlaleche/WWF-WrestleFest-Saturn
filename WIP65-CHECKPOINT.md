# WIP65 — live callback +$42 arm/clear ordering without unsafe held release

Status: targeted Ground Actions runtime checkpoint. No BIN/CUE. No ROM files.

## Runtime change
- `ground_attack_update()` now executes the already-proven prone callback gate helper after the floor/contact resolver.
- Exact order is now:
  1. resolve contact from persistent `actor32/raw_z/probe42`;
  2. apply exact floor correction;
  3. identify the current `$FF00` hold phase;
  4. run `arcade_prone_hold_callback_step()` so immediate probe writes arm the next resolver pass and consumed contact clears the probe;
  5. pass release to the action clock only if `arcade_prone_hold_live_release_ready()` says every blocker for that action/phase is closed.

## Safety
- No held action is currently promoted live because the blocker matrix still rejects all eleven held occurrences.
- raw `$10` remains blocked by actor32 provenance only.
- raw `$0E` phase2 has the same local status, but phases0/1 still have inherited probe blockers.
- 32-tick fallback remains.
- `tick==12` remains unchanged.

## Why this is safe
- WIP31/Test300 already established resolver-before-callback ordering.
- WIP39/WIP40 established exact callback `+$42` arm/consume/clear semantics.
- This checkpoint activates only that temporal shadow mutation; it does not infer missing Z/probe/actor32 inputs and does not globally release `$FF00` holds.

## Tests
- Test319 PASS.
- Test321 PASS.
- Test330 PASS.
- Test334 PASS.
- Test335 PASS.
- Test336 PASS.
- Added Test349 runtime ordering/gating guard: PASS.
- `python3 -m py_compile tools/check_host.py` PASS.
- ROM-dependent Test318 was not rerun because `program.bin` is intentionally absent from the persisted source; its previously retained byte audit remains the evidence source.

## Persistence
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP65-source.zip`
- SHA256: `8ed86f9b090a7b895e965a3fce74a912bc534a9bef6f62098b1f953e86bc8798`
- No ROMs. No BIN/CUE.

## Fidelity
- Callback probe literals and clear ordering: ROM-EXACT from retained audits.
- Missing inherited inputs remain OPEN and explicitly blocked.

## Next
With callback-side probe timing now live, close the inherited inputs in dependency order: first raw `$0E` phases0/1 and raw `$13` inherited `+$42`; then raw `$14/$35` inherited Z; then raw `$0B/$23/$46/$47` inherited Z+probe. Do not remove `tick==12` until contact-driven damage timing is independently closed.
