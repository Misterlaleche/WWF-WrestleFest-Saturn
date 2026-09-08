# WIP119 CHECKPOINT — raw14 exact contact and terminal lifecycle

## Scope
ROM-EXACT raw `$14` contact split and `+$25==$FE` terminal behavior. No live timing migration.

## Closed
At the exact `+$20` probe / contact37 gate `$14C1A`, provided actor `+$60.bit7` is clear and `+$25==3`:
- `$11152` carry set -> target main state4 + word `$0007`, pending damage 11.
- `$11152` carry clear -> attacker main state4 + word `$0005`; linked target pending damage 3 fallback.
At `+$25==$FE`, `$14CD2` writes attacker main state7.

Added `arcade_raw14_contact_result()` / `arcade_raw14_terminal_state7()`; Test402 PASS.

## Consequence
Raw `$14` machine behavior after demonstrated contact is structurally closed. Remaining live barrier is inherited Z provenance and faithful contact/release timing.

## Safety
No live release, no held fallback removal, no `tick==12` damage migration, no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP119-source.zip`
SHA256 `39ca7595b09841630393ef07ebea418e348e7fec28f6ed54bc2189ba49b2b958`
