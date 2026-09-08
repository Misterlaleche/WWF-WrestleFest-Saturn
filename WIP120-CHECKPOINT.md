# WIP120 CHECKPOINT — raw35 exact contact and terminal lifecycle

## Scope
ROM-EXACT raw `$35` contact split and `+$25==$FE` terminal behavior. No live timing migration.

## Closed
At the exact `+$20` probe/contact gate `$1780A`, provided actor `+$60.bit7` is clear:
- `$11152` carry set AND `$11412` carry clear -> target main state4 + word `$001A`, target pending damage 18.
- otherwise -> attacker main state4 + word `$0005`, attacker pending damage 3 fallback.
At `+$25==$FE`, `$178BE` writes attacker main state7.

Added `arcade_raw35_contact_result()` / `arcade_raw35_terminal_state7()`; Test403 PASS.

## Consequence
Raw `$35` post-contact machine behavior is structurally closed. Remaining live barrier is inherited Z provenance and faithful contact/release timing.

## Safety
No live release, no held fallback removal, no `tick==12` damage migration, no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP120-source.zip`
SHA256 `113f11ee978926ae4c5b4e1276c7a416d6968dda39b15984bf0fa840cc01a878`
