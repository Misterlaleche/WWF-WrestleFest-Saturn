# Runtime18 disc build

## Latest checkpoint: hardware scrolling and resident sprites

The ring retains a 512x256 bitmap per layer and uses VDP2 scroll registers.
The CPU updates exposed strips and changed overlay cells. Host tests compare
the visible pixels with the ROM-derived lookup across wrapping, negative
positions, transparency and overlay writes/restores.

The build automatically generates HOTSPR.BIN from the checked SPRITES.PAK
SHA-256 and `src/runtime/validation/runtime18-hot-tiles.txt`. Its 1,204 sorted
tile IDs were observed in no-input frames 0–253 and controlled frames 0–359.
The 4,096-byte header contains big-endian IDs; each following 128-byte tile is
copied unchanged. Output size: 158,208 bytes; SHA-256:
`a82837ce3408690ec1e0c66f93659d5413234f9fc2b6880377568e708e2445a6`.
Startup reserves 154,112 bytes of the existing 384 KiB tile cache for these
tiles. All other IDs retain exact on-demand access to SPRITES.PAK.

The HOST combat probe now uses the Saturn cache capacity and preload path.
The new test compares every hot tile with the original pack, checks zero
pack reads for resident tiles and cold-cache behavior, and rejects malformed
headers, duplicate IDs, invalid counts and failed payload reads.
Run `make -C src/runtime test-runtime18-hot-sprites
test-runtime18-ring-scroll-cache` alongside the existing checks.

A temporary Mednafen diagnostic build measured VBlank 558 at frame 30 and
937 at frame 120. The first figure includes startup. Frames 31–120 average
4.21 VBlanks/frame, versus 6.86 at the previous checkpoint: about 39% less
time. Hardware scrolling alone measured 6.57. Diagnostic stops were removed
before the final build. These are emulator measurements of one no-input
sequence, not physical Saturn results. The CPU fail-closed boundaries below
remain open; no complete match or ROM-EXACT claim is made.

Run `DOCKER=/path/to/docker validation/build-runtime18.sh /path/to/sh2eb-elf`.
The script uses the pinned Yaul Docker image, the supplied SDK read-only, and host
Python 3. Outputs are under `validation/results/runtime18/`. The build emits a
single MODE1/2352 track with no CD audio. Place runtime CDFS files under
`src/runtime/audit/runtime18-cd/` before building.

`Makefile.saturn-disc` uses the SDK's `make-ip` and `make-iso`, the boot path
recorded in WP33 and WIP160: master stack and initial load address 0x06004000,
slave stack 0x06001E00. The encoder is copied from the read-only WP33 FINAL
package's `tools/iso_to_bin_exact.py`; the verifier comes from that package,
adapted only for Runtime18 paths plus an initial-load-address check.

Validation compares every raw sector including sync, address, EDC and ECC,
checks the Saturn/IP and ISO9660 signatures, and compares every CDFS payload
against its source, including A.BIN against the linked binary. This is a disc
structure check, not evidence of gameplay or hardware execution.

The reference archive is `sources/zips finales.zip`, nested
`zips finales/WF-CLEAN-R01-33-SATURN-BOOT-CDFS-BUILD-DISC-LAYOUT-FINAL.zip`.

## Direct-combat checkpoint (2026-10-20)

This checkpoint boots straight into the ring, without menus or audio. The
Saturn path reads the original arcade program and sprite pack from CDFS,
draws the ring through VDP2, and draws the wrestlers through VDP1. Its CPU,
linked holds, throw, and damage branches used by the tested sequence are
implemented from the canonical 68000 ROM. The temporary diagnostic color
screen means a ROM dependency was reached; it must not be interpreted as a
completed match.

From the repository root, reproduce the host integration checks with
`make -C src/runtime test-runtime18-direct`. That target requires the three
local files already under `src/runtime/audit/runtime18-cd/`:
`ROM68K.BIN` (524288 bytes; SHA-256
`265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`),
`SPRITES.PAK` (8388608 bytes; SHA-256
`130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31`),
and `R18RING.BIN` (114918 bytes; SHA-256
`774fbc5d9d0b404d1b3fdd825581efb5de892ab15504ecb257190dfa6b30191c`).
The check runs 200 frames without input and 300 frames with RIGHT on frames
60–179 and A on frames 180–183. In the controlled run, actor 4 loses three
stamina points (145 to 142). This is a host integration observation, not a
Saturn rendering or hardware proof.

The known fail-closed boundaries are CPU `$1F7F0` at host frame 255 with no
input and CPU `$1C3FC` at frame 361 in the controlled sequence. Both are
ROM-owned branches, and neither has been replaced with a guessed gameplay
path. In Mednafen, the final BIN/CUE booted through Saturn BIOS and CDFS,
displayed the ring and wrestlers, and visibly reached a punch and linked hold.
The later green/cyan diagnostic screen coincides with the unresolved CPU path;
`validation/results/runtime18/mednafen-ring.png` records the last captured
combat scene. Thus the emulator visual check is partial, not a complete-match
pass. Physical Saturn CD-R execution remains untested. This checkpoint is not
ROM-EXACT and is not certified for physical hardware.

Keep the generated `.bin` and `.cue` together and launch or burn the `.cue`.
It describes one MODE1/2352 data track with no audio tracks. The file
`validation/results/runtime18/disc-validation.json` records sector-level and
CDFS-payload checks plus exact hashes of the generated pair.

## CD and ring performance checkpoint (2026-10-20)

The Saturn build now reads 16 aligned sectors per CD command for large file
transfers, keeps eight eight-sector CDFS windows, and preloads the exact
512 KiB arcade program into low work RAM. The remaining 384 KiB low-RAM tile
cache retains sprite bytes on demand. A fully associative fallback ROM cache
also preserves hot pages for host and non-preloaded callers. The ring renderer
decodes each 16-pixel tile run once per scanline, rather than doing a page,
overlay, and tile lookup for every pixel. Host checks compare the optimized
scanlines with the original ROM-derived pixel lookup, including wrapping,
flips, transparency, and an overlay tile.

Temporary diagnostic builds measured the first 30 game frames in Mednafen:
2,980 Saturn VBlanks at the previous checkpoint and 655 with these changes.
The corresponding CD command count fell from 348 to 43. At frame 120 the
optimized diagnostic build had consumed 1,272 VBlanks, so frames 31–120 still
averaged 6.86 display refreshes per game frame. These are measurements inside
Mednafen, including startup in the first-30 count; they do not establish
hardware speed or a fluid frame rate. The diagnostic overlays and forced
30/120-frame stops were removed from the delivered BIN/CUE.

Reproduce the new checks with `make -C src/runtime test-runtime18-cdfs
test-runtime18-rom-cache test-runtime18-ring-scanline`; run the existing
`test-runtime18-direct` target for the no-input and controlled-input combat
sequences. The game still has the CPU-owned fail-closed boundaries described
above. Native VDP2 tilemap scrolling and sprite-data scheduling remain the
principal integration work for a fluid match. The BIN/CUE is for emulator and
CD-R testing, not ROM-EXACT or hardware-certified.
