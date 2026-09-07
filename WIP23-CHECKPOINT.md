# Test255 WIP23 checkpoint

Status: source-only WIP. No BIN/CUE generated.
Canonical validated baseline remains Test254 / V0.269.

## New in WIP23
- Added `stream_overlay.[ch]`, a no-allocation dual overlay runtime for streamed WFC2 packs.
- Test281 opens RING14 and OBJ15 simultaneously over Saturn-style sector reads, shares sector staging and block cache, alternates packs deliberately, and verifies tile bytes against resident `roster_pack` decoding.
- Actual metadata retained: 85,332 bytes combined.
- Test workspace including metadata, 4 KiB sector staging, 4 KiB scratch and shared block cache: 114,768 bytes.
- Gameplay activation remains deferred until renderer/CD wiring has a dedicated host/source audit and a later hardware build is explicitly requested.

## Labels
- ROM-EXACT: pack contents, frame/record/tile composition data.
- PROVISIONAL: Saturn streaming transport/memory policy.
- REGISTERED BUT NOT IMPLEMENTED: live VDP1 consumption of streamed RING14/OBJ15.

## Library
- `/WWF-Saturn/WWF-Saturn-Test255-WIP23-source.zip`
- SHA256 `f4c50e99611cd7af95ee51a869d104a2667198d8c68c5e2c52f1992ac2586f6e`
