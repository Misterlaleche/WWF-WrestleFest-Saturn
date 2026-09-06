# Test255 WIP18 — Doomsday live gameplay checkpoint

Status: **WORK IN PROGRESS**. Test254 / V0.269 remains canonical until the user validates Test255 on real Sega Saturn hardware.

## Source checkpoint

- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP18-source.zip`
- SHA256: `fc9191006caa757d056a1511c8a8bc486ed2c440abf7a1381273dc45f958caf8`
- Derived from the authoritative WIP16 line through the host-only WIP17 checkpoint.
- No BIN/CUE generated.

## Scope

WIP18 connects the already host-proved WIP17 state machine to `game_t` and to the on-demand `LODTEAM.PAK` renderer. It does not replace the recovered producer with a synthetic `Animal holding -> Hawk jumps` rule.

## ROM-EXACT now active in gameplay

`Animal K/raw $89` -> reciprocal `+$26` -> Hawk state8/state9 carried bit -> exact 8 ticks -> partner `+$86` state5/raw `$89` -> `$12C22` -> exact profile `$17` gate -> target type `$12` / reciprocal `+$92` -> `$12C0A` -> third actor from `victim +$26` -> victim state4/selector `$19`/damage `$20`, Animal state5/raw `$8006` -> exact three-role composition clock.

`LODTEAM.PAK` is loaded after the gameplay tick that commits `PAIR_LOD_TEAM`, owns the shared low-RAM overlay slot for that frame, and draws Hawk/Animal/victim runs without separately rendering the three fighters.

## PROVISIONAL

- native `PAIR_HOLD` -> exact linked-category adapter;
- Saturn world-coordinate / raw `+$0E` launch progression;
- Hawk composition anchor transform;
- minimum one-tick release bridge for callback-held pose 166 (`$FF00`);
- committed-sequence rescue/interruption policy;
- native apron restore after completion.

## REGISTERED BUT NOT IMPLEMENTED

- exact `$FF00` release producer;
- exact launch-to-collision world integration;
- exact Doomsday interruption/rescue producer.

## Validation

PASS: Test203, 205, 206, 207, 211, 253 struggle assets, 259, 260, 267, 270, 271, 272, 273.

Historical Test210 also fails on a pristine extraction of authoritative WIP16 with the same assertion, so it is pre-existing baseline debt, not a WIP18 regression.

The aggregate byte-level runner still requires the user's `program.bin` / original `wwfwfest.zip`; ROM data is not stored in this checkpoint.
