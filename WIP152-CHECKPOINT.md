# WIP152 CHECKPOINT — live actor32 provenance shadow and A2 role-swap fix

## Scope
Continue Ground Actions after WIP151 by carrying exact actor32 event provenance in each native fighter without aliasing native states to ROM states.

## ROM-EXACT runtime changes
`fighter_t` now stores:
- `actor32_shadow_valid`: true only after an exact mirrored actor32 event;
- `actor32_last_rom_event`: exact ROM address of that last mirrored event.

The existing event dispatcher now records both fields whenever a proven one-to-one event is applied.

### `$214A4` correction
A pre-existing wiring bug was found: `actor32_linked_role_swap_set()` called the generic A0-only dispatcher for ROM `$214A4`, whose exact target is A2. The dispatcher therefore returned without changing the shadow.

WIP152 introduces target-qualified event application:
- normal current-actor events use expected target A0;
- the already-audited reciprocal grapple role-swap uses expected target A2 for `$214A4`.

This is an exact event alias already established by the grapple role-swap audit; no generic A2 mapping is introduced.

## Raw `$10` implication
The per-actor last-event record can feed WIP151 `OTHER_STATE1` provenance only when caller history proves that no intervening actor32 event occurred. Merely having a boolean shadow still does not prove raw10 provenance.

No live held release consumes this provenance yet.

## Safety
- `state_set()` remains actor32-free.
- No `$1C4CA/$1C52E/$1CED0` runtime wiring.
- No native-state similarity inference.
- Unknown state1 remains blocked.
- No `+$42`, `+$0E`, contact37, 32-tick fallback or tick12 changes.
- No BIN/CUE and no ROM bytes persisted.

## Tests
- Updated Test340 for target-qualified event application.
- Updated Test348 to require exact `$214A4` A2 target wiring.
- Added Test414: state0 `$114CC` provenance is recorded at reset; native reversal executes exact `$214A4` A2 SET, updates bit2, records last-event provenance, and can be resolved by the WIP151 OTHER_STATE1 pure policy.
- Tests340/345/348/380/410–414 pass.

## Next
The next Ground blocker from WIP149 is the truncated 16-bit `raw_z_shadow`. A ROM-faithful held pass requires actor `+$0E:+$10` as a 32-bit 16.16 accumulator while word-level Z rebases/corrections preserve the inherited fractional low word.

Source ZIP: `WWF-Saturn-Test255-WIP152-source.zip`
SHA256: `f2dd4fc4ecbdb8640000825dcb94f9357c035bf1ed9e332f76e2f46c03a743fc`
