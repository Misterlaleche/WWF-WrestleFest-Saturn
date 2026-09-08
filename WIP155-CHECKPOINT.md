# WIP155 CHECKPOINT — Ground actor +$25 previous-+$23 latch

## Scope
Close the exact provenance transform for actor byte `+$25`, which WIP104 proved must not be inferred from descriptor `+$22/+24`.

## ROM-EXACT rule
Common actor/control arbiter `$5B66` executes:
`MOVE.B +$23(A0),+$25(A0)`
before it reads and table-selects the next `+$23` value.

Therefore `+$25` is the previous `+$23` latch for that arbiter pass. It is not the descriptor counter, descriptor phase, or a synthetic timer.

`arcade_actor25_latch_from_actor23()` exposes this exact transfer and refuses to synthesize `+$25` when prior `+$23` validity is unknown.

## Safety
- No physical held release enabled.
- No tick12/fallback changes.
- No `+$23` value guessed.
- No BIN/CUE; ROM used only temporarily for Test418 and removed before packaging.

## Test
Test418 PASS, pinning ROM `$5B66` and the validity rule.

Source ZIP: `WWF-Saturn-Test255-WIP155-source.zip`
SHA256: `c72d55bdac0021dab6c04a8f65872691530e541413dd3fc72efad542e2c4d679`
