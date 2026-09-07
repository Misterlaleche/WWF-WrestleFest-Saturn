# WIP90 — raw-$29 callback ownership drives HOLD

## Runtime
- Adds raw grapple-state shadow for the two linked wrestlers (`+$20` raw `$0B/$0C/$FF`) plus `+$44` shadow used by the recovered action-$29 callback tail.
- A successful exact raw-$29 cycle no longer increments a semantic two-score threshold and independently declares a winner.
- On action-clock completion the runtime applies the recovered `$16582` terminal model:
  - first completion -> both raw `$0B`, attacker `+$44=0`, target `+$44=$8000`;
  - second completion -> acting wrestler raw `$0C`, `+$44=$2000`, linked target raw `$FF`, `+$44=0`.
- `PAIR_HOLD` begins only when the terminal produces `$0C/FF` ownership.

## Fidelity boundary
- ROM-EXACT: `$16582` completion state writes and `$0B -> $0B -> $0C/FF` ownership topology.
- PROVISIONAL: mash/CPU arbitration deciding which side is the acting wrestler for each completed raw-$29 cycle.
- Reversal windows after HOLD remain native/provisional and are not reclassified as ROM-EXACT.

## Tests
- Test365 PASS.
- Test377 PASS.
- Test378 PASS.
- `python3 -m py_compile tools/check_host.py` PASS.

## Source
- `/WWF-Saturn/WWF-Saturn-Test255-WIP90-source.zip`
- SHA256 `cadec7885137696f3024866029744da659c62a2134dbda215d1ee50fa8309f09`

## Next
- Route won `$0C/FF` HOLD input through exact category `$09` P/K/P+K raw action selection while leaving semantic `$15/$17` callback identities provisional.
