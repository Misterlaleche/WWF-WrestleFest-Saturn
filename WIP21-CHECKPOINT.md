# Test255 WIP21 checkpoint

## Source

- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP21-source.zip`
- SHA256: `62e6da5bed5ab9920d70955f70fb12f05a737ff7c03948b7d2d63438712f393c`
- Derived directly from WIP20.
- Test254 / V0.269 remains canonical. Test255 / V0.270 remains WIP.

## What changed

WIP21 extends the WIP20 streaming groundwork without activating it in Saturn gameplay.

### Test277 — Saturn-style sector reader

Added `stream_sector.c/.h` implementing arbitrary byte reads over whole 2048-byte sector fetches, matching the offset/skip/rounded-sector strategy already used by the existing Saturn audio backend.

The implementation supports:

- offsets not aligned to 2048;
- reads spanning sector boundaries;
- reads larger than the small sector staging buffer by chunking;
- no heap allocation inside the reader.

Test277 routes both `RING14.PAK` and `OBJ15.PAK` through this sector reader and verifies every streamed tile remains identical to the existing full-pack decoder.

### Test278 — streamed frame/record metadata

Added accessors for WFC2 frame indexes and records so a renderer can consume composition geometry directly from the retained metadata without a full resident pack.

Test278 checks every frame index and every record in RING14/OBJ15 against the original WFC2 metadata.

## Current streaming memory target

- RING14 metadata+directory: 53012 bytes.
- OBJ15 metadata+directory: 32320 bytes.
- Maximum compressed block: 3013 bytes RING14 / 2741 bytes OBJ15.
- Sector staging used in host model: 8192 bytes.
- Block decode staging used in host model: 8192 bytes.

This is dramatically below the previous ~376 KiB resident/arena approaches that produced CHECK347/348 on real hardware.

## Status labels

- **ROM-EXACT:** WFC2 metadata, records, tile payloads and decoded output are unchanged.
- **PROVISIONAL:** HWFIX7 visual fallbacks remain the active runtime policy while streaming is not yet connected.
- **REGISTERED BUT NOT IMPLEMENTED:** Saturn `cd_block_sectors_read` adapter in gameplay, stream-backed VDP1 composition renderer, OBJ15 activation, RING14 activation.

## Validation

- Test275 PASS: streamed WFC2 tile decode equals full-pack decode for every tile in both packs.
- Test277 PASS: Saturn-style sector reads preserve the same output, including unaligned offsets.
- Test278 PASS: streamed frame indexes and records equal original WFC2 metadata.

## Next task

Implement the Saturn adapter and a stream-backed actor/composition renderer behind an opt-in compile flag. Activate only `OBJ15.PAK` first; keep HWFIX7 fallback available until real-hardware validation. No BIN/CUE unless explicitly requested.