# WIP56 — first live ROM-exact actor32 shadow mutation

Status: source/runtime checkpoint only. No BIN/CUE. No ROM files.

## Closed
- Added an address-driven `actor32_apply_rom_mutation()` runtime helper backed exclusively by the nine ROM-exact mutation profiles already pinned by Tests305/329.
- The helper preserves every `+$32` bit except bit2 and rejects A2/linked mutations when no linked-actor event source has been proved.
- Wired the exact state-0 initialization clear at ROM `$114CC` into normal fighter construction/reset paths, including rumble replacement construction.
- This is an initialization-event mapping only; it is deliberately **not** implemented as `ST_IDLE == ROM state0`.

## Safety boundary
- The four exact state-2-transition set sites (`$1C4CA/$1C52E/$1CED0/$1F354`) are represented but not invoked from native states because no safe native event alias has been proved.
- Unclassified `$0F3BC/$11E04`, raw-$75 `$1D85A`, and linked-role-swap `$214A4` likewise remain uninvoked until their concrete native event sources are proved.
- Raw `$10` therefore remains blocked by the incoming actor32 bit2 lifecycle; live `$FF00` release is still disabled.
- `tick==12` and the 32-tick held fallback remain PROVISIONAL.

## Tests
- Directed Tests329–340 PASS.
- Test231 gameplay compile/runtime regression PASS with the normal host dependency set.
- Test339 updated to assert exact mutation wiring exists while live raw-$10 release remains disabled.
- New Test340 prevents guessed state2/native-state mutation calls from being introduced.

## Source archive
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP56-source.zip`
- SHA256: `79cac8cba7a7f131921b2cc95a9b5baf7d2516720c9bc20115c067468c7e4321`

## Next
Identify one concrete ROM state-2 transition producer that corresponds to a native runtime event on the raw-$10 attacker path. Once that event can set `actor32_shadow bit2` without alias guessing, re-evaluate raw-$10 live-release readiness.
