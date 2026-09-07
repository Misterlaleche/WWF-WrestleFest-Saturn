# WWF WrestleFest Saturn — Test255 WIP34 checkpoint

## Scope
Complete the remaining immediate actor +$32 bit2 producer classification and prove main-state 2 continuity.

## ROM-exact findings
- Test309 classifies the two previously open immediate BSET producers:
  - $1D85A is BSET #2,+$32(A0) followed immediately by raw action $75 in +$60(A0).
  - $214A4 is BSET #2,+$32(A2), i.e. the linked/secondary actor role-swap path rather than current A0.
- Test310 pins raw-$75 callback $1A0AC main-state writes: exactly 6, 0 and 2.
- Test311 pins state-2 descriptor $11C62 -> callback $11C82. The callback writes only states 3, 6 and 4 and contains no BCLR #2,+$32(A0).
- Therefore actor +$32 bit2 is genuinely inherited state across main-state 2 and main-state 5. The Saturn bridge must carry it explicitly; it must not infer or hard-code bit2=0 on prone entry.

## Implementation consequence
The live prone floor-contact bridge must take persistent actor32_bit2 as an input together with the normal-combat mode, actor +$0E and callback +$42 probe. Existing pure ROM-exact helpers already support actor_32 in producer selection; the remaining work is to map the actor bit lifecycle into native Saturn state producers and to recover the exact +$0E evolution for the held-prone phases.

## Tests
- Test309: remaining immediate BSET producer classification.
- Test310: raw-$75 callback state writes.
- Test311: state-2 bit2 continuity and terminal state writes.

## Still open
- Pin the exact +$0E evolution at each held prone phase, especially the +32 probe paths and raw $0E's three consecutive holds.
- Wire persistent actor32_bit2 + raw Z + probe42 into the native fighter runtime only after those Z transitions are closed.
- Do not claim ground timing provisional=0 yet.

No ROM image is included. No BIN/CUE generated.
