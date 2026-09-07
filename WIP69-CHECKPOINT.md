# WIP69 — PICK UP namespace guard + complete victim state-4 subtype dispatch registry

Status: Ground Actions / victim-state ROM-EXACT registry checkpoint. No BIN/CUE. No ROM files.

## Closed
- Proved and encoded the namespace separation between raw action `$22` PICK UP and state-4 subtype `$22`:
  - raw action `$22` descriptor `$15B6C`, callback `$15B8C`;
  - state-4 subtype `$22` descriptor `$1BDD8`, callback `$1BDE8`.
- Therefore state-4 callback `$1BDE8` cannot be used to justify the still-PROVISIONAL PICK UP victim handoff.
- Added complete ROM-EXACT state-4 subtype `$00..$2A` descriptor/callback registry from the final corrected victim65 research.
- This gives the Ground Actions runtime a bounded exact victim-state dispatch surface for subsequent producer/retype integration without conflating action IDs with state-4 subtype IDs.

## Deliberately unchanged
- PICK UP victim handoff at native tick 18 remains PROVISIONAL.
- No held Ground Action release enabled.
- No 32-tick held fallback removed.
- No `tick==12` ground damage timing changed.

## Tests
- Test351 PASS.
- Test352 PASS.
- Test353 PASS: raw `$22` callback `$15B8C` != state-4 subtype `$22` callback `$1BDE8`.
- Test354 PASS: every state-4 subtype `$00..$2A` resolves to its exact descriptor/callback; out-of-range rejected.
- Python syntax PASS.

## Next
Use the exact state-4 registry to integrate only fully proven victim subtype transitions relevant to reachable Ground Actions, while continuing the inherited `+$0E/+$42` release investigation separately.

Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP69-source.zip`
SHA256: `96d89151827c66c068b06fbb332e26b55fa173439b0521f9c21257b7765c34e8`
