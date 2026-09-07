# Test255 WIP20 checkpoint

## Source

- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP20-source.zip`
- SHA256: `5b09082b5d58707931ca8c7ca0a31ef4400485a2f6f75b547dc9626373e4bb81`
- Derived from WIP19, not from WIP9/HWFIX diagnostic payloads.
- Test254 / V0.269 remains the canonical validated baseline. Test255 / V0.270 remains WIP.

## Hardware-stability consolidation

The user validated HWFIX7 on real NTSC-U Saturn as restoring normal combat/demo behaviour after the CHECK309/339/342/347/348/324 investigation.

WIP20 folds the validated stability behaviour into the WIP19 source line while preserving WIP19 Doomsday/Hawk/Animal work:

- front-end caches are explicitly released before roster loading;
- resident full-pack loading of `RING14.PAK` / `OBJ15.PAK` is disabled for the stability baseline;
- `Cxx.PAK` binding uses build-verified metadata after exact size/hash validation instead of a second full structural walk in Saturn LWRAM;
- CHECK324 diagnostics record character/state/raw pose;
- runtime stability fallback is compile-gated with `WWF_HWFIX7_STABILITY` so historical Test252 fail-closed coverage remains testable;
- only ring-transition, object, ground-attack/ground-pickup and throw-recovery families receive the PROVISIONAL stability fallback.

Labels:

- **ROM-EXACT:** recovered raw action/state/descriptor identities remain unchanged.
- **PROVISIONAL:** temporary visual fallback while RING14/OBJ15 are not resident.
- **REGISTERED BUT NOT IMPLEMENTED:** exact streamed Saturn rendering of these packs is not yet active in gameplay.

## Streaming groundwork

Added `stream_pack.c/.h` and Test275 host validation for WFC2 metadata+block streaming.

Measured real WIP19 packs:

- `RING14.PAK`: 268319 bytes total; 53012 bytes metadata+directory; max stored block 3013 bytes.
- `OBJ15.PAK`: 105279 bytes total; 32320 bytes metadata+directory; max stored block 2741 bytes.

Test275 opens each pack without retaining compressed texture payloads and compares every decoded tile against the existing full-pack `roster_pack` decoder. Result: tile-for-tile identical for all 2762 RING14 tiles and all 1015 OBJ15 tiles.

Streaming is host-tested only in WIP20 and is intentionally not yet connected to Saturn CD reads/gameplay.

## Directed regression

Passed after WIP20 integration:

- Test203
- Test205
- Test206
- Test207
- Test211
- Test253 struggle assets
- Test259
- Test260
- Test267
- Test270
- Test271
- Test272
- Test273
- Test274
- Test275
- Test276

The complete host suite still stops at Test164 when `program.bin` is absent from the source checkpoint; this is ROM-dependent test infrastructure, not a WIP20 regression.

## Next work

1. Implement the Saturn CD-sector read callback for `stream_pack` using a small sector staging buffer.
2. Add host tests for unaligned compressed-block reads matching Saturn sector semantics.
3. Activate streaming for one pack at a time, starting with `OBJ15.PAK`; keep the HWFIX7 fallback as rollback until hardware validation.
4. Then activate `RING14.PAK`, validate TAG/Royal/demo/title cycles on real Saturn, and only after stability remove the corresponding PROVISIONAL fallbacks.
5. Resume Hawk/Animal individual moveset and full-roster closure after the graphics memory path is stable.

Do not generate BIN/CUE unless explicitly requested by the user.