# Native CPU deadline diagnosis and exact ring optimization

This directory is a frozen **v3** copy. Only this copy was built and instrumented. The parent's later v4 CPU, input route and atlas are separate; their normal, uninstrumented capture is the final cadence result.

## Measured cause

The diagnostic records native SH-2 FRT ticks for game frames 7400–7799, plus video-clock boundaries, the VBlank IRQ phase, VDP1/VDP2 sync entry/exit and hardware command workload. FRT uses DIV128; milliseconds are calibrated from observed native FRT ticks per video field. No emulator clock, frame skip, gameplay or synchronization wait is changed.

In the baseline, background animation ticks occur at frame % 10 == 1. Their world/ring CPU burst causes 16 of 17 extra VBlanks in the observed 400-step window. One other miss comes from a 109-tile sprite replacement at frame 7582.

When work crosses VBlankIn, vdp2_sync queues after that interrupt; wait1 returns at VBlankOut, then wait2 waits approximately 15.28 ms for the following VBlankIn. The next game frame starts late and records the extra VBlank. When work already crosses VBlankOut, the extra VBlank appears in that same row. The IRQ-phase bitsets and relative timestamps distinguish these cases.

| Diagnostic, same 400 native steps | Baseline | Ring proposal + stable FRT sampling |
| --- | ---: | ---: |
| World CPU per animation tick, mean |4.986 ms|1.923 ms|
| Ring CPU per animation tick, mean |2.207 ms|1.719 ms|
| Combined CPU per animation tick |7.194 ms|3.642 ms|
| Extra VBlanks in 400 steps |17|5|
| Total VBlanks in 400 steps |417|405|
| Peak CPU work excluding sync waits |20.048 ms|16.776 ms|
| Sync requests observed during VBlank |8|3|

The measured saving is **3.551 ms per animation tick, 49.4%**. Fine profiling adds work; these numbers establish the cause and improvement, not the final demo's cadence.

### Baseline timer caveat

The initial profiler used Yaul's two-byte FRT read. Two short wait2 samples (frames 7518 and 7688) spuriously include 256 ticks, approximately 1.219 ms; other aggregate stage values can have corresponding small sampling error. The candidate profiler reads high-low-high and retries on high-byte change. VBlank counts and the raw native game frame counts are independent of these FRT sample anomalies. Both original RAM dumps and all original rows are preserved.

## Production proposal1: ring lookup

`proposal/wf_r18_ring.c` and `proposal/ring-immutable-lookup.patch` contain only:

1. A 16 KiB HWRAM lookup table from source IDs 0..8191 to the bound pack's tile index. A valid bind rebuilds it; an invalid bind leaves g=NULL and prevents use. Missing IDs retain failure behavior. IDs 8192..65535 use the original sorted-table search. Duplicate or unsorted pack IDs remain rejected.
2. Checking a valid overlay before reading the base map. Previously those LWRAM reads were immediately overwritten by the overlay values.

The pack remains borrowed and immutable during its binding lifetime. Rebinding another pack rebuilds the table. No command, tile, palette, flip, cell, wait or update is omitted. The extra 16 KiB must be checked against the final normal build's map; the parent's measured v4 layout has sufficient room. The diagnostic's profile rows themselves consume 48 KiB and are absent from production.

### Equivalence evidence

`ring-test/test-result.log`: ASAN/UBSAN pass (LSan disabled because of the environment). It covers 262144 full-domain source-ID probes, 11 binds including invalid duplicates/order/size and source A → source B → source A, 1752 map/bitmap comparisons, 420 views and 1,881,600 independent scalar raster pixels. Scrolling, wrap, flips, dirty writes/restores, missing IDs, upper 16-bit IDs and invalid layer/cell inputs are exercised.

The actual native captures also match all 4000 per-frame workload fields (400 frames × 10 fields): tile/command/record counts, decodes, changed uploads, DMA bytes/runs, narrow commands and pixel area. The final 320×240 native RGB raster is exactly equal:

`d0e225107a818abf082e80b4c57fa91030430dc02aa3e5af7b927b39013b93d5`

## Production proposal2: stable audio boot timer read

`proposal/wf_r18_scsp.c` and `proposal/scsp-stable-frt-read.patch` replace the SDK's high-then-low sample with high-low-high/retry. Both boot initialization and subsequent now() calls use the stable sample. Timer divisor, conversion constant, deadlines, sound behavior and runtime VBlank clock remain unchanged.

The first optimized diagnostic build encountered an audio boot timeout before ring_bind ran: OWNER 0x2052 / DEP 0x68000 at VBlank 135. Its boot_us read 2,970,819 us from starting VBlank 13, approximately three full 16-bit FRT periods above elapsed video time; last_frt=0x8500. A torn byte rollover can produce a backward count that unsigned16-bit delta interprets as an extra ≈312 ms and can prematurely expire the 100 ms command deadline. The failed ELF/ISO/CUE/RAM/report are preserved in `capture-candidate-v3`.

`ring-test/test_frt_rollover.py` extracts the exact production helper body and mechanically substitutes only MMIO reads with a deterministic counter model. An independent interval oracle exhausts every 16-bit start value under nine inter-read advance schedules, including full 16-bit wrap: **589824 cases, 2304 invalid old samples, 0 invalid stable samples**. UBSAN passes. The helper hash and exact command are in `ring-test/frt-rollover-result.json`.

The corrected native capture boots SCSP, uploads SFX, starts MUSIC at VBlank 188 and executes to game frame 7800 without fatal errors. It produces 6,321,000 samples per channel for 8600 emulated fields at 44.1 kHz. All 266 measured 30-field DSP intervals after field 600 equal 22050 samples, exactly 735 samples per emulated field. MUSIC remains RUNNING with error 0. See `STABLE-FRT-NATIVE-VERIFICATION.json`.

## VDP1 timing limitation of the frozen emulator

This core is Yabause HLE/interpreter/software renderer, SHA256:

`176c4611333b458c6aa04cb877d69795070f05480bd5c082884c4259cacb6297`

Source `tooling/yabause-scsp-dsp/yabause/src/vdp1.c`, lines 614–638, executes Vdp1Draw synchronously, then sets CEF and sends DrawEnd immediately. `vdp2.c` lines 445/450 invokes this in AUTO mode. This path has no modeled VDP1 raster-cycle budget. Pixel cropping reduces actual command pixel area and host rendering work; this emulator cannot establish physical VDP1 timing or credit that reduction as a Saturn fill-rate gain.

Yaul AUTO's vdp1_sync_wait waits a software flag cleared at VBlankOut (`vdp_sync.c` lines 314–326 and 826–855). Thus elapsed wait1 is not a physical GPU busy-time measurement. `CORE-TIMING-LIMIT.json` records exact source hashes and line locations. The core and SDK are unchanged by this task.

## Evidence and reproduction

- `SNAPSHOT-SHA256.json`: original frozen v3 source/assets identity.
- `CANDIDATE-SHA256.json`: instrumented candidate identity.
- `proposal/SHA256.json`: two production files, frozen.
- `capture-fine-v3`: original 400-row diagnostic, RAM/report, original timings.
- `baseline-fine`: original diagnostic ELF and changed-source baselines.
- `capture-candidate-v3`: preserved audio failure before stable timer read.
- `capture-candidate-safe-frt-v3`: completed stable profile, ELF/map, native RAM/audio/report and exact capture command.
- `PROFILE-COMPARISON.json`, `profile-comparison.png`: measured comparison and native raster identity.
- `analyze_fine.py`, `compare_profiles.py`: reproducible extraction and comparison.

Build only this isolated copy:

```bash
source /workspace/scratch/1e822aeaeac7/wp1979/tooling/env.sh
cd /workspace/scratch/1e822aeaeac7/wp1979/perf_probe/src/runtime
make -f Makefile.wp1979 -j4 R18_ENTRY=tests/r18/wp1979_profile_entry.c R18_PROFILE_START=7400 R18_PROFILE_STOP=7800
```

Capture commands, including fixed core path, source cue, RAM watches and 8600 real retro_run calls, are saved as JSON argument vectors in each capture directory. The native profile stops simulation after completing game frame 7800, allowing the 400 rows to be read from RAM; this is a diagnostic artifact, not the final demo entry.
