# WIP108 CHECKPOINT — controller state1 neutral-input preservation

## Scope
ROM-EXACT reduction of the main-state1 inherited-input blocker for the controller/movement provenance class.

## Closed
- `$F228` enters main-state1 from the controller state0 path; `$F2A2` reasserts state1 in the same controller family.
- State1 descriptor `$115F2` dispatches callback family beginning `$11652`.
- No exact `+$42` literal writer and no actor `+$32.bit2` mutation lies in `$11652..$11C61`.
- Because controller state1 begins after the state0 init (`$114CC` actor32 clear, `$11500` probe42 clear), this state1 provenance preserves `probe42=0` and actor32 CLEAR.
- Other direct producers of main-state1 are deliberately not folded into this class.
- Added `arcade_prone_state1_control_neutral_inputs()` and Test391.

## Consequence
Normal state0/controller-state1 Ground selection has closed neutral `+$42` and actor32 provenance. Remaining inherited uncertainty is restricted to non-control state1 producer histories.

## Regression
PASS Test391; prior Tests386/389/390 remain compatible.

## Safety
No ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP108-source.zip`
SHA256 `8ed3f4baae08410b93d93d5df040b11b53a09ac47623ecec61576f44081841fa`
