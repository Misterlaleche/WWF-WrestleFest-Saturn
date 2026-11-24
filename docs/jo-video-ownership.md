# Jo Engine video ownership

The native presenter retains its legacy `wf_saturn_yaul_*` function names solely
as an internal C ABI. `port/wf_jo_backend.c` implements them using Jo Engine
2026.3. It contains no Yaul calls or link dependency. The original backend is
excluded from `core_sources.mk`.

## Submission and lifetime

The native frame budget is 256 unique 16 × 16, 4-bit sprite tiles and 512 sprite
commands. The backend crops transparent top/bottom rows and packs a wholly
transparent left or right half into an 8-pixel-wide sprite. Pixel tests cover
both widths, all flips, negative screen coordinates, and color 15. End codes
are disabled so color 15 is drawn. Pen zero stays transparent.

Jo owns both command tables, the command sort/DMA pipeline, interrupts, VDP1
framebuffer changes and frame timing. The adapter appends complete 32-byte
commands through `__jo_vdp1_reserve_commands()` and
`__jo_vdp1_commit_commands()`. All depth keys are equal, preserving arcade
submission order through Jo's stable sort. `jo_core_run()` ends the frame;
the backend does not call a second graphics synchronizer.

A frame leases one of four 32 KiB texture banks. Its last use is the Jo
building frame tag, including repeated frames. Four banks cover the displayed frame,
two newer Jo submissions and the frame currently being built. A bank is released only after
Jo reports a **strictly later** displayed frame tag. Equality is insufficient:
VDP1 may still repeat its current command table. The comparison permits tag
wrap. No extra field wait occurs when a bank is free. If all four banks are in
flight, acquisition keeps interrupts enabled and waits for a displayed-frame
fence, with an eight-field error deadline.

Texture contents have independent versions for each logical slot and each
physical bank. A released bank receives only changed, contiguous tile runs.
CPU tile residency never grants permission to overwrite an in-flight bank.
No allocations occur during frame submission, repetition or palette/map
publication.

The 256 logical texture slots persist between frames. A source already in a
slot keeps that slot even when the current frame has fewer tiles. New source
IDs remain temporary until the complete visible command list is known; only
then can they replace unused slots. `port/wf_jo_tile_slots.h` uses a bounded
1,024-entry index and closes deletion gaps instead of accumulating tombstones.
Unreferenced slots keep their CPU contents and each physical bank's version.
Their submitted version is zero, which skips transfer; live versions never
use zero. This avoids loading unused tiles or compacting the whole stream.

The backend and repeat path borrow Jo's existing CPU command prefix. This
requires the single-CPU Jo configuration: its `__jo_vdp1_end_frame()` waits
until the SCU DMA has finished before the next callback can reuse that prefix.
Sorting changes an index array, and the DMA builder reads commands without
modifying them. Our normal sprites do not enter Jo's optional wireframe
polygon conversion. The CPU prefix remains intact after Jo resets its count.

The adapter reserves the whole expected command capacity before opening the
workspace. It requires Jo's command count to be zero and rejects another
writer or a submission in a different building tag. Opening invalidates
replay before either pixels or commands can change. Submission commits this
prefix directly; repeat reserves that same prefix again, without copying it.
Generic callers with separate command storage still copy into Jo's prefix,
and overlapping shifted source/destination ranges are rejected. No other Jo
drawing API may append while this adapter owns the game display. Jo's later
DMA reads the committed prefix until its end-frame wait completes; VDP1 then
reads the VRAM copy. A successful submission restores replay. An aborted build leaves replay/clone blocked
until another complete frame arrives, preventing old commands from being
combined with partially changed staging pixels.

## Memory map

| Owner | Offset in its VRAM | Size |
| --- | ---: | ---: |
| Jo VDP1 command tables | `0x00000`–`0x0ffff` | 64 KiB |
| Texture bank 0 | `0x10000` | 32 KiB |
| Texture bank 1 | `0x18000` | 32 KiB |
| Texture bank 2 | `0x20000` | 32 KiB |
| Texture bank 3 | `0x28000` | 32 KiB |
| Jo Gouraud area | `0x7e4f8` and above | Reserved |
| VDP2 foreground characters, A0 | `0x00000` | 128 KiB |
| VDP2 names group 0, A1 | `0x20000` | 24 KiB |
| VDP2 names group 1, A1 | `0x26000` | 24 KiB |
| VDP2 names group 2, A1 | `0x2c000` | 24 KiB |
| VDP2 names group 3, A1 | `0x32000` | 24 KiB |
| VDP2 back color, A1 | `0x3fffe` | 2 bytes |
| VDP2 background characters, B0 | `0x40000` | 128 KiB |
| VDP2 HUD characters, B1 | `0x60000` | 128 KiB |

Within each map group, foreground names are at `+0`, background at `+0x2000`,
and HUD at `+0x4000`. Each world map uses 1,024 one-word pattern names and
16 × 16 characters with flip bits. The HUD uses 4,096 one-word pattern names,
8 × 8 characters and 12-bit glyph IDs. CRAM mode 2 preserves all 4-bit arcade
DAC components in RGB888, with independent 256-entry foreground, sprite,
text and background domains.

The four map groups share texture-bank fences. CPU code updates canonical
name tables, scroll, priorities and colors. `wf_jo_video_commit()` copies only
changed maps into the released bank and queues its immutable palette/register
snapshot under that frame tag. In Jo's VBlank callback, after Jo has advanced
its own VDP1/VDP2 handlers, the adapter publishes only the snapshot belonging
to the newly displayed sprite frame. A changed palette costs one 4 KiB DMA;
unchanged palettes cost none. Jo's VDP2 extension writes its existing register
shadow and flushes it during that same VBlank.

The explicit extension is `sdk/jo_engine/jo/wf_video.h` plus the final section
of `sdk/jo_engine/vdp2.c`. Fixed cycle slots provide the three pattern-name
reads in A1, their character reads in A0/B0/B1, and CPU access after an idle
slot. Ordinary Jo image/palette allocation and printf must not run while the
game owns these planes. `wf_jo_video_diagnostics()` restores Jo's font, CRAM
mode and NBG0 configuration for error text.

## Repeats, HUD updates and scene changes

`wf_jo_video_repeat()` resubmits the saved complete sprite list and refreshes
its texture fence. Repeated calls within one Jo building tag do not duplicate
commands. HUD/palette-only commits acquire another released bank, reuse its
matching textures when possible, and publish changed VDP2 state with the same
sprites. Failure to reserve command capacity rolls back the saved command
addresses.

A second state change after a frozen repeat has already been queued in the
same building tag returns `WF_BLOCKED`; it must be retried in the next tag.
This protects maps/palettes still referenced by the current display. A fresh
sprite submission's unpublished snapshot can be refined within its own tag.

A scene replacement gates VDP2 layers and sprite priority before character
data is replaced, keeping the display clock running. Initial video setup can
also blank TV output.
The first new submission latches a resume tag; later submissions do not move
that target. Output resumes only with that frame's matching names, palette
and sprites. The native Royal HW1 viewport correction is preserved across
all layers: original source Y8 through Y247 maps to Saturn Y0 through Y239.
Sprites receive -8, world scroll receives +8, and HUD scroll is 8.

Ring character conversion preserves the original four-quadrant 16 × 16 layout
but uses aligned 32-bit reads/writes; an unaligned fallback remains available.
The boot-only font reader opens `FONT8.BIN` once and reads sequentially through
a 2 KiB HWRAM buffer before DMA to VDP2. It does not reopen and seek for each
sector. All font/scene/initializer reads should finish before priming CD music.

## Measurements and validation

`wf_jo_video_stats` exposes submitted/repeated/displayed frames, total and
last texture bytes, texture transfer count, map/palette bytes, bank waits and
waited fields, and fields without a newly displayed frame while a queued
snapshot exists. The latter can include a main-thread submission not yet
handed to Jo's end-frame function; use it together with native tick/field
counters to diagnose pacing, rather than treating it as a direct GPU timer.

Run `tests/run_jo_video_tests.sh` after generating the seven scene packs in `cd`.
The tests use AddressSanitizer and UndefinedBehaviorSanitizer:

- Crop/flip pixels, transparent tiles, fixed viewport, bank bounds and fence
  wrap behavior.
- The actual driver with a deterministic Jo boundary. Its DMA mock rejects any
  write into a displayed or pending texture/map bank. It checks four queued
  frames, exact palette timing, repeat protection, sparse transfers, repeated
  commit idempotence, inactive-slot preservation, shared-command aborts and
  failure rollback.
- Persistent slot assignment with late references, rejected 257th unique
  tiles, aborted plans, hash collisions/deletion and 1,800 randomized frames
  spanning multiple epoch wraps.
- All seven scene packs through aligned and unaligned character conversion,
  independent quadrant pixel decoding, pattern-name composition and Royal
  dirty-cell updates.

The production video files and the VDP2 extension compile with the supplied
SH-2 GCC 8.2.0 and this project's Jo options. These tests establish code and
memory-lifetime behavior; actual TAG/Royal frame timing and transition duration
still require the integrated Saturn/emulator captures.

The complete native replays now use `port/wf_jo_backend.c` on the host, including
its real crop/command builder. `WF_JO_TEST_COMMANDS` records both a canonical
command digest (source IDs, coordinates, flips, palette and draw order) and a
digest of emitted hardware fields plus cropped pixel bytes. The stable-slot
change matches both digests byte for byte across TAG's 8,538 sampled states
and Royal's 8,475, including their genuine endpoints at ticks 8,537 and 8,474.
Evidence is in `evidence/stable-slots/comparison.json` and its `before`/`after`
replays. These comparisons establish rendering equivalence, not console FPS.
