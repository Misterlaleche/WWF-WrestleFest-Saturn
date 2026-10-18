# ASSEMBLER HANDOFF — SPECIAL-MULTI-ACTOR-DOOMSDAY

## Authority
- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`
- Depend on victim65 FINAL for collision subtype semantics and victim state-4 lifecycle.
- Depend on FINAL audio transport for raw SFX commands.
- Depend on Workpack 03/28/06 FINAL outputs for exact motion/scheduler/TAG staging where those scopes own the behavior.
- Supersedes WIP18's provisional "release pose166 after minimum one native tick" bridge.

## Implementable now
- Animal `$89` prerequisite/state exposure and preservation of `+$26`.
- Hawk state9 counter (`+$BD`) and `+$86` partner predicates.
- internal actor4 dispatch to `$12BDA/$12C22`.
- carried `+$32 bit1` consume; `+$34 bit3`; launch raw fields `$58/$5A/$5C/$5E` and facing sign.
- `+$0E` profile-$17 threshold rule.
- profile `$17` target `$0012` and reciprocal `+$92` acquisition through canonical collision engine semantics.
- phase1/`+$22==0`/team-bit three-actor gate.
- third actor strictly from victim `+$26`.
- Animal state5/raw `$8006`; victim state4/`+$64=$0019`/`+$68=$0020`.
- descriptor clock 459:4, 460:8, 166:FF00, 378:32.
- phase2 release/contact predicate = current-turn `+$37.bit4`.
- raw audio command producer routing through FINAL audio.

## Must remain fail-closed
- Do not commit if exact `+$26/+86/+92` topology is unavailable.
- Do not synthesize Hawk launch from "Animal holding" alone.
- Do not replace raw `+$0E` threshold progression with a timer/proximity heuristic.
- Do not auto-release pose166 after N frames; release only from exact contact37 once its live provenance is available.
- Do not invent player finisher buttons, rescues, cancels or camera effects.

## Exact constants
- CPU wait first eligible `+$BD`: `8`.
- partner required: `+$21=5`, `+$61=$89`.
- Hawk writes on success: `+$20=5`, `+$60=4`.
- launch: `+$58=$0400`, `+$5A=$0100`, `+$5C=$0200`, `+$5E=$0038`; negate `$58/$5A` when facing flag requires it.
- collision threshold: `+$0E > $0160`, or `> $0120` with `+$33.bit2`.
- collision profile: `$17`; FIRST index `$0B` -> `$247EC`.
- acquired victim word: `+$64=$0012`.
- commit gate: linked bit7, `+$25=1`, `+$22=0`, team `+$34.bit3` set.
- Animal: `+$20=5`, `+$60=$8006`.
- victim: `+$20=4`, `+$64=$0019`, `+$68=$0020`.
- descriptor `$12C0A`: `(459,4),(460,8),(166,FF00),(378,32)`.

## Required fields / links
`+$01,+$0E,+$18,+$20,+$21,+$22,+$25,+$26,+$2E,+$32,+$33,+$34,+$37,+$4C,+$58,+$5A,+$5C,+$5E,+$60/+61,+$64/+65,+$68,+$86,+$92,+$BD` as consumed/produced by the exact branches.

## Ordering
- Preserve actor callback scheduling.
- Collision success must produce reciprocal `+$92` before the next `$12C22` link test.
- Pending damage `$20` follows the canonical global damage/stamina ordering; do not apply a separate immediate Doomsday HP formula.
- `+$37` is a current-turn contact classifier; feed it from the canonical motion/contact pipeline.

## No-clobber
- Preserve Animal/victim `+$26` throughout raw `$89`.
- Preserve team `+$86`.
- WORD semantics for victim `$64` writes.
- Preserve Animal `$8006` high bit.
- Do not clear Hawk transition/team/link flags earlier than the ROM branch does.

## Minimum integration tests
Use the 15 tests listed in the FINAL technical report section 20. In particular, assert no pose166 release with contact37 false and no move commit from a synthetic timer.

## Runtime-dependent
- time to threshold; time to contact37; slot-order observation timing.

## Not provable
- separate player/attract trigger, special camera, extra rescue/cancel timeout, exact apron-return sequence, second special three-actor move.
