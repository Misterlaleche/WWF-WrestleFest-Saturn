# WIP113 CHECKPOINT — state0/state1 inherited-Z preservation

## Scope
ROM-EXACT audit of direct actor `+$0E` writes in the main-state0 and controller-state1 callback families used by the neutral prone-selection provenance class.

## Closed
- Main-state0 callback family `$114B2..$115F1` contains no direct `+$0E(A0)` writer.
- Controller-state1 callback family `$11652..$11C61` contains no direct `+$0E(A0)` writer.
- These states therefore preserve the actor Z inherited from predecessor/common physics; their exact `+$42=0` and actor32-clear provenance does not imply floor Z.
- This independently reinforces the WIP111 inherited-Z guard for raw `$0B/$14/$23/$35/$46/$47`.

## Regression
Test396 PASS.

## Safety
No live release enabled, no fallback/tick12 changes, no ROM bytes packaged.

## Source archive
`WWF-Saturn-Test255-WIP113-source.zip`
SHA256 `8d0864493bb780f23c71099ef666861bc056abc6ba30c5cb8fa125842f4247ab`

## Next
Audit raw `$0E` terminal callback outcomes and continue predecessor/common-physics Z provenance separately.
