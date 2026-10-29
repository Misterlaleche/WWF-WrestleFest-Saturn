# Native audio boot diagnosis and emulator corrections

The preserved native disc `../../measure/audio-first/wrestlefest-wp1979.cue`
boots and runs both the 68000 sound firmware and the slave SH-2 music module
under the corrected Yabause core. The game binaries, firmware readiness checks,
input logic and emulated CPU clocks were not changed for this comparison.

The usable core is:

`../yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so`

SHA-256: `176c4611333b458c6aa04cb877d69795070f05480bd5c082884c4259cacb6297`.

This is a **modified Yabause libretro core with the open-source HLE BIOS**.
It is not an unmodified emulator or a physical Saturn hardware test. Three
source lines differ from the retained upstream source: select the existing
SCSP engine with DSP support; correct its SHIFT3 branch; correct the argument
list of the SH-2 interpreter's VRAM instruction-fetch function. Frameskip is
disabled, and no CPU overclock is configured.

## Cause and controlled comparisons

All native-disc comparisons used ISO SHA-256
`b819634041ed94b79d9c756f15a6eb697e4ae112096eae7dace2653d3c4b5044`.

| Stage | Observation | Evidence directory |
| --- | --- | --- |
| Original core, legacy SCSP | DSP program never executes. Firmware reports `B014`: the DSP clock did not start. Host fails at owner `0x2052`, dependency `0x68000`. | `probe` |
| Upstream DSP engine selected | Same zero DSP clock: the interpreter repeats the `shift == 2` condition and has no SHIFT3 branch. | `dsp-enabled` |
| SHIFT3 corrected | Firmware becomes READY. SFX_STOP acknowledges and SFX.DAT uploads. Master then enters the SDK exception handler while calling MUSIC.prepare in VDP1 RAM. | `shift3-full`, `vram-fetch-before` |
| VRAM fetch signature corrected | Music prepare and slave run complete. Native gameplay and nonzero native audio continue for 1,200 emulated frames without a game fatal state. | `vram-fetch-corrected` |

The 68000 executable's loaded file segments matched SCSP.ELF byte for byte.
The entire 128,060-byte music executable segment in VDP1 RAM matched MUSIC.ELF
before the VRAM fetch fix. Asset loading was therefore excluded as the cause.

## Exact corrections and authority

The complete review diff is `../yabause-scsp-dsp/WP1979-ALL-FIXES.patch`.

1. `libretro.c`: select `yinit.use_new_scsp = 1` instead of `0`. The existing
   upstream legacy SCSP path does not execute DSP programs.
2. `scspdsp.c`: change the last duplicated `shift == 2` condition to
   `shift == 3`, retaining `dsp->acc & 0xffffff`. Sega's original
   [SCSP/DSP Assembler User's Manual, Store options](https://docs.exodusemulator.com/Archives/SSDDV25/segahtml/sndt/tool/dasm/hon/p04_10.htm)
   specifies S3 as multiply by one, without overflow protection. The retained
   upstream `scsp_dsp_jit.cpp` implements the same no-shift, no-saturation mode.
3. `sh2int.c`: change `FetchVram(u32 addr)` to
   `FetchVram(UNUSED SH2_struct *sh, u32 addr)`. The upstream `fetchfunc` typedef
   in `sh2int.h` and all dispatch calls pass `(SH2_struct *, u32)`. The old
   function interpreted the context pointer as the instruction address.
   The actual native module had entered SDK `__exception_assert` / `abort`
   despite byte-exact VRAM contents.

## Verification

`test_dsp_shifts.c` compiles the actual original and corrected interpreter
source, tests all four shifter modes with eleven accumulator values each,
and verifies saturation and wrap boundaries. The original has ten failures,
all in SHIFT3; the corrected interpreter passes all 44 cases. Results:
`dsp-shift-regression.json`.

In the 1,200-frame native-disc test, all 1,170 measured intervals from frame30
onward advanced the DSP clock by exactly735, including 16-bit counter wrap.
At the core's reported60Hz video rate, this is44,100 DSP samples per second.
The WAV contains882,000 stereo samples per channel over20 emulated seconds;
1,047,834 individual channel samples are nonzero, peak amplitude1717 and RMS
224.09. These are unprocessed emulator audio callbacks, not a substituted track.

At the end of this test, MUSIC state is RUNNING2, error0, events587,
generation2 and last command4. The 68000 mailbox remains READY2. Native game
fatal_owner is0. Native drawing counters read714 draws over720 VBlanks,
maximum gap2, giving59.5 draws per emulated second over that initial game
interval, including its start boundary. Longer cadence measurements are
separate from this boot regression.

`unchanged-sh2-timing.json` records identical source hashes for `yabause.c`,
`yabause.h`, `sh2core.c`, `sh2core.h` and `sh2int.h`. The sole SH-2 interpreter
change is the fetch argument list; no instruction-cycle accounting, scheduler,
clock divisor or video-rate logic changed.

## Reproduce the corrected core

Original source archive: `../yabause-libretro.tar.gz`, downloaded from
`https://codeload.github.com/libretro/yabause/tar.gz/refs/heads/master`.
Archive SHA-256:
`7f1bff1d902b8c0574c650463ba872d86029eb7c513b3e39ee03bf3b991795a4`.

Extract that archive into an independent destination, then run:

```sh
python3 apply_core_fixes.py /path/to/extracted/yabause-master
make -j4 -C /path/to/extracted/yabause-master/yabause/src/libretro ENABLE_CHD=0 HAVE_THREADS=0
```

`apply_core_fixes.py` validates all three source patterns before writing and
preserves line endings. It refuses unexpected source instead of applying a
fuzzy change. A build in another directory or toolchain can produce a different
binary hash; the retained binary hash above identifies the exact tested core.

The original core remains untouched at
`../yabause-master/yabause/src/libretro/yabause_libretro.so`, SHA-256
`9128e3e1ca5b26ced9bf7c6879424d38a138c6d55c654bd61e651a414c72230a`.
Intermediate configuration-only and SHIFT3-only binaries are preserved next
to the final alternate core. The adjacent `.configuration.json` is embedded
by the capture runner into every new report.

## Capture the same native disc

From the workspace root:

```sh
python3 wp1979/tooling/capture_yabause.py \
  --core wp1979/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so \
  --cue wp1979/measure/audio-first/wrestlefest-wp1979.cue \
  --out wp1979/tooling/audio-integration/reproduction \
  --frames 1200 --probe-audio --dump-memory \
  --watch fatal_owner:0x060d8aa0 \
  --watch video_clock:0x060d8aa8 \
  --watch wp1979_draws:0x060c81bc \
  --watch wp1979_vblanks:0x060c81b8 \
  --watch game_frame:0x060d6cd4
```

These addresses belong to the preserved audio-first ELF. Extract symbols again
for a newly linked game. `--probe-audio` reads sound RAM, VDP1 RAM and CPU
registers through existing core ELF symbols; it writes no emulated memory.
The video canvas remains320×240, with black padding for the initial224-line
frames and no cropping or resizing when gameplay changes to240 lines.

## Longer native audio cadence measurement

`cadence6000/report.json` observes6,000 emulated video frames of the preserved
audio-first build. With the first30 game frames excluded, the native counters
advance5,424 steps and5,424 draws over5,488 VBlanks:59.30029 FPS. The minimum
rolling five-second rate is58.0 FPS, ten-second minimum58.2 FPS, and one-second
minimum56.0 FPS. Draw gaps comprise5,360 single-VBlank gaps and64 double-VBlank
gaps. MUSIC stays RUNNING with zero errors throughout gameplay, ending with
6,831 events. All5,970 measured DSP-clock intervals advance exactly735 ticks.

This is the older audio-first fixture; it is not a measurement of the later
three-tag fixture or subsequent renderer changes. CD transfer counters were
not observed in this capture. No zero-CD-I/O assertion is made.

## Capture accounting correction

The runner now increments an explicit `executed` counter only after
`retro_run()` returns and uses it for reported frame count and emulated
duration. `capture-count-regression.json` verifies a timeout before the first
run reports0 frames/0 seconds, and a complete three-run capture reports3
frames/0.05 seconds/2,205 audio samples per channel. Complete prior captures
are unaffected by this accounting correction.
