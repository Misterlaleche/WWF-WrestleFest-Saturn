# Test255 WIP17 — Doomsday host state-machine checkpoint

Status: **WORK IN PROGRESS**. Test254 / V0.269 remains canonical until Test255 is validated on real Saturn hardware.

## Source checkpoint

- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP17-source.zip`
- SHA256: `36b25b10cf7cadcf10e5ef7df7019d22ff28a74339974be24a8e9a8a06e5e306`
- Derived only from the authoritative WIP16 source checkpoint.
- No BIN/CUE generated.

## What changed

The recovered Doomsday chain is now represented as a native host-testable state machine in `lod_team_runtime.[ch]`. It is deliberately not yet enabled by normal `game.c` gameplay.

## ROM-EXACT coverage

- Animal raw `$89`, victim raw `$FF`, preserved reciprocal `+$26`.
- Hawk state8 -> state9 carried `+$32 bit1`.
- exact 8-tick CPU wait and `+$86` partner predicate.
- state5/internal4 -> `$12BDA/$12C22` dispatch.
- `+$32 bit1` consume, `+$34 bit3`, exact launch vector.
- collision threshold and profile `$17`.
- target type `$12`, reciprocal `+$92`.
- three-actor gate and Animal resolution through `victim +$26`.
- victim state4 / selector `$19` / damage `$20`.
- Animal state5 / raw `$8006`.
- exact `$12C0A` pose clock 459/460/166/378.

## Validation

- PASS Test262
- PASS Test267
- PASS Test270
- PASS Test271

The aggregate `tools/check_host.py` stops at historical Test164 because this clean source snapshot intentionally contains no local `program.bin`. This is not caused by WIP17; further byte-level audits need the user's original ROM set.

## Next

Connect this tested state machine to `game_t` and the on-demand `LODTEAM.PAK` renderer without replacing any predicate with an `Animal holding -> Hawk auto-jump` shortcut. Keep any native coordinate/input adapter explicitly **PROVISIONAL** until proved from ROM.
