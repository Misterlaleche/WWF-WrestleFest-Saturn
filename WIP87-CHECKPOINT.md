# WIP87 — state4 prone transition blocker map + raw $35 victim chain closure

## Safety/readiness API
- Added explicit dynamic blocker map for the exact state-4 prone-family transition core:
  - INITIALIZED
  - CONTACT37
  - ACTOR25_FE
  - UNMODELED hard blocker
- `arcade_state4_prone_transition_ready(...)` returns true only when all currently demonstrated predicates for that subtype are satisfied.
- This API is a safety/readiness surface only. It does not release descriptor `FF00` holds or invent callback-to-clock coupling.

## Exact subtype coverage
- `$05/$2A`: require initialized state + current-turn contact37.
- `$06/$07`: require actor `+$25==$FE`.
- `$21`: requires current-turn contact37.
- Added exact `$1A -> $09` transition on current-turn `+$37 bit4`, from ROM callback `$1BBC6`.
- Other subtypes remain UNMODELED for this prone-family transition API.

## Raw $35 chain
- Unique action-side producer: raw `$35`, ROM `$17858`, WORD `$001A` on linked victim.
- Exact state-4 callback transition: `$1A --contact37--> $09`.
- Exact prone projection: `$09` + tier0 -> category6; `$09` + nonzero tier -> category7.
- Therefore the structural chain is closed. The remaining runtime gap is the victim's current-turn contact37 source, which is not guessed.

## Tests
- Extended Test367 for `$1A->$09` word transition.
- Test374 validates blocker matrix and hard-blocking of unmodeled subtypes.
- Test375 validates the complete raw `$35` producer -> `$1A` -> contact -> `$09` -> category6/7 chain.
- Directed Tests367/373/374/375 PASS.

## Persistence
- Library source: `/WWF-Saturn/WWF-Saturn-Test255-WIP87-source.zip`
- SHA256: `82fa241bdcba4bb6fbb7877dd62f8672e4baa2cf7611399f494a406d43e13792`

## Fidelity boundaries
- ROM-EXACT: numeric producer, transition predicate, subtype result and category projection.
- REGISTERED BUT NOT IMPLEMENTED: live victim contact37 callback input and callback-to-state4-clock release coupling.
- No Ground Action held release enabled. No held fallback removed. No `tick==12` replacement. No BIN/CUE. No ROM files.
