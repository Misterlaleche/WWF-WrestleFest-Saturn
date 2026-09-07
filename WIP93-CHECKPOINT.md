# WIP93 — raw `$15/$17` exact visual/callback profile closure

## ROM-EXACT profiles
- Category `$09` raw `$15`:
  - descriptor `$14CE0`
  - callback `$14CF4`
  - finite 43 ticks
  - poses: `248(11) -> 249(15) -> 250(17)`
- Category `$09` raw `$17`:
  - descriptor `$14E2C`
  - callback `$14E40`
  - finite 37 ticks
  - poses: `254(9) -> 255(11) -> 256(17)`

## Boundary
- The retained source does not contain enough callback-side evidence to label raw `$15` as a particular throw or raw `$17` as Irish whip.
- Runtime therefore keeps those semantic bridges PROVISIONAL rather than treating visual timing alone as move identity.
- No artificial 43/37-tick gameplay delay is added until the callback semantics and victim-side writes are recovered.

## Tests
- Test379 PASS.
- Test380 PASS.
- Test381 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP93-source.zip`
- SHA256 `6616a46fa359494618a747e5d1d8e30fde4db50d45fb30ce0ed80920ddf26b52`

## Next
- Continue grapple/throw closure using independently proven move/finisher callbacks; raw `$15/$17` semantic replacement remains blocked pending executable callback evidence.
