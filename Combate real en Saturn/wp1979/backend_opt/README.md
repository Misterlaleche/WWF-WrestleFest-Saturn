# Exact 8-pixel VDP1 column trimming proposal

## Integration

The only proposed production files are:

- `proposal/wf_saturn_yaul_backend.c`
- `proposal/wf_saturn_yaul_backend.h`

The review diff is `column-trim.patch`, based on the saved copies in `baseline`.
`baseline-source.json` identifies their original hashes. No presentation,
slot-planning, game logic, emulator, CPU clock or synchronization code is
changed by this proposal.

## Behavior and limits

If either complete eight-pixel source half contains only transparent pen0,
the other half is packed into the beginning of its existing128-byte staging
slot. Its hardware command uses width8. Fully empty tiles retain the prior
behavior of consuming a logical command without emitting a hardware sprite.
Tiles with visible pixels in both halves keep width16 and their raw layout.

The existing top/bottom crop is preserved. Packing begins at the slot base,
so an odd original top row does not violate the command source address's
eight-byte alignment. Source half `left` is0 or8; screen X is adjusted by
`left` without a horizontal flip, or `16-left-width` with a horizontal flip.
Y continues to use `top` or `16-top-height`. PenF remains a visible color;
end-code handling and transparent pen0 are unchanged.

All VRAM slot addresses,128-byte DMA strides, dirty runs and residency rules
remain unchanged. Texture transfer volume is not reduced. The benefit is
fewer transparent pixel positions traversed by VDP1.

The old `crop_top` byte becomes `crop_meta`: five bits for top, one for width8
and one for the right source half. The SH-2 backend structure remains exactly
1,684 bytes. Static backend object BSS is unchanged at16,544 bytes. At the
supplied compiler's `-O2`, object text grows from2,432 to3,024 bytes (+592).

The immutable ROM source-ID fast path is unchanged. A generic upload must
compare its original16×16 pixels with the logical picture represented by
compacted staging, including all discarded zero pixels. A raw128-byte
`memcmp` against compacted staging would permit false hits, so narrow cached
tiles use a logical comparison. The test explicitly constructs that false-hit
case. Same-source ROM hits do not decode, rescan or repack the tile.

There is additional CPU work on changed uploads: a bounded half-width scan
and at most64 copied bytes. Native capture must determine the net timing
benefit. Pixel-area reduction alone does not establish a frame-rate gain;
the existing GPU-wait measurement also includes idle time.

## Verification

`tests/test_backend.c` compiles the actual Saturn branch of the proposed C
file against a narrow Yaul hardware mock. One independent reference draws
the original16×16 tile. Another draws only the resulting hardware commands
and copied VRAM; neither uses crop metadata to decide expected pixels.

Coverage includes all four flip combinations, every top/bottom span, left,
right and full-width shapes, all individual pixel positions, penF, empty
tiles, negative/edge clipping, raw unaligned input, overlapping sprites,
generic and immutable ROM identity, duplicate pixels under different IDs,
changed crop origin, abort/incomplete commits, dirty DMA runs, slot255 and
out-of-range requests. Guard regions protect staging and neighboring VRAM.
Deferred submission has no added GPU wait, and immediate submission retains
exactly one sync/wait pair.

The historical6,381-tile atlas was independently decoded against
SPRITES.PAK: every tile matched. Its four-flip atlas tests contribute25,524
comparisons; synthetic/cache tests bring the total to28,203. The original
backend fails the expected width8 assertion; the proposal passes the full
raster/cache suite and SH-2 compilation with `-Wall -Wextra -Werror`.

ASan and UBSan pass the same tests. LeakSanitizer cannot run under this
environment's ptrace monitor, so only leak detection is disabled with
`ASAN_OPTIONS=detect_leaks=0`; address and undefined-behavior checks remain on.
The harness uses static staging/VRAM buffers, not heap-owned renderer data.

## Asset area evidence

`atlas-evaluation.json` covers the historical6,381-tile atlas:719 tiles use
only their left half,774 only their right half,4,885 both halves and3 are
empty. Width trimming removes116,160 of1,330,064 pixel positions after the
existing vertical crop, an8.7334% reduction when each unique tile is counted
once.

`historical-weighting.json` weights the same tiles by presence per frame in
the historical `tag-after3600-tiles.csv` trace and gives11.3452%. That trace
does not count repeated draw commands, and it predates the corrected RNG
fixture. It is not a final native performance measurement.

The current corrected6,450-tile atlas is separately verified in
`corrected6450-evaluation.json` and
`tests/corrected6450-sanitized-result.json`.

## Reproduction

From the workspace root, decode and verify the corrected atlas:

```sh
python3 wp1979/backend_opt/evaluate_atlas.py \
  wp1979/audit_gameplay/fixes/routes/tag-after3600/HOTSPR-32.BIN \
  wp1979/audit_gameplay/assets/SPRITES.PAK \
  wp1979/backend_opt/corrected6450
```

Compile the host raster/cache harness with the actual proposed backend:

```sh
gcc -O1 -g -std=c11 -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DEXPECTED_ATLAS_TILES=6450u \
  -DBACKEND_SOURCE='"/workspace/scratch/1e822aeaeac7/wp1979/backend_opt/proposal/wf_saturn_yaul_backend.c"' \
  -Iwp1979/backend_opt/tests/mock -Iwp1979/native/src/runtime/include \
  wp1979/backend_opt/tests/test_backend.c \
  wp1979/native/src/runtime/src/wf_saturn_memory.c \
  -o wp1979/backend_opt/tests/corrected6450-sanitized
ASAN_OPTIONS=detect_leaks=0 wp1979/backend_opt/tests/corrected6450-sanitized \
  wp1979/backend_opt/corrected6450-decoded.bin
```

The corrected fixture's per-frame tile-presence histogram gives 11.6361% reduced area (`corrected6450-weighting.json`). This still is not weighted by repeated draw commands or a substitute for native timing. Its complete sanitizer run passes28,479 raster cases across28,481 submitted frames, including all6,450 tiles in four flip combinations.
