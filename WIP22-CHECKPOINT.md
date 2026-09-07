# Test255 WIP22 checkpoint

Status: source-only WIP. No BIN/CUE generated.
Canonical validated baseline remains Test254 / V0.269.

## Base
- Derived from Test255 WIP21.
- Keeps hardware-stability behavior validated in HWFIX7.
- Keeps Doomsday/Hawk/Animal work from WIP19+.

## New in WIP22
- Added complete host validation of streamed WFC2 compositions for `RING14.PAK` and `OBJ15.PAK`.
- Test279 proves streamed frame -> record -> tile data is byte-identical to the resident WFC2 loader for all 210 RING14 frames and all 100 OBJ15 frames.
- Test280 adds `stream_pack_visit()` and proves the stream path emits records in resident order and respects bounded run slices.
- No gameplay activation yet: `RING14.PAK` and `OBJ15.PAK` remain disabled in runtime until Saturn/CD adapter and renderer integration are validated.

## Labels
- ROM-EXACT: recovered raw pose/action identities and existing composition mappings remain unchanged.
- PROVISIONAL: HWFIX7 bounded visual fallbacks and the future streaming transport policy.
- REGISTERED BUT NOT IMPLEMENTED: runtime streaming activation of RING14/OBJ15 on Saturn.

## Validation
Directed host suite passes: Test203, 205, 206, 207, 211, 253, 259, 260, 267, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280.
Full host suite still stops at ROM-dependent Test164 because `program.bin` is intentionally absent from source checkpoints.

## Library
- `/WWF-Saturn/WWF-Saturn-Test255-WIP22-source.zip`
- SHA256 `614661a625e6dc431c2f4335801861a7e5dabe7c97196939f6c080e695fcbfc6`
