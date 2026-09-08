# WIP111 CHECKPOINT — neutral-entry inherited-Z guard

## Scope
ROM-EXACT safety closure for inherited Z on held Ground Actions after WIP110.

## Closed
- Prone selector branches `$E0B8/$E110` validate attacker main-state0/1 and target state/subtype/tier, but contain no actor `+$0E` predicate.
- Common floor resolver compares `(+$0E + +$42)` against `$0140/$0100` and uses BCC to skip correction whenever the candidate is already at/above threshold.
- Therefore ROM-neutral Ground entry (`probe42=0`, actor32 clear) does not by itself prove inherited `+$0E` equals the floor threshold.
- The inherited-Z blocker must remain for raw `$0B/$14/$23/$35/$46/$47` until predecessor/common-physics Z provenance is separately closed.

## Regression
PASS Test392, Test393, Test394.

## Safety
No release blocker removed. No physical release enabled. No 32-tick fallback or tick==12 damage change. No ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP111-source.zip`
SHA256 `7d45d1614a378825ad33517d74dfd9ba2a636446923934e5199d49fed6b8aefb`
