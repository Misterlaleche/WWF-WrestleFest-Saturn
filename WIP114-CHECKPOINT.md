# WIP114 CHECKPOINT — raw0E exact outcome-site map

## Scope
ROM-EXACT replacement of the old “raw `$0E` terminal unknown” bucket with the bounded set of direct outcome writes in its callback family. No live release/damage timing change.

## Closed outcome sites
- `$13E38`: linked target main state5; `$13E44` pending damage `$0011`.
- `$13F12`: actor main state7.
- `$13F2A`: actor main state4 + word `$001E`.
- `$13F3A`: actor main state4 + word `$0005`.
- `$14028/$14034/$14052`: actor and target main state5; target pending damage `$000F`.
- `$140E0`: actor main state4 + word `$001E`.
- `$14186`: actor main state4 + word `$0005`.

Added `arcade_raw0e_outcome_profile_count()` / `arcade_raw0e_outcome_profile()` and Test397.

## Consequence
Raw `$0E` no longer lacks direct terminal/outcome ROM evidence. It remains branch-dependent: there is deliberately no single `arcade_prone_terminal_state_profile(0x0e)` result and no live release inference from this table alone.

## Safety
No physical release enabled. No 32-tick fallback removal. No `tick==12` damage migration. No ROM bytes packaged.

## Regression
Test397 PASS.

## Source archive
`WWF-Saturn-Test255-WIP114-source.zip`
SHA256 `875c7e5f2b4ba8dc28730b5915b646ea9bf0c4f01dc7146f177e74c2351046c1`
