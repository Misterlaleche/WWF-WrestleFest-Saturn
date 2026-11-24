# C26 constructor recovery — 2026-11-13

This candidate recovers the literal C23 and C25 changes supplied in the user's
recovery Page. It starts from the measured C24 production source. Neither this
recovery nor native tests establishes a Saturn speed improvement.

## Source provenance

The recovered C23 commit is
`ae4f8ced42cb53ef23330220a891e79bee50e704`. Its `port/zlib/inftrees.c` is SHA-256
`01514df58d229f68e9116cbb0e94895c4ced868d396c662b98c06b36ab992489`, exactly the
identity supplied by the Page. The direct table differential and its pinned
C22 reference were restored from the same commit.

C25 replaces the original backward Huffman increment's scan, mask and
conditional addition with clearing each carried bit using XOR and setting the
final bit with XOR. Applying the literal supplied replacement to C23 produces
SHA-256 `854ed3593f7f2c748ac18efcced3f61f751ddd9d6c256bedd8f59d7f5e95fa10`,
exactly the combined source identity supplied by the Page. No recovered C26
binary or earlier C26 target result is used as fresh evidence.

Only `port/zlib/inftrees.c` changes in production. Packing preserves each
four-byte table entry's native representation, alignment and aliasing contract.
The carry change preserves all 65,534 legal `(len, huff)` inputs for lengths
1 through 15. Decoder table sizes, compressed assets, scheduling, game logic,
clock and display publications are unchanged by this patch.

## Independent verification

`tests/test_inftrees_store.py` compares the complete constructor state with
the immutable C22 reference: table, workspace, input lengths, result pointer,
return code, root width and guards. Its native-endian configuration and three
missing-macro configurations each run 80,688 cases under ASan/UBSan. Wrong
code-width and packed-word controls must fail with table-byte mismatches.

`tests/test_inftrees_carry.py` extracts the actual production increment block
and checks every legal input against the independent definition
`reverse((reverse(huff) + 1) modulo 2**len)`. Wrong carry start, omitted carry
clear and omitted final carry set must fail semantically. This is an exhaustive
local arithmetic check; the full constructor differential checks its context.

`tests/test_inflate_refill.py` now links a separately compiled, hash-pinned C22
table builder into the reference decoder, in addition to its existing pinned
C18/C22 fast inflater. `inflate_table` and `inflate_copyright` are namespaced
with the other reference symbols. Previously both decoders shared the current
builder, so their agreement alone could not expose a shared constructor error.
The existing public-ABI, streaming, dictionary, invalid-input, allocation and
resume comparisons now exercise independent builders. Candidate-only table
width and packed-word mutations provide semantic negative controls.
The public packed-word control changes only literal value bits, preserving
operation and width fields. An initial broader opcode-bit control stalled and
was terminated; that run is retained under `build/c26-public-packed-negative`
and is not counted as a successful negative control. The final controls have
a 60-second decoder-process bound; a timeout still fails the gate.

Commands from the repository root in the Linux test environment:

```sh
python3 tests/test_inftrees_store.py --output build/c26-tables-final
python3 tests/test_inftrees_carry.py --output build/c26-carry-final
python3 tests/test_inflate_refill.py --skip-codegen --build-dir build/c26-public-final-v2
```

The separate negative-control commands are expected to exit nonzero; inspect
their logs for a public status mismatch, not a setup error or sanitizer crash:

```sh
python3 tests/test_inflate_refill.py --skip-codegen --configuration normal \
  --table-mutant code-width --build-dir build/c26-public-width-negative
python3 tests/test_inflate_refill.py --skip-codegen --configuration normal \
  --table-mutant packed-bit --build-dir build/c26-public-packed-negative
```

The root task owns linked SH-2 code review, asset/worker and complete native
gameplay comparisons, frozen image construction and cold-boot captures. The
combined candidate still needs those results and an actual full Royal run,
including the common late window, before any performance acceptance.

## Fresh local results

The final source passed all four constructor configurations (322,752 table
cases, eight detected table-byte controls), the complete 65,534-input carry
domain and three detected carry controls. The final public-decoder runner
passed all three configurations in both plain and observed builds: each set
contains 934,312 paired calls and 205,201,170 compared output bytes. Normal
digest is `c3844107e57da1d5`; both permissive digests are `1a6755bc5ab8cbde`.
These match the recorded C24 generic results. All successful reports record
unchanged source inputs during their runs.

Both candidate-only public table controls fail semantically at
`canonical-boundary-444`, schedule 0, call 0: candidate status `-3`, independent
reference status `1`. The width-control receipt is in
`build/c26-public-builder-negative-before`; the corrected packed-literal receipt
is in `build/c26-public-packed-negative-final`. The unmutated final runner
receipt is `build/c26-public-final-v2/report.json`.

## Complete reproduced target result

The isolated restored C26 source produces the exact historical ELF and RAW
hashes from the recovery Page. Stock Mednafen cold boot completed the original
14,409-tick Royal route in15,193 combat fields, compared with15,207 for the
byte-identical rebuilt C24 baseline. Both match the native terminal and have
no reported audio issues. Against the native original-wait schedule of15,075
fields, C26 retains118 extra fields; arcade cadence is not yet achieved.

The predetermined late range8000..10000 has common observed endpoints8008 and
9988: C24 uses2151 fields, C26 uses2147. The corresponding cadence improves
95.86595%→96.04456%. These are asynchronous observed endpoints, not interpolated
or chosen to maximize the gain. Full captures, native224+ trace artifacts and
analysis are retained in the task workspace's `c24-baseline` and `c26-candidate`.
The original Page is preserved literally, including unified-diff context spaces.
