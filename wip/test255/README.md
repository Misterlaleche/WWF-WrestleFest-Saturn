# Test255 / V0.270 WIP — ROM-exact attract/presentation recovery

Not canonical yet. Test254/V0.269 remains the validated baseline.

## ROM-EXACT recovered and executable
- 14 attract-state durations at program ROM `$119A`.
- Exact state participant tables at `$0F3E`, including the Royal state-1 six-actor branch from `$0F76`.
- Featured wrestler/portrait table at `$8E9A`: `FFFF, FFFF, Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush`.
- Overlay routine around `$8CF8`: TAG text ID `$29`, gfx block `$15`, palette bank 7; Royal text ID `$2A`, gfx block `$16`, palette bank 6.
- Two state-entry Demolition interstitial scripts from `$AE20`: mode1 `$B1E2` and mode2 `$B25E`, with corrected speaker-only `$B0C8` interpretation and exact audio consumption (`$316A`; `$316B->$3178`).
- Exact state-transition placement: entering state 1 performs the first Demolition interstitial then TAG Records; entering state 2 performs the second interstitial then Royal Records before the biography/demo flow continues.
- Large biography portrait sets `$30-$3B`, three poses each, for all 12 wrestlers. Hawk is set `$3A`; Animal is set `$3B`.
- Biography text selectors: `6`, `7+(state-2)`, `$40+(state-2)`.
- Biography portrait animation policy: poses 0/1 use `(RNG & $0F)+8` reload timing; pose 2 forced at `$0100`; portrait/text removed at `$0200`.

## WIP9 asset contract
- `ATTRPRT.PAK`: 36 exact portrait compositions, 79,409 bytes, 741 unique tiles, 1,630 records, SHA256 `01ce6295e6ff2da86087c7096fea09fb4e14b9a4b02579b7cdeefc8995c008ff`.
- `ATTRPRT.PAL`: 13 exact palette banks (12 wrestler-specific plus common `$1E`), 416 bytes, SHA256 `b0d238d4526629b5f0801503912f6d3789e45abe32451a85647ce56fb4876abd`.

## Still pending
- Hawk/Animal as live combat actors in demo states that require IDs 10/11; no fabricated C10/C11 gameplay packs.
- Final integration of the exact attract controller into `main.c` replacing the old fixed native autoplay path.
- Real-hardware validation of Test255/V0.270 BIN/CUE.

The complete WIP9 source snapshot (including generated binary assets) is stored separately as `build-input/WWF-Saturn-Test255-WIP9-source.zip` for the GitHub Actions Saturn build.
