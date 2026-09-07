# WWF WrestleFest Saturn — Test255 WIP35 checkpoint

## Scope
Pin the exact raw +$0E behavior inside the nine reachable prone callback-held ($FF00) actions.

## ROM-EXACT findings
- Common vector helper $10BD0 applies D2 exclusively to actor +$0E (`ADD.W D2,+$0E(A0)`).
- In demonstrated linked-target setup branches:
  - raw $0E copies A2+$0E to A0+$0E; the following $10BD0 calls use D2=0, net Z delta 0.
  - raw $10 copies A2+$0E, adds +$10 via $10BD0, then adds +$40 directly: net target Z +$50 on the pinned setup path.
  - raw $13 copies A2+$0E and adds +$20 via $10BD0: net target Z +$20.
- Across the exact callback-held contact windows of raws $0B,$0E,$10,$13,$14,$23,$35,$46,$47 there are no writes to actor +$0E.
- Therefore the held phase does not animate raw Z continuously. Release uses inherited/rebased +$0E plus callback +$42 probe, then shared +$37 bit4 floor contact.

## Tests
- Test312: byte-pins $10BD0 Z semantics and the raw $0E/$10/$13 linked-target Z rebase paths; helper API exposes the demonstrated net deltas.
- Test313: proves all nine held contact windows leave actor +$0E unchanged.

## Runtime status
- ROM behavior is pinned, but native fighter shadow raw-Z is not yet integrated.
- actor +$32 bit2 remains persistent actor state per WIP33/WIP34.
- Legacy 32-tick ground fallback is still present and must not be claimed closed yet.

## Next
1. Add persistent shadow raw-Z / actor32-bit2 fields to the native fighter runtime without inventing resets.
2. Map only proven native transitions to their exact raw-Z rebases.
3. Feed shadow Z + probe42 into existing contact37 helpers.
4. Replace held-descriptor timeout release and audit ground timing provisional=0.

No ROM image included. No BIN/CUE generated.
