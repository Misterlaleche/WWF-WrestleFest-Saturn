# WP1979 isolated renderer micro-optimizations

## Proposal ready for native measurement

Copy only these two proposed translation units into the independent WP1979
native project, or apply `render-microopts.patch` from its runtime directory:

- `proposed/wf_r18_present.c`
- `proposed/wf_vdp1_plan.c`

No native project, reference tree, SDK, Makefile, or diagnostic-entry files
were modified by this work. `baseline/` contains byte-exact snapshots of the
two native source files at the start of this work.

## Changes and exactness

`add_cmd()` formerly cleared all 12 command bytes with `memset`, then
overwrote the first nine bytes. It now clears the bytes from `chain_index`
through the end of the object, including trailing padding, and writes all
preceding fields exactly as before. This remains a complete initialization
even when a previously used command slot contains arbitrary stale bytes.

`wf_vdp1_plan_retain_tile_slots()` now uses an eight-byte constant mask table
for the four expressions formerly written as `1u << (index & 7u)`. This is
the same strategy already used by the Saturn backend. The hash insertion,
collision order, uniqueness validation, resident duplicate handling, slot
selection, command remapping, and epoch management are unchanged.

No gameplay, sprite selection, draw order, clipping, palette selection,
DMA scheduling, VDP waits, or frame pacing behavior changes are proposed.

## Differential verification

Commands, run entirely inside this isolated directory:

```sh
python wp1979/render_opt/run_equivalence.py
python wp1979/render_opt/run_equivalence.py --sanitize
python wp1979/render_opt/inspect_assembly.py
```

Both normal and ASan/UBSan builds report:

```text
PASS retain_cases=8198 present_cases=4096 compared_commands=2478928 compared_tiles=1233733 full_plan_bytes=9376 digest=13495aa1dbe19bec
```

The same digest was obtained by running the test before making any changes
(baseline versus baseline). The tests compare status and **all bytes** of
each host frame plan, including counted prefixes, inactive tails, and padding.
The 9,376-byte host plan includes 64-bit `size_t`; the Saturn plan is smaller,
while the command object and its field layout remain 12 bytes.

Coverage includes:

- 8,192 sequential retention calls, followed by invalid-argument cases.
- Tile counts from zero through 256 and command counts through 512.
- More than 60 lookup-epoch wraps, including failures between successful calls.
- Dense and sparse resident masks, all-clear/all-set masks, variable resident
  counts, and repeated source IDs in distinct old resident slots.
- Current duplicate source IDs and out-of-range command tile references,
  preserving `WF_BAD_STATE` and partial-plan state.
- Deliberate hash collisions in buckets 511, zero, and one, exercising
  linear-probe wraparound.
- 4,096 sequential complete production `submit_records()` calls, with exact
  source text included independently for baseline and proposal.
- Duplicate sprites, 256 distinct tiles, 512 commands, over-capacity failure,
  clipping edges, chain expansion, horizontal/vertical flips, inactive
  records, all palette bit positions, stale command memory, and palette ACKs.
- An independent identity check proves every remapped command still names
  the same source tile and keeps all other command bytes unchanged.

GPU sinks validate the complete counted upload/emission contract. They do
not emulate the GPU or claim a visual/performance result. Native emulator
measurement and video verification remain the root agent's task.

LeakSanitizer cannot enumerate `/proc/<pid>/task` in this environment. The
sanitized run disables only leak scanning (`detect_leaks=0`); AddressSanitizer
and UBSan remain enabled. The test owns no heap allocations.

## SH-2 assembly evidence

`inspect_assembly.py` uses GCC 13.2.0 with the production `-O2`, `-m2`, `-mb`,
`-ffreestanding`, aliasing, section, and Saturn platform flags. LTO is omitted
only so the final per-unit machine instructions can be inspected without
rebuilding the shared Saturn image.

| Function | Baseline text bytes | Proposed text bytes | Relevant result |
|---|---:|---:|---|
| `submit_records` | 1,164 | 1,112 | Per-command `memset` removed; epoch-wrap clear retained |
| `wf_vdp1_plan_retain_tile_slots` | 844 | 836 | All `__ashlsi3_r0` references removed |

The proposed command tail clear compiles to:

```asm
mov     #0,r0
mov.b   r0,@(9,c)
mov.w   r0,@(10,c)
```

The compiler combines the final two bytes into an aligned word store.

The existing native ELF was inspected read-only and its disassembly saved as
`build/native-before.dis`. Its production `submit_records` has `mov #12,r6`
and an indirect call to `_memset` for every visible command. For an aligned
12-byte object, that linked implementation executes 64 instructions inside
`memset` (including return and its delay slot), plus call/setup/spill work.
The proposed clear uses three instructions including loading zero. The
direct modeled saving is roughly 65 SH-2 cycles per command before reduced
spills and surrounding compiler scheduling; this is an instruction-level
estimate, not a measured frame-time claim.

The original mask helper path calls `__ashlsi3_r0`, whose software branch
table executes six to ten instructions for shifts zero through seven, plus
the call. The proposed mask lookup has no call and uses a byte load. Its
expected saving is on the order of 6–10 cycles for each mask evaluation,
subject to compiler register allocation and memory/cache stalls.

Illustrative scale only: 200 visible commands would remove around 13,000
modeled cycles from their clears alone, about 0.48 ms at a 26.8 MHz CPU clock.
The actual native busy-window profile is required to determine frame-rate
improvement and whether the 57.44485 fps target is met.

## Evidence files

- `build/equivalence.json`: normal run and exact source SHA-256 values.
- `build/equivalence-asan.json`: ASan/UBSan run and source SHA-256 values.
- `build/assembly.json`: code-generation flags and function-size results.
- `build/baseline-*.s` and `build/proposed-*.s`: inspectable SH-2 assembly.
- `render-microopts.patch`: narrowly scoped patch; source copies retain CRLF.

## Next candidate only if native measurement still needs it

A cached successful source-order fast path in tile retention could avoid
rebuilding and probing the hash when the input source sequence repeats.
It requires proving uniqueness from a previous successful plan, retaining
the original duplicate-error behavior, validating every command index,
checking current resident bits/order, and preserving output offsets. It is
not included in this proposal. The two changes above should be measured
first so any additional complexity is justified by evidence.
