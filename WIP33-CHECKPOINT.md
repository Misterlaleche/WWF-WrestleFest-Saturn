# WWF WrestleFest Saturn — Test255 WIP33 checkpoint

## Scope
Close the false assumption that prone main-state 5 clears actor +$32 bit2.

## ROM-exact findings
- Test307 proves four immediate A0 BSET #2,+$32 producers ($1C4CA,$1C52E,$1CED0,$1F354) are directly coupled to a preceding MOVE.W #2,+$20 transition.
- Main-state dispatch table $11478 entries 4 and 5 alias descriptor $11DCA.
- Descriptor $11DCA points to callback $12612.
- $12612 is a bare RTS.
- Therefore entering main-state 5 does not clear +$32 bit2; the bit is inherited from the prior actor lifecycle.
- Known BCLR sites remain $114CC and $11E04 and must not be conflated with generic state-5 entry.

## Tests
- Test307: actor32 bit2 state-2 producer coupling.
- Test308: state-5 bit2 preservation / descriptor proof.

## Still open
- Classify exact lifecycle/role of BSET producers $1D85A and $214A4.
- Model actor32 bit2 explicitly in the Saturn runtime only after all reachable producers/clears are mapped to native transitions.
- Do not claim ground timing provisional=0 yet.

No ROM image is included. No BIN/CUE is generated.
