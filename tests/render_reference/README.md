# Immutable B12 renderer references

These two complete source files are byte-exact copies from the restored B12
reference (initial repository revision `805483a`). They are compiled with
renamed exported symbols; the differential tests call their actual functions.
They are not reconstructed implementations or copied expected-output tables.

| Reference | SHA-256 |
| --- | --- |
| `b12_wf_jo_video.c` | `62e778c59a67ec918480a8c3e1db85d163b6d2fb864ab34c05fe801d26019d62` |
| `b12_wf_r18_metasprite.c` | `7575c8982e3589c0fd3e15d0182bde279d4a35200856e1f864efddc6dde04494` |

From the source ZIP root, run:

```sh
python3 tests/run_jo_render_tests.py
```

The runner uses Python's standard library, a C compiler with ASan/UBSan, the
sources in this tree, `cd/ROM68K.BIN` and `cd/SPRITES.WFP`. It needs no Git or
historical workspace. `--case` selects individual gates; `--source`, `--output`
and `--cc` override the source tree, evidence directory and compiler.

The crop gate decompresses every sprite block with Python zlib, verifies block
bounds and the pack index checksum, and requires the 8 MiB B12 tile corpus hash
`130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31`.
All 65,536 tiles are compared at eight pointer alignments; synthetic patterns
also exercise metadata overlapping the tile buffer. All 128 output bytes and
canaries are compared, including untouched tails.

The parser gate compares every record byte, cache state, read sequence, status,
D6/D7 and rollback against B12, including repeated programs, changed palettes,
clipping, capacity failures, callback mutations and unaligned record buffers.
The cache policy, scalar/bulk preparation, command batches, pixel geometry and
map/texture lifecycle gates use independent assertions or differential paths.
The real dual-CPU SDK device gate is under `tests/jo_sdk_pipeline/`.

These are correctness gates. They do not claim target timing or hardware speed.
