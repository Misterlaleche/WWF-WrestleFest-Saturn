# Test255 WIP16 — LOD / Doomsday handoff checkpoint

Status: WORK IN PROGRESS. Test254/V0.269 remains canonical until Test255 is validated on real Saturn hardware.

## Source of truth

Library source snapshot:
`/WWF-Saturn/WWF-Saturn-Test255-WIP16-source.zip`

SHA256:
`dac151299786211086d10b42ba0f2df4156f2efd137792934d457c58ae9be374`

Resume branch:
`test255-lod-handoff`

## ROM-EXACT closure reached

The complete selection bridge for the Legion of Doom team special is now closed:

1. Animal linked hold uses raw action `$89`, descriptor `$1ABA4`, callback `$1ABC0`.
2. The Animal/victim reciprocal grapple relation `+$26` survives the hold.
3. Hawk waits exactly 8 ticks in the recovered AI path.
4. Hawk checks its partner through `+$86` and enters `$12C22` only when that partner is in state 5 / raw `$89`.
5. Hawk arms collision profile `$17`; handler 11 / `$247EC` converts the victim to type `$12` and establishes reciprocal `+$92` via `$24DEC/$24E02`.
6. Hawk's three-actor path uses descriptor `$12C0A`; victim `+$26` resolves Animal as third actor.
7. Special three-actor poses include 459/460 then 162/163.
8. Victim receives state 4, selector `$19`, raw damage `$20`; third actor receives state 5 / raw `$8006`.
9. Wrestler state 8 sets `+$32 bit1`; Hawk `$12C22` consumes that exact flag and uses the recovered special launch vector.

This means the missing Animal-hold -> Hawk-special trigger is no longer PROVISIONAL. It is ROM-EXACT.

## Next task

Integrate the above exact state/link/timing chain into the native Saturn runtime. Do not replace it with a synthetic `if Animal is holding then Hawk jumps` shortcut. Add focused host tests for each transition before enabling normal gameplay.

`LODTEAM.PAK` and exact three-actor compositions remain REGISTERED BUT NOT IMPLEMENTED until this runtime integration is completed.

## Roster correction that must be preserved

Logical Saturn IDs vs arcade graphics sources:
- 0 Hogan -> 0
- 1 Warrior -> 1
- 2 Jake -> 2
- 3 Boss Man -> 3
- 4 Smash -> 10
- 5 Crush -> 11
- 6 Slaughter -> 6
- 7 Perfect -> 7
- 8 Earthquake -> 8
- 9 DiBiase -> 9
- 10 Hawk -> 4
- 11 Animal -> 5

Do not revert to the earlier incorrect direct-bank assumption for Hawk/Animal.

## Build policy

No new BIN/CUE is required for this checkpoint unless explicitly requested. When a BIN/CUE is requested, the assistant should run/fix GitHub Actions and deliver the artifact directly rather than asking the user to operate the workflow.
