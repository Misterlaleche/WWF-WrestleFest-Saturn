# Test255 WIP9 — ROM-exact biography portraits including Hawk/Animal

## Portrait composition sets

The biography renderer at `$8DD2-$8DF6` reads the featured wrestler from `$8E9A`, stores `featured+$30` in the portrait object and loads the palette resources required by that composition. The composition tables prove that the twelve portrait sets are `$30-$3B`, each with poses 0, 1 and 2.

- Hawk (logical/original ID 10): set `$3A`, 40/40/47 non-empty pieces.
- Animal (ID 11): set `$3B`, 44/44/46 non-empty pieces.

All sets use the wrestler-specific palette bank `$10+id` plus common bank `$1E`. `ATTRPRT.PAK` contains all 36 exact compositions; `ATTRPRT.PAL` preserves the twelve specific palettes plus common `$1E` without normalizing colour zero.

Pack: 79,409 bytes, 741 unique tiles, 1,630 records, SHA256 `01ce6295e6ff2da86087c7096fea09fb4e14b9a4b02579b7cdeefc8995c008ff`.
Palette: 416 bytes, SHA256 `b0d238d4526629b5f0801503912f6d3789e45abe32451a85647ce56fb4876abd`.

## Biography animation and text selectors

For states 2-13, `$8D6C-$8E98` proves:

- text IDs: `6`, `7+(state-2)`, `$40+(state-2)`;
- portrait poses 0/1 alternate with reload interval `(RNG & $0F)+8`, i.e. 8-23 raw countdown values? No: the code masks to `$0F` and adds 8, therefore 8-23 inclusive;
- at phase counter `$0100`, pose 2 is forced;
- at `$0200`, portrait resources and biography text are removed.

The runtime mirror intentionally exposes the raw ROM countdown rule rather than inventing a fixed animation rate.

## Fidelity status

**ROM-EXACT:** portrait-set IDs, all 36 compositions, palette-bank identities, Hawk/Animal portrait assets, text selector arithmetic, pose-2/removal thresholds, countdown formula.

**REGISTERED BUT NOT IMPLEMENTED:** Hawk/Animal as live combat actors in states that require them. Their biography portraits are no longer blocked, but the gameplay/demo actor path still cannot rely on fabricated C10/C11 resident packs.

Canonical Test254/V0.269 remains unchanged. Test255/V0.270 remains WIP. No BIN/CUE generated.
