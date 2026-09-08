# WIP117 CHECKPOINT — raw10 exact contact and terminal lifecycle

## Scope
ROM-EXACT raw `$10` contact result and `+$25==$FE` terminal behavior. No live timing migration.

## Closed
At the exact `-$20` probe / contact37 gate `$142D8`:
- `$11152` carry set -> target state4, high flags cleared/preserved-low then exact `08->06` / `09->07`, pending damage 10.
- `$11152` clear + `$11192` set -> target state5, pending damage 10.
- both helpers clear -> attacker state4 + word `$0005`, attacker pending damage 3 fallback.
At `+$25==$FE`, `$143AC` writes attacker main state7.

Added `arcade_raw10_contact_result()` and `arcade_raw10_terminal_state7()`; Test400 PASS.

## Consequence
Raw `$10` machine behavior after a demonstrated contact is structurally closed. The remaining live barrier is faithful provenance/timing of the contact/release event, not unknown damage or terminal state.

## Safety
No live release, no fallback removal, no tick12 damage/retype migration, no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP117-source.zip`
SHA256 `96a6cc59e4d53c694f3295faeff657c1f5d2033e15328a7b802ed64ae9ea3c73`
