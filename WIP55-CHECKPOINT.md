# WIP55 — actor32 runtime-shadow safety closure

Status: source/research checkpoint only. No BIN/CUE. No ROM files.

## Closed
- Source audit proves the Saturn ground runtime currently consumes `fighter_t.actor32_shadow` in `arcade_floor_resolve_base_context()` but never mutates/assigns that shadow anywhere in `game.c`.
- Therefore the raw `$10` ROM-side callback evidence is not sufficient to activate live `$FF00` release yet: the port would otherwise always behave as if inherited actor `+$32 bit2` stayed at its reset/default value.
- New Test339 locks this safety invariant and prevents accidental release activation before the shadow lifecycle is wired.

## Tests
- Directed Tests329–339 PASS.
- Test339 confirms the runtime shadow-consume-without-update gap.

## Unchanged
- raw `$10` remains the closest held action to completion.
- Z rebase and immediate `+$42=-$20` probe are already exact.
- `+$32 bit2` remains the sole live-release blocker.
- `tick==12` and the 32-tick held fallback remain PROVISIONAL.

## Next
Wire only ROM-proven `+$32 bit2` mutations into the Saturn shadow lifecycle (clear/set events already enumerated by Tests305/329), without inventing native-state aliases. Then re-evaluate raw `$10` release readiness.

Source archive SHA256: `8b195bec844dbdb38337e0ac35c0cf44db7dda8bc364b18860cdf4f2f79b81ae`.
