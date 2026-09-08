# WIP103 CHECKPOINT — exhaustive actor +$42 literal writer inventory

## Scope
ROM-EXACT executable scan of every 68000 `MOVE.W` destination `+$42(A0)` used by the prone/contact probe lifecycle.

## Closed in WIP103
- Exactly 14 `MOVE.W` producers target actor `+$42(A0)` in the 0x80000-byte main program.
- Every one uses immediate source encoding `MOVE.W #imm,+$42(A0)`; no register-fed or memory-fed MOVE.W producer exists.
- Exact non-zero literals are limited to `-$20`, `-$18`, `-$10`, and `+$20`.
- Every non-zero writer has a bounded callback clear (`CLR.W +$42(A0)`) registered by address.
- Added `arcade_probe42_literal_writer_*` API and exhaustive Test386.

## Consequence
Inherited held probes are no longer an open-ended producer problem: they can only be inherited from this finite 14-site set or from the zero established by initialization/previous clear. This does not yet prove the predecessor path for raw `$0B`, raw `$0E` phase0/1, raw `$13`, raw `$23`, raw `$46/$47`; those remain blocked until their exact incoming control-flow path is closed.

## Regression
PASS Test316, Test318, Test330, Test334, Test386.

## Source materialization
`WWF-Saturn-Test255-WIP103-source.zip`
SHA256 `2e7231eb42dd2dc7e5daae90a5341d19d27d679dbcecebce056911f0a7250116`
