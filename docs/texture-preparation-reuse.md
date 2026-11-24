# Consecutive immutable texture preparation

The private presenter can reuse the preceding successful frame's texture
preparation when its complete logical source array, asset binding, slot count
and normalized active bitmap are unchanged. Every sprite command is rebuilt
with the current positions, flips and palette. Consumption, submission tags,
display fences and the arcade clock retain their existing behavior.

## Ownership and invalidation

The texture cache saves 440 bytes of physical pins and 32 bytes of readiness
after an immutable publication. The backend claims that preparation only after
its own successful private submission, with `dirty == rom_identity` and the
same consumed bitmap as the published ready bitmap. Prepared but unconsumed
slots, generic uploads and unrelated publications cannot become candidates.

Normal begin opens an empty builder while retaining one candidate. Every
protect, resolve, fill or transfer revokes it before mutation. Abort, source
invalidation and reset revoke it as well. Restore requires the same owner,
nonzero asset generation, count and active bitmap. Only begin and display
repeat can have intervened, so no identity, crop or clock-recent bit has
changed. Restore copies only the counted frame prefix; its exposed tail keeps
the same bytes as the normal path. Publication still updates every physical
texture's last tag and clears the live pins normally.

The presenter's source-array proof is cleared before any plan attempt, on
initialization and before a BUP workspace loan. It is restored only after a
successful end. A next plan is unchanged only if it introduced no pending
source IDs. The private source/touched arrays and backend metadata have
separate ownership; the public scalar, protect and bulk-consume APIs keep
their existing validation and alias behavior.

Successful and failed asset binds advance a read-only preparation token. At
its first uint32 wrap it permanently returns zero, disabling reuse without
changing the worker's established generation counter.

## Private consumption of a restored preparation

Immediately after a successful private restore, the presenter may consume
the complete all-resident frame without entering the per-slot public consumer.
The active count was computed while normalizing the restore bitmap. The result
is exactly `dirty = resident`, `emitted_tiles = expected_tiles`, and the same
uint32 addition to `texture_reused`. It runs no callbacks and leaves all pixel,
mapping, readiness and publication state unchanged.

This permission is separate from the next-frame preparation candidate. The
cache grants it once after restore and revokes it before any intervening cache
operation. A backend sentinel also revokes it through every public tile
consumer, including a zero-count batch. Owner/count validation, abort and
initialization remain required. The private call must be immediate, before
callbacks or changes to the privately owned source/touched arrays or backend.
The public consume and alias contracts are unchanged.

For a private success, the presenter's demand frame uses count zero because
there are no missing ROM tiles. It still performs demand cleanup and the
post-consumption cleanup; the common done-label cleanup also remains. All
three demand-generation increments, worker joins and discards are retained.

## Verification

Run these from the source root with the normal immutable `cd/` assets present:

```sh
python3 tests/test_jo_preparation.py
python3 tests/test_jo_backend_counters.py --output build/preparation-counters
python3 tests/jo_sdk_pipeline/sdk_test.py --mode edges --preparation --frames 2400 --seed 1 --output build/preparation-sdk
python3 tests/jo_sdk_pipeline/sdk_test.py --mode edges --private-consume --frames 2400 --seed 1 --output build/prepared-consume-sdk
```

The preparation suite compares every legacy cache field across 2,920 frames,
including cache pressure, source reset, tag wrap, owner/count/active changes,
partial preparation and aborted tails. Its backend differential compares
38 completed frames through each of three paths, exact command/pixel digests,
staging, reads, counters and legacy cache state. Direct private-consume tests
cover one-use revocation, wrong owner/count, empty batches, exact inactive
holes, counter wrap, public/cache operations, transfers, abort and reinit.
The suite also executes the real presenter and asset code through
BUP, initialization, failed plans before/after logical assignment, rebind,
cache loans, failed binds and token wrap.

Paired-worker lifecycle runs compare the normal consumer with the private
consumer and require identical outputs, positive balanced jobs and exactly
three demand-generation increments for each successful frame.

The SDK option uses the actual dual-CPU command pipeline with delayed DMA,
fresh moving commands on repeated texture preparations, physical eviction,
four retained map banks, repeated displays, tag wrap and the existing abort
that has already flushed 17 tiles. It requires positive preparation reuse and
retains all in-flight byte-ownership assertions.

The C11 frozen build adds one four-byte backend counter; the new cache flag
uses existing padding. Its NTSC capture build links at `0x060DB83C`, leaving
1,988 bytes below the `0x060DC000` music-allocation boundary. The linked end is
476 bytes above C10. GPU texture capacity, decoded cache, DMA stage, logic,
audio and arcade-clock constants are unchanged.

Full native Royal/TAG comparisons remain necessary when changing the private
proof or preparation path. Host execution establishes output equivalence;
Saturn execution establishes performance.
