# WIP99 CHECKPOINT — Front-grapple throw lifecycle after exact ownership

## Changes
- Added Test383 for full live throw completion after the modern grapple path.
- Covers Hogan Body Slam (tier0/slot0) and Suplex (tier2/slot3).
- Both cases must traverse:
  raw `$29` struggle -> `$0C/$FF` ownership -> category `$09` P/raw `$15` -> native throw bridge.
- Verifies exact-once impact accounting in the current runtime, native move damage application, throw recovery flag and post-throw RECOVER/DOWN transition.

## Fidelity boundary
- ROM-EXACT: raw `$29` struggle, `$0C/$FF` ownership and category `$09` P -> raw `$15` input identity.
- PROVISIONAL: semantic raw `$15` -> four-move native grapple rota and generic native throw impact clock/damage unless separately ROM-closed by the specific move.

## Validation
PASS: Test383, grapple_rota, irish_whip, submissions, Tests377-379.

Source ZIP SHA256: `811c171549f21164c923fe473e907a20cdbf1f48a3bcd13f198e26e736a99e5b`

No BIN/CUE generated.
