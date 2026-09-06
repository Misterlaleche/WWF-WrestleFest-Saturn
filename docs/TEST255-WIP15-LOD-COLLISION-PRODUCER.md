# Test255 WIP15 — LOD collision producer and Hawk transition flag

## ROM-EXACT closure: Hawk collision producer

The missing upstream producer for Hawk's `$12C22` reciprocal `+$92` gate is now closed.
During the callback, while `+$60 bit7` is clear and phase `+$25 < 2`, Hawk clears `+$4C`,
selects an X threshold (`$0160`, or `$0120` when `+$33 bit2` is set) and arms collision
profile `$17` once `+$0E` has crossed it.

Collision row `$17` at `$24F97` is `80 00 00 01 0E 0B 00`; its first-handler index is 11,
which dispatches to `$247EC`. That handler writes target `+$64=$0012` (therefore low byte
`+$65=$12`). The generic successful-collision tail then calls `$24DEC` and `$24E02`, writing
Hawk->target and target->Hawk reciprocal pointers in `+$92`. On the next `$12C22` callback,
those are exactly the conditions tested at `$12CD2..$12CEA`.

This is **ROM-EXACT**. It replaces the prior REGISTERED-BUT-NOT-IMPLEMENTED 'upstream
collision/link producer'.

## ROM-EXACT closure: Animal hold preserves the third-actor topology

The normal grapple producer writes reciprocal `+$26` links. Animal raw `$89` changes the
linked victim to held state `$FF` and synchronises position, but does not overwrite those
`+$26` pointers. Hawk's special collision creates a separate reciprocal `+$92` relation with
the held victim. In the three-actor branch, `$12DB2` reads `victim +$26` into A3, therefore
resolving the still-linked Animal as the third LOD actor.

This gives the exact topology:

`Animal <-> victim (+$26)` + `Hawk <-> victim (+$92)` -> `Hawk callback resolves Animal via victim +$26`.

## ROM-EXACT flag bridge into Hawk's special launch

Normal wrestler animation dispatch at `$1C06A` indexes table `$11478` by state `+$1C`.
State 8 points to descriptor `$11EA6`, whose callback `$11F3A` selects one of three
initialisers (`$11FAE`, `$12078`, `$12122`) according to `+$44`. All ten table variants enter
one of those initialisers and all three execute `BSET #1,+$32(A0)`.

Hawk `$12C22` initialisation executes `BCLR #1,+$32(A0)`. When the previous value was set it
enters the special branch at `$12C56`, sets `+$34 bit3` and installs the exact launch vector:
`+$58=$0400`, `+$5C=$0200`, `+$5E=$0038`, `+$5A=$0100` (with sign adjustment from facing).
Later transition paths explicitly clear `+$32 bit1`, so this is a bounded transition-origin
flag rather than an arbitrary persistent wrestler flag.

The **raw flag lifecycle is ROM-EXACT**. The human semantic label for state 8 (ring
entry/transition versus a narrower subcase) remains **PROVISIONAL** until its producer set is
fully classified; no gameplay trigger should be fabricated from the label alone.

## Still open

- Exact AI/input/event decision that causes Hawk to enter the special callback during the
  Animal hold window.
- Exact semantic classification of every producer of wrestler state 8.
- Live Saturn gameplay wiring of the complete Doomsday sequence.

`LODTEAM.PAK` remains REGISTERED BUT NOT IMPLEMENTED in live gameplay until that final
selection/entry condition is closed.
