# SATURN MEMORY / CDFS INTEGRATION R01

Status: **CANDIDATE contract for master integration**. This document does not claim a complete game build or hardware validation.

## Authority consumed

1. `wwfwfest.zip` reconstructed main SHA1 `a726c46f350723ea73158bc78055d17d418b14e2` (reverified locally).
2. CLEAN-R01 WP32 FINAL and WP33 FINAL.
3. Graphics25 FINAL and Renderer26 FINAL.
4. Audio24 FINAL plus the repository hardware-validation record.
5. SATURN-TECH-AUTHORITY-R01 / supplied libyaul.
6. Existing gameplay RUNTIME-11 only as a lower-authority implementation checkpoint.

## Superseded baseline detail

WP32 was created before Graphics25/Renderer26 FINAL were available to that audit. It therefore recorded a WIP160 CRAM-mode-1 baseline. Renderer26 FINAL is newer/higher for renderer implementation and explicitly requires **CRAM mode 2**, with four complete 256-entry RGB888 windows: TEXT, SPRITES, FG_WORLD and BG_WORLD. This integration contract uses mode 2. WIP160's mode-1 layout is not promoted.

## Physical ownership

- HWRAM: runtime/code/data/heap plus mandatory graphics transfer staging.
- LWRAM: CDFS bulk/packed actor residency/overlays; never an SCU-DMA endpoint.
- VDP1 VRAM: Renderer26 only.
- VDP2 VRAM: Renderer26 only.
- VDP2 CRAM: Renderer26 only.
- SCSP RAM: Audio24 only.

Cross-subsystem code receives owner APIs/offsets; it does not allocate private addresses inside another owner's memory.

## VDP2 capacity closure

Renderer26 proves the active worst-case cache sizes required without dropping source tiles:

- FG world: <= 1024 16x16 4bpp tiles = 131072 bytes.
- BG world: <= 1024 16x16 4bpp tiles = 131072 bytes.
- text: <= 2048 8x8 4bpp cells = 65536 bytes.

Using libyaul's exact page-size rules, reserve two-word PND storage:

- FG 2x2 page: 4096 bytes.
- BG 2x2 page: 4096 bytes.
- TEXT 1x1 full page: 16384 bytes (the arcade source uses a 64x32 subset; reserving the full VDP2 page changes no visible behavior).

The resulting fixed technical map occupies 352258 bytes including the 2-byte back color and leaves 172030 bytes unassigned/reserved. No asset is reduced or deduplicated by approximation to make this fit.

## CRAM closure

Renderer26 FINAL uses all 1024 CRAM mode-2 entries exactly as four 256-entry windows. Capacity is exactly 4096 bytes. No fifth palette owner exists. Dynamic arcade palette resource allocation must still preserve its source semantics when updating the SPRITES window.

## VDP1 remains fail-closed

Renderer26 proves source sprite-list semantics (`512` records max, chain up to `7`) and permits only exact whole-command offscreen culling. Static evidence does **not** prove a maximum simultaneous number of visible VDP1 commands or unique texture cells small enough to assign a final partition without runtime evidence.

Therefore:

- do not promote RUNTIME-11's 1024-command / 64 KiB dynamic texture partition to project-wide FINAL;
- keep required command count and unique texture-tile count measurable per frame;
- reject overflow rather than drop/reorder sprites;
- close the final VDP1 partition only after authoritative runtime reachability/traces demonstrate maxima with safety margin and real Saturn validation.

## LWRAM/HWRAM

WP32 proves the six-wrestler packed-residency policy. Worst current 6P combination: five LWRAM residents plus 128 KiB overlay reserve = 903168 bytes. A sixth actor may reside in HWRAM when required. This is a proven capacity policy, not permission to SCU-DMA from LWRAM.

Any bytes destined for VDP1/VDP2 via SCU DMA must be staged in HWRAM first. The included `wf_saturn_platform_memory.*` guards that rule.

Final HWRAM fit remains dependent on the linked ELF/map (`__end`/static usage) plus scene arenas, gameplay structures, audio stream staging and any sixth-fighter overflow.

## SCSP

Audio24 owns the entire 512 KiB sound RAM. The current closed layout is recorded in the integration tables; the music ring is `0x60000..0x6FFFF`, physical slot 31, two 32 KiB halves. Graphics/gameplay must never write SCSP RAM.

## CDFS / loading

Current proven CDFS contract:

- 2048-byte sectors;
- current file-list capacity 384;
- size checks use sector-rounded byte counts;
- final playable media, when explicitly requested, is verified MODE1/2352 BIN/CUE; ISO is intermediate;
- no CDDA tracks are part of the current production design; music is WP24 SCSP streaming.

Loading policy:

1. use the original arcade transition as the load window;
2. preload during existing fades/presentation where proven and safe;
3. retain common assets only when the measured budget permits it;
4. release owner-qualified temporary assets before loading replacements;
5. visible LOADING is allowed only after a measured Saturn requirement and must be marked `SATURN PLATFORM ADAPTATION`.

No scene may become visible before all required destination resources are verified ready.

## Scene matrix status

Frontend exact byte counts remain OPEN where the current block outputs do not yet expose a canonical asset manifest. Those values must be imported from the final block source/artifact rather than estimated from screenshots or historical TITLE.ANM captures.

## Gates before master build

1. gameplay checkpoint imported from its canonical branch/checkpoint;
2. final linked ELF/map yields HWRAM static end;
3. VDP1 maximum command/unique-tile reachability proven;
4. every scene has an exact asset manifest with load/release lifetime;
5. CDFS root count <= configured capacity;
6. no owner overlap in VRAM/CRAM/SCSP;
7. no LWRAM SCU-DMA route;
8. host + SH-2 compile/link pass;
9. real Saturn + MODE validation for transfer windows and visible atomicity.
