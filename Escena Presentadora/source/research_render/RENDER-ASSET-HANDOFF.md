# WP02 native render asset handoff

## Deliverables

`generated/wp02_assets.h` and `generated/wp02_assets.c` expose immutable native metadata. The runtime preloads three files once:

| File | Size | Native destination |
|---|---:|---|
| TEX.BIN | 381,056 bytes | VDP1 texture partition; original 16×16 actor cells and 12 distinct resident wordmark textures |
| CRAM.BIN | 4,096 bytes | VDP2 CRAM mode 2; big-endian `[CC,B,G,R]` bytes |
| RING.BIN | 131,072 bytes | NBG0 indexed 512×256 bitmap; first 320×240 is visible |

The ring bitmap uses palette entries 768 onward, with 54 actual colors. The 25 sprite/text palette banks occupy entries 0–399. All original xBGR444 channels are replicated to eight bits with no quantization. `CRAM.BIN` follows the supplied Yaul `rgb888_t` physical layout, not conventional `0x00RRGGBB` integer ordering.

The atlas plus 1,024 VDP1 commands and alignment occupies 413,856 bytes of 524,288-byte VDP1 VRAM, leaving 110,432 bytes free. No scene-time CD read or texture upload is required.

## API

Each `wp02_piece_t` contains atlas index, signed X/Y offset, absolute CRAM entry base, flip bits, an opacity flag, and explicit `uint16_t width,height`. Atlas address remains `texture_base + tile*128`, including variable-size textures. Actor and foreground cells are 16×16. Each static name, the common VS, and CREDIT 0 is precomposed from the exact original 8×8 glyph indices into one native VDP1 texture. Width is a multiple of 8; height is 1–255; neither scaling nor resampling occurs. Use the explicit dimensions for culling and `vdp1_cmdt_char_size_set`.

- `wp02_pose_find(raw_gfx_set, raw_selector)` masks the selector's mirror bit and returns a frame or null for an unavailable pose.
- `wp02_name_find(raw_identity, logical_slot)` returns the ROM-produced name wordmark for that identity and slot 0–3.
- `WP02_FOREGROUND_FRAME` is the exact 60-cell transparent BG_WORLD layer for this scene.
- `WP02_CREDIT_FRAME` is the original CREDIT 0 text.
- `WP02_VS_FRAME` is the original common VS wordmark.
- `WP02_CAPTURE_NAMES_FRAME` is only the reference fixture's complete text layer, retained for independent validation.

For pose mirroring, negate the descriptor X coordinate and XOR flip bit 0. Do not subtract another 16 pixels. Place actors at `X-480`, `232-(Y+Z-560)` using the high word of each native 16.16 field.

The runtime's raw identities already equal graphics-set numbers:

| Raw ID | Wrestler |
|---:|---|
| 0 | Hulk Hogan |
| 1 | Ultimate Warrior |
| 2 | Jake Roberts |
| 3 | Big Boss Man |
| 4 | Hawk |
| 5 | Animal |
| 6 | Sgt. Slaughter |
| 7 | Mr. Perfect |
| 8 | Earthquake |
| 9 | Ted DiBiase |
| 10 | Demolition Smash |
| 11 | Demolition Crush |

The normal selectable roster excludes raw 4/5. No old UI permutation should be applied to the new scene's actor type.

## Exact sources

The exporter imports the independently audited WP25 `Rom` decoder and verifies the supplied arcade ROM hash. Actor and presenter composition metadata comes directly from numeric ROM tables. Presenter set 0x1F includes all 21 real poses, including the exact terminal selector families 8–11, 4–7, and 13–16. The historical seven-image WIP presenter is not used.

The opaque ring and transparent foreground come from the actual MAME 0.264 paid TAG snapshot in `verification/mame/full/frames.bin`, index 0, frame 3249. Both tilemaps and palettes remain unchanged through the captured intro. The observed mode is 0x7C: opaque FG_WORLD, sprites, transparent BG_WORLD, fixed text. No invented crowd animation or rope deflection is added.

Name glyph runs, slot positions, and palettes were recovered directly from ROM helpers 0x98BA, 0x993A, 0x9ACA, 0x99D0, and 0x9A78. Demolition's wordmark uses the 0x900 font-page prefix written by helper 0x99D0. Historical WIP logo rescaling is not used.

## Verification performed

1. `python test_assets.py` passes all three independent artifact tests. Every emitted sprite cell hash matches WP25's separate tile manifest, all pose piece coordinates/flip flags match its expanded CSV, all 21 presenter poses exist, native CRAM channel ordering is checked, and every static text frame uses one valid native texture.
2. `cc -Wall -Wextra -Werror -c generated/wp02_assets.c` passes.
3. Independent reconstructed scene with the original actor sprite snapshot plus newly generated ROM name helper output matches MAME `full/sn0030.png` exactly: 0 of 76,800 differing RGB pixels. See `generated/pixel_comparison.json`.
4. `dump_native_scene.c` compiles and runs the actual new `scene/src/wp02_scene.c`. `verify_native_composition.py` then consumes those native pose records, the native atlas, and the root renderer's painter order/anchor/mirror equations. All **236 of 236** available MAME frame PNGs match exactly, including every captured terminal exit tick. The MAME image pipeline displays native state index `PNG index - 1` uniformly. See `generated/native_composition_audit.json`.
5. All 1,792 canonical rosters allowed by the root selection policy were checked for initial scene command count. After exact wordmark composition, maximum is **186 of 1,024** commands. A conservative bound with every actor using its largest resident pose simultaneously while all names remain visible is **220 commands**. The full 1,570-update reference cast peaks at **195 commands**, down from 562. Its initial name phase falls from 562 to 186 commands. See `generated/capacity_audit.json` and `generated/command_timeline.json`.
6. `verify_rom_names.py` executes original 68000 helper 0x98BA in Unicorn and normalizes its actual byte-smeared text writes. Five rosters covering all ten selectable identities match the native text assets with zero differing pixels, including Demolition in each team position. The test uses the same duplicate-wordmark suppression as the root renderer. See `generated/rom_name_audit.json`.
7. `generated/reference_frame_hashes.json` maps every native update index 0–1569, including the terminal presenter clear, to SHA-256 of the complete 320×240 RGB888 image. The fixed reference uses seed `0xCEB7C58E` and identities `[0,6,9,1]`. It contains 1,570 entries and 90 distinct images; repeated hashes reflect original stationary animation holds. It uses the native C update indices, with the established MAME screenshot latency documented explicitly.

The renderer order in the reviewed root main is correct for the closed fixed prematch coordinates: referee, presenter, wrestler slots 1,3,0,2, transparent foreground, credit/name/VS text. Original actor rendering and name visibility are supplied by the independently translated native state machine.

## Required integration detail

For a canonical Demolition team `[10,11]`, both per-person name frames are the same full-width team wordmark. Draw that wordmark once per team; suppress the second identical emission. The exact text composition is static throughout its original visibility interval, so its precomposition changes only native command count and transparent texel work.

Before precomposition, all 342 glyphs in the reference roster's names were nontransparent and onscreen. Only two VS cells could be eliminated by blank-cell culling, which would reduce 562 commands to 560. Composing the exact wordmarks reduces the name-phase count by 376 commands for 49,280 additional resident texture bytes. Original source glyphs and the captured text reference remain available for independent validation; unused former per-name command lists are removed from emitted C metadata.

## Actual Mednafen capture verification

`verify_mednafen_capture.py` independently decoded the finalized `verification/reference-v4.mov` (3,288 CSCD frames, 704×480). It cropped X=32, Y=0, width=640, height=480 and reduced exactly 2:1 with nearest-neighbor sampling. Every scene frame was compared using SHA-256 of all 320×240 RGB888 pixels, with no color or geometry tolerance. The known-corrupt v3 capture was not read.

The complete scene begins at video frame 1207, timestamp 20.175079 s. Terminal presenter clear first appears at frame 2842, timestamp 47.504286 s. All **1,636 displayed frames** in that interval are exact native reference images. The capture contains all **90 distinct images** and all **169 consecutive visual runs**, in the exact native sequence, with no omitted occurrence, reversed progression, extra run, or unknown image. Comparing the complete run sequence prevents repeated animation images from hiding an omitted state.

| Interval | Original update intervals | Expected duration | Captured duration | Difference |
|---|---:|---:|---:|---:|
| Complete scene through terminal clear | 1,569 | 27.313152 s | 27.329207 s | +0.016055 s |
| First visible exit movement through clear | 190 | 3.307520 s | 3.309592 s | +0.002072 s |

The original update rate is 7,000,000 / (448×272) = **57.444852941 Hz**. A linear fit of all observed visual transition times yields **57.444906181 Hz**. The entire transition-phase spread is 0.016399 s, smaller than one captured refresh interval. Every individual visual hold duration and both intervals above pass a tolerance of **0.016757351 s**, defined in advance as one observed output refresh interval (0.016712 s) plus two 1/44,100-second container timestamp quanta. The largest absolute visual-hold duration error is 0.015343 s.

The exit state begins at native update 1378 with an unchanged image; observable movement begins at update 1379. Exit duration is measured from that observable boundary. The final hash is native update 1569, when the presenter active word clears.

Detailed evidence is in `generated/reference_v4_capture_audit.json`; all decoded frame hashes and timestamps are in `generated/reference_v4_decoded_hashes.json`. The focused sequence/timing verifier passes four tests covering ordinary refresh repeats, omitted repeated states, reversed known states, and natural refresh quantization. These findings apply to the captured reference build, seed `0xCEB7C58E`, identities `[0,6,9,1]`. Sound and physical hardware validation are outside this pixel/timing audit.
