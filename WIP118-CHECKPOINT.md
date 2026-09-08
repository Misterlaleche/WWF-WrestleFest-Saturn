# WIP118 CHECKPOINT — raw13 exact contact and terminal lifecycle

## Scope
ROM-EXACT raw `$13` contact result and `+$25==$FE` terminal behavior. No live timing migration.

## Closed
At the exact `$14A72` contact37 gate, provided actor `+$60.bit7` is clear and `+$24==1`:
- `$11152` carry set -> target main state4 + word `$0007`, pending damage 10.
- `$11152` carry clear -> attacker main state4 + word `$0005`, attacker pending damage 3 fallback.
At `+$25==$FE`, `$14AD4` writes attacker main state7.

Added `arcade_raw13_contact_result()` / `arcade_raw13_terminal_state7()`; Test401 PASS.

## Consequence
Raw `$13` machine behavior after contact is structurally closed. Like raw `$10`, the remaining live barrier is faithful provenance/timing of contact/release rather than unknown damage/terminal behavior.

## Safety
No live release, no fallback removal, no tick12 damage migration, no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP118-source.zip`
SHA256 `2cbd98b91878f712ca3e3e75852ea1d9ee1be82180ea4b06b7dcd0b78773185b`
