# WIP70 — close the exact victim 08/09 <-> 06/07 prone subtype cycle

Status: Ground Actions victim-state ROM-EXACT retype checkpoint. No BIN/CUE. No ROM files.

## Closed
- Existing raw `$0A/$0B/$10` linked-target callbacks already model the exact low-byte retype `08->06`, `09->07`.
- Added the inverse state-4 callback `$1B288` rule:
  - only when actor `+$25==$FE`;
  - `06->08`;
  - `07->09`.
- The complete demonstrated cycle is now represented explicitly:
  - `08 -> 06 -> 08`
  - `09 -> 07 -> 09`
- The inverse uses low-byte semantics and therefore does not invent a high-byte flag clear.

## Deliberately unchanged
- No target subtype shadow is connected to gameplay runtime yet.
- No held release enabled.
- No 32-tick fallback removed.
- `tick==12` remains untouched.
- PICK UP tick18 handoff remains PROVISIONAL.

## Tests
- Test355 PASS.
- Tests353/354 remain PASS.
- Python syntax PASS.

## Next
Continue from the exact victim-state graph toward a runtime target subtype shadow only where its producer and state ownership are unambiguous; independently keep inherited `+$0E/+$42` holds blocked until their producer is proven.

Source ZIP: `/WWF-Saturn/WWF-Saturn-Test255-WIP70-source.zip`
SHA256: `1e99e441fd301dc95f7219ca43f96e895a7e84688eea10c9e961096385ecbfe2`
