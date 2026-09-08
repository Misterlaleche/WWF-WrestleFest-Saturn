# WIP115 CHECKPOINT — raw0E held-contact branch predicates

## Scope
ROM-EXACT decision model for direct outcomes behind the three raw `$0E` held-contact gates. Evidence/data only; no live release.

## Closed
`arcade_raw0e_contact_outcome()` now reproduces the demonstrated branch predicates:
- phase0: `+$25==3`, contact37, `$11152` carry and `$11412` carry choose target state5 damage17 versus actor state4 `$1E/$05` fallback.
- phase1: `+$25!=$FE`, contact37 and actor `+$60.bit7==0` choose actor state7.
- phase2: contact37 with actor `+$60.bit7==0`; `$11152/$11412/$11192` carries plus target `+$21/+$61` choose both-state5 damage15 versus actor state4 `$1E/$05`.

No universal terminal state is inferred.

## Regression
Test398 PASS.

## Safety
No physical release enabled. No held fallback removal. No tick12 damage change. No ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP115-source.zip`
SHA256 `59bd16991efe1b75f5c17b86c1bf6356d8a1b0369ec567df51493c1ef7f5c36d`
