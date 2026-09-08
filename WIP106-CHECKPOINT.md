# WIP106 CHECKPOINT — prone-selection probe42 entry provenance

## Scope
ROM-EXACT proof of `+$42` provenance at the prone-selection surface for main-state0 versus main-state1. No live release change and no BIN/CUE.

## Closed
- Main frame order calls input/control `$F18A` before actor callbacks `$F4C2`.
- Ground dispatcher `$DCDE` requires `+$20.bit7` already set.
- A freshly written main-state0 has that bit clear, so Ground selection is skipped until `$F4C2` processes the state transition.
- State0 callback `$114B2` is first-entry latched and clears `+$42` exactly at `$11500`.
- Therefore a prone action selected while attacker main-state0 has exact entry `+$42=0`.
- The prone selector also accepts main-state1; multiple state1 producer paths exist, so state1 remains inherited rather than guessed zero.
- Added `arcade_prone_selection_probe42_entry_policy()` and Test389.

## Consequence
The prior inherited-probe blocker is no longer universal: main-state0 Ground entry is closed at zero. Main-state1 predecessor provenance remains the bounded unresolved branch.

## Regression
PASS Test386, Test388, Test389 (ROM supplied temporarily for Test389).

## Safety
No ROM bytes included in source package.

## Source archive
`WWF-Saturn-Test255-WIP106-source.zip`
SHA256 `ffc926f2d497a76daf3fbda385632b20c804f0ef8a0439b59ee328f2690c3d33`
