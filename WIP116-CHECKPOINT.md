# WIP116 CHECKPOINT — raw0E helper carry semantics

## Scope
ROM-EXACT semantic reduction of helpers `$11152`, `$11192`, `$11412` used by raw `$0E` held-contact branch predicates.

## Closed
- `$E958`, called with profile `$0A`, is a mirrored X/Y rectangle overlap test.
  - linked facing clear: X offsets `-8 .. +104`;
  - linked facing set: X offsets `-104 .. +8`;
  - Y offsets `-72 .. +24`;
  - lower bounds strict, upper bounds inclusive.
- `$11152` carry set iff linked actor is main state4, low subtype `$08/$09`, and profile10 overlap succeeds.
- `$11192` carry set iff linked actor is main state5, `+$61==$61`, and profile10 overlap succeeds.
- `$11412` carry clear iff linked actor's linked pointer returns to the current actor; otherwise carry set.

Added exact helper APIs and Test399.

## Consequence
Raw `$0E` WIP115 branch decisions can now be expressed from state/subtype/geometry/linkage instead of opaque carry inputs. Live wiring still waits for explicit native shadows/event mappings of the remaining ROM fields.

## Safety
No release/fallback/tick12 changes. No ROM bytes packaged.

## Regression
Test399 PASS.

## Source archive
`WWF-Saturn-Test255-WIP116-source.zip`
SHA256 `416f3c04ef1aafa53b0671d1113f49a0493bad2718298f5b0ab2d8fcf35324f1`
