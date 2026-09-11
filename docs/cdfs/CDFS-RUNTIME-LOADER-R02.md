# CDFS Runtime Loader R02

Status: **CANDIDATE platform infrastructure**. No gameplay semantics are introduced.

## Contract

The runtime loader is split into a platform-neutral core and a Yaul adapter.

Destination classes:
- **HWRAM staging**: intended to feed later renderer-owned SCU-DMA transfers.
- **LWRAM bulk**: CPU/CD-block-loaded residency; never an SCU-DMA endpoint.

The loader never reads CDFS data directly into VDP1, VDP2, CRAM or SCSP memory. Those remain owned by Renderer26 / Audio24.

A scene transaction performs complete preflight before the first read, rejects overlapping destination ranges, then reads every requested file. `ready` becomes true only after every read succeeds.

This is atomic **publication**, not memory rollback. If an I/O failure occurs after an earlier staging buffer has been filled, all transaction buffers are invalid until the next successful transaction and the scene must remain unpublished.

## Yaul adapter

Uses the supplied APIs:
- `cdfs_config_default_set()`
- `cdfs_filelist_init()`
- `cdfs_filelist_root_read()`
- exact root-file name lookup
- `cd_block_sectors_read()`

The audited supplied libyaul implementation of `cd_block_sectors_read()` transfers the requested byte length through the CD block data register. The destination is not padded with invented sector bytes. Sector-rounded sizes remain relevant to disc/layout accounting.

## Fail-closed rules

- root filename length > 11: reject;
- missing or zero-length file: reject;
- destination smaller than file: reject;
- HWRAM/LWRAM class mismatch: reject;
- overlapping transaction destinations: reject;
- any CDFS I/O failure: scene remains not-ready;
- no scene publication until `ready == true`.

## Validation

- host compile/test: PASS with `-Wall -Wextra -Werror`;
- SH-2 core compile: PASS;
- SH-2 Yaul adapter compile: PASS;
- full Saturn ELF link: not claimed until merged with runtime;
- no BIN/CUE generated;
- no hardware PASS claimed.

## Not invented

No frontend/gameplay asset manifest, loading duration, loading screen, CDFS LBA, or gameplay transition is synthesized by this layer.
