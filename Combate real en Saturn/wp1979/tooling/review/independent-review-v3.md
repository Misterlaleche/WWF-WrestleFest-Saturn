# WP1979 independent review — final candidate 4

## Status and scope

**Final review complete. Candidate 4 is ready to deliver as the measured native TAG fixture, with a genuine emulator recording, native audio, ordinary-controller takeover evidence and a structurally verified raw BIN/CUE. The previous measured minimum-speed issue is closed for the two reviewed routes. Physical Saturn timing and compatibility remain untested.**

This is a read-only source, provenance and saved-execution review. No native source, pinned reference, repository, Saturn build or Saturn capture was changed or run by this reviewer. Existing host regression executables were rerun. Only the review reports and adjacent evidence were written. The observations below were independently calculated from the final saved samples, rather than accepted solely from the producer's summary.

This report supersedes the performance acceptance and final-artifact sections of `independent-review-v2.md`. The earlier review remains the detailed record for CPU/human ROM corrections, physical direction mapping, CD transport, renderer micro-optimizations, half-width sprite packing, build-stamp fixes and the three disclosed emulator source changes. The reference is the byte-verified export of commit `d95b8c37f62402bce4dead13cf7c0567b8fc2439`.

Primary final evidence is in `measure/final-full`, `measure/final-controller`, `release-candidate-4` and `tooling/review/independent-review-v3-evidence.json`. Original captures were closed in `/tmp/wp1979-captures` before copying. I compared every source file with its destination: **96 full-capture files and 61 controller-capture files, with zero missing or different files**. Destination-only files are the copy-verification records and a controller analysis from frame zero.

## Strengths

### 1. The final performance changes are narrow and verified

The integrated ring and SCSP files equal their reviewed proposals byte for byte. Both proposal baselines equal the pinned reference files. The actual reference-to-native differences contain the described lookup/overlay changes and the FRT helper with two call replacements; no profiling or altered gameplay timing is hidden in those differences.

| Reviewed source or asset | SHA-256 |
|---|---|
| `native/src/runtime/src/wf_r18_ring.c` | `4bc36b61448d3f0f2e557183e7b878118cccdfbbaf7ccb5d9ae7a764c114d9e5` |
| `native/src/runtime/src/wf_r18_scsp.c` | `6dae944a9b0306aa29d6b8e19dfced0bac60ab9e80a7b9c562cb5bdb9f11f023` |
| Final 6,474-tile atlas | `ba8a5be1869cb6a8e101eaf06bff8506bb51f7d5258201b5290b9aea1ca377d9` |

Other previously hashed native files retain their candidate-2 hashes, including the corrected CPU/human owners, entry, input adapter, tape, crop backend and micro-optimizations. The only later header change is a verified comment correction. The inspected candidate-4 configuration selects the normal entry and native audio, with no PROFILE or DIAG_RATE defines.

### 2. The immutable lookup and overlay order preserve tested behavior

`native/src/runtime/src/wf_r18_ring.c:15` adds 8,192 uint16 entries, exactly 16,384 bytes of static HWRAM. Binding validates the pack and its strictly sorted IDs before building this table. The maximum accepted tile count is 1,024, so the absent sentinel `0xffff` cannot equal a valid index. Failed binds leave `g` null; valid rebinds clear and reconstruct the table. `source_tile_index` at line 307 retains the original sorted-table fallback for public uint16 IDs at or above 8,192.

`surface_pixel` at line 127 checks a valid overlay before reading an unused base cell. Both branches produce the same cached attributes and tile index. Dirty tracking, coordinates, flips, palettes, hardware destinations and pixel interpretation are unchanged.

I reran the existing ASan/UBSan differential executable with the actual staged `R18RING.BIN`: **11 binds, 262,144 source-ID probes, 1,752 map/bitmap comparisons, 420 views and 1,881,600 independent raster pixels pass**. Inspection of the harness confirms coverage of the entire 16-bit ID domain, high IDs, valid rebinds, invalid/duplicate/unsorted IDs, overlay writes/restores, wrapped and negative coordinates, flips, palettes, dirty updates and repeated views. The proposed source included by the test equals the integrated native source.

### 3. The stable FRT sample fixes the demonstrated torn-read failure

`native/src/runtime/src/wf_r18_scsp.c:54` reads high, low and high again, retrying when the high byte changes. The SDK's `cpu_frt_count_get` at `tooling/sh2eb-elf/sh2eb-elf/include/yaul/cpu/frt.h:94` reads high and low separately without this consistency check. The helper reads volatile registers and writes no timer register. It replaces only the boot-time samples at native lines 68 and 99. Clock divisors, elapsed-time arithmetic, firmware deadlines and the gameplay VBlank clock path are unchanged.

The exact helper's SHA-256 `aedd610d2e83c80677acaa6b1b6cb1c24f88ed22d113b8233d72d56d2879372d` matches the tested extracted body. I reran the existing UBSan executable: **589,824 modeled schedules pass, with 2,304 invalid old samples and zero invalid stable samples**. The specific rollover example replaces the impossible old `0x1200` result with `0x1302`, a value that occurred during the read interval. This closes the reproduced byte-tearing defect; the finite model does not establish every physical timer/interrupt behavior.

### 4. The atlas extension preserves art and fits the current layout

I independently decoded all **6,474** atlas tiles against canonical `SPRITES.PAK`; every byte matches. The sorted IDs equal the union of the 6,357-tile main atlas and the reconstructed manual route, with exactly 117 additions. Native `HOTSPR.BIN` is byte-identical to that atlas. Saved host actor/role traces are unchanged by the residency extension.

The map occupies 51,792 bytes, padded to 51,808, within the unchanged 114,688-byte HWRAM index allocation. The dictionary occupies 643,424 bytes, rounded to 643,456 resident LWRAM bytes within 650,240; **53 bounded cold slots remain**. The two final native captures add zero combat CD windows and zero sectors. These are verified routes, not a claim that every possible manual sequence is resident.

I reran both candidate-4 static memory verifiers. BSS is `0x06067120..0x060e85c0`; the slave stack top is `0x060ff000`, a static BSS-to-top distance of **92,736 bytes**. Linked end `0x060e8694` leaves **96,620 bytes** to HWRAM end. Master stack top remains `0x06004000`. Both checks pass. These figures describe placement, not measured maximum stack headroom.

### 5. Logical triggers preserve ordinary controller ownership

The final `tooling/capture_yabause.py` equals `render_opt/takeover/logical_schedule/capture_yabause-proposed.py`, SHA-256 `da52d16d1dfb32d3aa820772f00dcda95fd2419304e57086ed98ff80399a220b`.

At lines 279–296 the utility observes the state left by the previous `retro_run`, waits for the native started flag, and changes only its local pad-button set when a scheduled threshold is first eligible. `input_state` at line 156 supplies that set through the ordinary libretro controller callback. `read_memory` at line 180 reads bytes; no emulated RAM, CPU register, replay counter or actor state is written. Logical schedules require unique four-byte counter/started watches on every frame. Frame-only schedules retain their old stable sorting.

I independently reconstructed all **15 applied events** from the preceding samples. Every event occurred at its first eligible observation and equals the captured log. Thresholds derive from the prior actual controller route, not invented positions. This allows different boot duration and CPU cadence while leaving native SMPC callback latency intact.

I reran the proposed read-only analyzer and obtained exactly the saved passing result. I also separately compared actors 0, 1, 4 and 5 at every baseline observed game counter 0..512: **2,052 records and 361,152 bytes are identical**, with duplicate observations required to agree. This verifies the captured native outcome; the scheduler itself does not force that outcome.

First physical LEFT callback occurs at emulator frame 544; permanent demo cancellation is observed at frame 545, game counter 66. The route contains 13 exact LEFT walking steps and 6 exact UP walking steps, with 182 neutral samples after the final release. RIGHT/DOWN and A/B/C decode correctly in this capture, but their combat-state intervals do not independently prove exact walking displacement. The prior four-direction ROM/physics regression remains the evidence for those mappings.

## Final measured execution

The following values were recalculated directly from every captured sample. Each adjacent running sample advances the observed video clock by one; completed native counters advance by zero or one, and draw counts equal native counters. Both fatal-owner and fatal-dependency fields remain zero. All rolling windows are complete windows before the native terminal event; post-victory frozen frames are excluded from combat rates.

| Route and interval | Native steps / VBlanks | Native FPS | Minimum 1 s | Minimum 5 s | Minimum 10 s |
|---|---:|---:|---:|---:|---:|
| Full match, from game counter 0 | 7,372 / 7,376 | **59.967462** | **58.0** | **59.6** | **59.8** |
| Full match, after first 30 steps | 7,342 / 7,344 | **59.983660** | **59.0** | **59.8** | **59.9** |
| Controller route, from game counter 0 | 560 / 562 | **59.786477** | **58.0** | **59.6** | Not long enough |
| Controller route, after first 30 steps | 530 / 530 | **60.000000** | **60.0** | **60.0** | Not long enough |

The project uses the stated MAME arcade timing reference of 57.444852941 Hz, not a measured original PCB clock. The tested one-, five- and ten-second minima exceed that reference. This is evidence for those complete observed windows, not a guarantee that every single native tick finishes within one VBlank: the full match has four two-VBlank draw gaps, including two at startup. The maximum recorded gap is two.

The native fixture starts at emulator frame 477. Actor IDs remain 0, 1, 9 and 7, with legal roles changing from `(0,4)` to `(1,4)` and then `(1,5)`. The exchanges are observed at completed counters 528 and 5,767, corresponding to the audited completed host transactions 529 and 5,768. In the final video they appear at **16.783333 s** and **104.116667 s**. The original native endpoint is **7,372**, result `0x4001`, count `0x0300`, timer `0x0015`, followed by the completed **131-VBlank** result presentation. No physical controller events are supplied to the full demonstration; its disclosed native input tape drives the real input owner.

Both routes have **zero additional combat CD windows/sectors**. The measured full video contains 8,400 encoded frames at 60 Hz on a 320×240 canvas, exactly 140 seconds. The controller video contains 1,040 frames, 17.333333 seconds. FFprobe confirms frame counts, rates, dimensions and matching audio durations. Capture source writes one image per emulated run; smaller boot frames receive black padding without scaling or cropping. Emulator wall time is reported separately and does not set playback speed.

I inspected the actual full-capture stills at frames 1,080 and 6,360 and the final image. They show the post-tag participants and the congratulations/next-match presentation. No new concrete visual defect was identified in those stills; this is not an every-frame pixel equivalence claim.

### Native audio integrity

| Capture | Stereo PCM samples per channel | Duration | Peak PCM16 | Checked DSP intervals |
|---|---:|---:|---:|---:|
| Full | 6,174,000 | 140 s | 2,645 | 7,922 |
| Controller | 764,400 | 17.333333 s | 1,564 | 562 |

I read the closed WAV headers and their complete payloads independently. Header count, actual readable bytes, callback report and `735 × emulator_frames` agree exactly. Full WAV SHA-256 is `6d0ada280063b265d327ea08ea20b1afd02affcf0b5ae1af3c91e64f3339285a`; controller WAV SHA-256 is `e3968bb0054c107ebf240c1ae1cca774458faac772e79a3f84e7b923511d8f1c`. Both have nonzero PCM, matching independently calculated peak/RMS statistics. Every checked gameplay DSP interval advances exactly 735 samples, including counter wrap; MUSIC stays at state 2/error 0 and the SCSP mailbox stays in state 2. Audio comes from actual callbacks, without a replacement soundtrack or amplitude processing.

The current core hash is independently rechecked as `176c4611333b458c6aa04cb877d69795070f05480bd5c082884c4259cacb6297`. The earlier review bounds its three disclosed source changes: existing newer SCSP backend selection, generic SHIFT3 correction and FetchVram ABI correction. No overclock, frameskip or changed SH-2 scheduling was introduced. Its HLE BIOS and physical-renderer timing limitations remain material.

## Exact final identity

Both captures and `release-candidate-4` use this raw disc:

| Artifact | SHA-256 |
|---|---|
| Raw MODE1/2352 BIN | `f1232edb31d6e3748b60001d8d82bca5f9b6171d16aaf812f618f3dbf858da5e` |
| CUE | `e3a329a9b0e57b1b22440c4165e302fcdf64b5409a98149579f7fbb299ba0fdb` |
| Frozen ELF | `cf58d68ea37af04f6cbfdd382175329f4a5f40daa20b9c949f45f75faebb3967` |
| Frozen map | `08d75ea2499fc2ddf89820f22706b240966445500228872e2008d57873874ee3` |
| Flat program / disc A.BIN | `49b03fc81c6801eb376979079f96746f1bbc887346b29c90d55f1003bad2720e` |
| Full capture MP4 with audio | `c22fe2527b03dd64d36ea3c743226066a971dc48c98da863196ba6c82b2db9c2` |
| Controller capture MP4 with audio | `394109a3c0547e9ac2660446270686b4a57fe6ad2b2f57cda479b9d1ff815758` |

I independently regenerated and compared **all 6,592 raw sectors**, including headers, EDC and ECC. The reconstructed ISO hash is `a2078c9dab8a984b14be647dd8efa1b116d5121caebc157098ebaf6890ddb659`. All **19 CDFS root payloads** equal their staged sources; the boot area equals SDK IP.BIN. The CUE references the actual local single-track filename and correct raw mode. I flattened the frozen ELF through objcopy into a memory-only seekable file and checked exact equality with frozen `program.bin` and disc `A.BIN`. Both captures' five-file identities agree, and release BIN/CUE hashes match them.

## Issues and disposition

### Critical

**None identified** in the reviewed final changes or final recorded paths.

### Important

**I1 — Measured minimum speed: closed for candidate 4's demonstrated routes.** The old candidate-2 minima of 56 FPS in the full match and 53 FPS on the manual route no longer describe the final image. The fresh calculation above reaches minimum 58 FPS from frame zero, and 59/60 FPS after the first 30 native steps. Both final routes have zero combat CD reads. `measure/analyze_capture.py:58` reports complete rolling windows; the independent calculation agrees. No later binary or arbitrary route is implicitly covered by this closure.

**I2 — Final disc/video/audio/controller identity: closed.** The frozen binary, complete match, takeover route, native audio and copied artifacts all satisfy the identity/integrity checks above. The old replaced controller WAV remains historical and is not used as audio evidence.

**I3 — Physical raster timing remains an explicit limitation.** `tooling/yabause-scsp-dsp/yabause/src/vidsoft.c:2455` draws the VDP1 list synchronously; `vdp1.c:631` immediately sets CEF and signals draw-end; `vdp2.c:445` invokes drawing in display processing. This path has no independently competing VDP1 raster-cycle budget. Consequently these genuine emulator cadence measurements cannot prove physical GPU fill rate or shared-bus timing. The source/raster tests establish exact pixels and reduced submitted width, but not the resulting physical speed improvement. Resolution for delivery is explicit scope language; a hardware-speed claim requires a physical test or a separately suitable timing model.

### Minor

**M1 — Cold-slot comment: closed.** `native/src/runtime/include/wf_saturn_memory.h:27` now states that capacity depends on the atlas and the final selection leaves 53 slots. I compared it with the prior/reference header: bytes outside C comments and total line count are identical. This documentation-only correction does not change preprocessing tokens, debug line positions or the captured executable. Current header SHA-256 is `7e9b70a766f2a8e7b454d36c51b46514ed50a67c498cc5b4880db53d02d94d17`.

No other concrete new minor defect was identified in this final pass. Previously resolved build-stamp, physical-direction, state6, static-margin terminology and WAV replacement findings retain their recorded closures.

## Declined to judge, with reasons

- **Physical Saturn operation, retail BIOS boot, drive compatibility, PAL behavior and hardware speed:** no physical-console observation is available. The raw-disc structure and NTSC-path HLE execution are checked, but do not establish those physical properties.
- **VDP1 fill rate and bus contention with DMA or the slave music module in the VRAM tail:** inspected address ranges and software pixels do not replace a hardware timing model; the emulator renders synchronously as described in I3.
- **Every human input sequence, all gameplay states, tournaments, modes and unsupported dependencies:** this is a direct TAG fixture with a disclosed native tape and a bounded ordinary-input route. Targeted ROM tests and two actual routes do not exhaust the game state space.
- **Unconditional zero CD reads or full speed after arbitrary takeover:** residency is based on the union of two traced routes; 53 fallback slots are a bounded cache, not an all-input residency proof.
- **Worst-case master/slave stack depth, unrestricted heap headroom and pathological timer/interrupt timing:** static placement checks and finite timer models pass, but exhaustive stack high-water or physical interrupt analysis was not performed.
- **Complete original ROM equivalence:** earlier oracle tests cover specified branches and saved phases. They do not prove every owner, condition code or address-register side effect in all contexts.
- **Exact original PCB refresh frequency:** 57.444852941 Hz is the declared MAME timing reference, not a physical PCB measurement.
- **Exact arcade audio waveform, timbre, instrument balance, transcription or physical analog output:** actual native audio samples, firmware state and DSP cadence are checked; a calibrated original-audio comparison was not performed.
- **Every full-scene pixel at every tick:** atlas bytes, changed raster paths and selected actual images are checked, but complete original/native video and layer/palette equivalence was not established.
- **Later executables, assets, emulator changes or recaptures:** this assessment pins the final identities above. Reuse of this review does not approve changed deliverables.

## Assessment

**Ready for the scoped delivery.** The final image completes the real native TAG match with both legal exchanges and the original terminal result, preserves the audited controller route, produces intact native audio, and meets the measured arcade-reference cadence in every tested complete one-, five- and ten-second window. Source changes are narrowly verified, asset and static memory bounds pass, and the supplied raw disc is the exact executable captured in both final videos.

The remaining material limitation is physical validation. The deliverable can be presented as a native Saturn program and a raw disc prepared for the user's hardware trial, with measured emulator performance. It must not be presented as a hardware-tested speed or compatibility guarantee.
