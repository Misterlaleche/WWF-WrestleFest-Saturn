# WP02 TAG prematch audio — R02

This independent revision changes the background refill budget and exposes a
frozen first-failure record. The PCM/BGM assets, codec, original sound-event
tables, and M68K driver remain byte-identical to R01. The historical R01
measurements below are retained as provenance; current R02 results are in the
delivery's `audit/audio_r02/` and `audit/reference-r02-final-full.json`.

This module plays the original WrestleFest ring-introduction voices, reactions,
and the music already playing on entry through the existing Saturn
poneSound/M68K/SCSP driver. Voice/reaction samples are decoded from supplied
`31j10.ic73`. Background music is the original ROM-driven YM2151 plus OKI
percussion, isolated from the verified arcade session. No substitute or newly
composed audio is used.

## Integrate

Compile these six sources:

- `wp02_audio.c`
- `wp02_pcm16.c`
- `wf_audio_sfx.c` (unchanged A16 source)
- `ponesound_min.c` (unchanged A16 source)
- `wp02_bgm.c`
- `wp02_bgm_codec.c`

Include their headers, generated `wp02_audio_assets.h`, and
`wp02_pcm16_gain.h`, `wp02_bgm.h`, and `wp02_bgm_codec.h`. Put
`cd/WP02PCM.PAK` and `cd/WP02BGM.PAK` on the ISO. The caller supplies the
unchanged `assets/SDRV.BIN` contents as an array and its byte count. The main
WP02 build reads that file from CD before initialization; an embedded array
also satisfies the API. No driver asset symbol names are hard-coded here.

After choosing the four **raw ROM wrestler IDs**, and before starting scene
timing, call:

```c
bool ok = wp02_audio_init(pcm_pack_fad, sound_driver, sound_driver_bytes, ids);
ok = ok && wp02_audio_bgm_load(bgm_pack_fad, bgm_pack_bytes);
uint64_t scene_origin_us = wp02_audio_time_us();
```

`ids` is a four-byte array, each value in 0..11. Preserve the scene module's
raw IDs; do not translate them through an old UI roster order.

For each sound event produced by the native scene state machine, in order:

```c
wp02_audio_emit((uint8_t)scene.effective_commands[i]);
```

The adapter accepts low bytes `31`, `32`, `6D..75`, `7B..7D`. The ROM scene emits
`3132` on entry. It emits fixed phrase commands `317B`, `317C`, `317D`, `317C`
and identity commands through its original state/branch logic. The scene owns
the `3173` group skip and the actor activation decision; the audio backend
does not duplicate or approximate those branches.

Call `wp02_audio_vblank(NULL)` every display VBlank. Call
`wp02_audio_service()` once per display frame, outside interrupts, and handle
a false result as a playback error. Background starts automatically with
the first entry command `3132`.

R02 samples scene time at the display boundary, submits the VDP1 commands,
and then services audio while VDP1 draws. It checks the latched audio state
before updating the scene. Keep the clock sample ahead of variable decoding
work; moving that sample changes visual deadlines.

When the native scene clears the presenter and becomes done, call
`wp02_audio_scene_done()` once. This stops only the background at the end of
the requested scene; the demo does not enter the subsequent gameplay music
transition. Keep the FRT and audio VBlank calls running as the scene freezes
so natural voice/reaction tails finish. `wp02_audio_stop()` is an immediate
silence/reset for reload or teardown.

## PCM16 data and memory

The 14 assets contain the full decoded 12-bit OKI signal multiplied by 16,
stored as signed big-endian PCM16 at the original 8000 samples/s. No low bits
are discarded by the asset conversion. Each sample has an original ROM byte
range and SHA-256 in `audio_assets.json`.

The pack contains unattenuated source PCM. During preload, the adapter
applies each sample's original OKI command-volume numerator over 32 to the
resident copy. Every supported use of a given sample has the same original
volume nibble; the generated metadata and host test verify that constraint.
Fractional PCM16 output values use nearest-even rounding, with at most half
an output LSB of error. The pack itself is unchanged by this gain operation.

| Original sample | OKI volume nibble | Resident gain |
|---|---:|---:|
| Common 17 | 5 | 6/32 |
| Common 18 | 1 | 22/32 |
| Bank 1, 23 | 2 | 16/32 |
| Bank 1, 24..31 | 3 | 11/32 |
| Bank 1, 37..39 | 2 | 16/32 |

The CD pack is **509,952 bytes** including 2048-byte alignment. Its complete
PCM payload is 488,792 bytes. Only the fixed samples and the four selected
identity samples are loaded into sound RAM; duplicate identity sample IDs
are loaded once.

| Allocation | Bytes / range |
|---|---:|
| Original driver file | 8,732 bytes |
| Preserved driver/system reservation | `0x00000..0x13FFF` |
| Load-time HWRAM staging | 16,384 bytes, static |
| Maximum selected PCM16 allocation, including 32-byte alignment | 361,512 bytes |
| Maximum sound-RAM end, exclusive | `0x6C428` |
| Space above maximum voice/reaction allocation | 80,856 bytes |
| Background PCM16 transport ring | `0x6C800..0x7C7FF` (65,536 bytes) |
| Ring endpoint guard | `0x7C800..0x7C801` (2 bytes) |
| Compressed source in reserved LWRAM | 1,048,576-byte capacity |
| Compressed source overflow in HWRAM | 720,896-byte capacity |
| BGM decoder buffers | 4,108 bytes |

The maximum occurs for raw IDs 1, 2, 7, 9. All 495 combinations of four
distinct IDs from 0..11 were checked. That is a superset of the legal TAG
rosters used by the main scene. No CD reads occur after initialization.
Voice/reaction samples remain fully resident and require no runtime decoding.

The fixed assets are common samples 17/18 and bank-1 samples 37/38/39.
Identity samples are bank-1 samples 23..31. The raw identity sample table is:

```
23 24 25 30 29 29 27 26 28 31 29 29
```

The spoken wording is not transcribed or guessed. Raw sample 29 is shared by
multiple wrestler IDs, so its wording must not be inferred from a selected
name.

## Timing and native driver format

The module owns the master SH-2 FRT at phi/8 with the existing Yaul
`CPU_FRT_NTSC_320_8_COUNT_1MS = 3359` compare interval. The original A16
rational accumulator runs the recovered 3,579,545/(64*253) Hz sound scheduler.
The original command/event/priority/bank/F1 tables are unchanged.

The 16-bit FRC runs continuously after its initial reset. OCRA advances by
3359 ticks for each deadline; interrupt handling does not rewrite FRC.
Both the interrupt and the interrupt-masked clock getter accumulate the
modulo-16-bit elapsed count into a 64-bit epoch. Delayed interrupts catch up
the original scheduler for every elapsed compare interval. The counter
must be observed within its approximately 19.51 ms wrap interval; normal
compare interrupts run about once per millisecond. This avoids the former
software read/subtract/write loss and does not depend on the differing
CCLRA reset phase described by the SH7604 manual and emulated by Mednafen.
The focused review and regression evidence are in `../../review/REVIEW.md`;
the tests shipped here cover those cases.

For scene pacing, `wp02_audio_time_frt_ticks()` returns a monotonic 64-bit
count including the live hardware remainder. The getter never resets the
counter and preserves its epoch across module reloads.

`wp02_audio_time_us()` uses the verified Mednafen NTSC 320-mode ratio:

```
master clock = 1746818182 Hz
SH-2 divisor = 65
FRT divisor = 8
microseconds = FRT ticks * 520000000 / 1746818182
```

The multiplication is split into quotient/remainder terms to avoid long-run
overflow. This assumes NTSC 320-pixel clock mode. It does not measure a real
board's crystal tolerance. The legacy nominal 1 ms SFX scheduler differs
from this clock by about 79 ppm; it was preserved rather than redesigned.

The PCM16 format helper uses the existing shared command descriptors:

- `bitDepth=0`, the pinned driver's native signed PCM16 format;
- `playsize` is the sample count, not byte count;
- centered pan, zero loop start;
- `pitchword=0x69CE`: OCT=-3 and FNS=462, the closest SCSP setting to 8 kHz
  (7999.5849609375 Hz at a nominal 44.1 kHz SCSP output clock);
- `bytes_per_blank=266` in NTSC or 320 in PAL, as required by the driver
  format. The scene timer itself remains NTSC 320 only.

New cues use the driver's supported `PCM_SEMI=-2` mode. This permits an
intentional replacement cue to restart its assigned slot. The adapter still
enforces the original OKI F1 suppression before requesting a new start.
The included 8732-byte driver uses a three-bit direct-send volume, proven by
its byte-identical historical source build. The adapter therefore sends
native volume **7**, with original OKI attenuation already applied to the
resident PCM16. The former A16 mapping to 0..255 volume is not used: that
mapping assumes a different driver volume protocol. No gameplay SH-2 code
writes raw physical-slot KYONEX registers.

## Original background and resident transport

The complete arcade trace establishes `310E` at frame 2834,
49.334618083 seconds from boot. Entry `3132` occurs at frame 3248,
56.550597916 seconds. The original background therefore has already advanced
7.215979833 seconds when the introduction begins. No new music command is
issued during the introduction itself. Older source notes that inspected
only the intro routine missed this persistent source.

The selected source is the original-session MAME 0.264 capture
`verification/mame/audio_music/music.wav` under the WP02 root. It preserves
YM output and physical OKI voice 0 (sample 1 percussion, volume nibble 3),
while suppressing the other OKI starts used for speech and reactions. The
main CPU is held at the final intro frame for the extra source tail; the
sound CPU and chips continue. Both source channels are identical, so one
channel is retained without a mix operation.

The packed body begins at source sample 2,493,882, or 56.5506122449 seconds:
14.329 microseconds after the traced entry sound command. It contains
1,278,900 signed PCM16 samples at 44,100 Hz, exactly 29 seconds. No gain,
resampling, filtering, dither, or content repetition is applied. The source
and decoded-body hashes are recorded in `cd/WP02BGM.json`.

`WP02BGM.PAK` is **1,752,708 bytes**. Independent 1024-sample blocks use
first- or second-difference prediction with Rice-coded signed residuals, or
raw PCM16 when smaller coding is unavailable. Decoding reproduces every
source sample exactly. The full compressed file is loaded into reserved
LWRAM plus at most 704 KiB of HWRAM before timing begins. The loader caches
the source by FAD and byte count across START replays, and reprimes the sound
ring after each driver reload.

The background uses logical poneSound descriptor 3; the driver selects and
owns its physical slot. Native 44.1 kHz playback uses `pitchword=0` and
volume 7. The pinned driver subtracts one from descriptor `playsize` when
writing hardware LEA, so the background descriptor uses 32769 to obtain
LEA 32768 and a 32768-sample forward-loop transport cycle in Mednafen. A
duplicate endpoint guard word supports the sample fetch at LEA. This is a
rolling buffer, not a repeated musical PCM segment.

The SH-2 exclusively owns the slot monitor selector; the pinned M68K driver
does not access it. Once the physical slot is known, the service selects
that slot at `0x25B00408` and waits for the hardware latch before reading it.
CA is a 4096-sample bucket, so the 32768-sample ring has buckets 0..7 and
each half spans four buckets. Only the consumed, inactive half is written.
Normal service calls decode and copy at most one 1024-sample block. R02
permits two blocks when the gap since the previous service is at least
23,220 microseconds. In the last 4,096-sample bucket of either half,
`(ca & 3) == 3`, the budget remains one. This cap avoids the additional
second-block boundary window demonstrated by the duration regression.
The inherited one-block stale-monitor window is documented in
`audit/audio_r02/SERVICE-TIMING-REVIEW.md`; the tests do not establish a
universal physical execution-time bound. A cached
32-bit bit reservoir and constant-parameter decoder kernels avoid SH-2
variable-shift helper calls in the sample loop. A half
lasts about 371.5 ms, leaving time to refill its 16 blocks across successive
display frames. CD I/O and large interrupt-masked decoding are absent from
playback.

The longest enumerated legal scene is 1633 arcade updates, 28.427264 seconds.
The 29-second source covers it without repeating content. Finite final-half
padding is tracked; leaving the source's final half without the scene stop
is an explicit error. A service gap of at least one half duration, a lost
physical slot, invalid data, or unfinished refill also stops the background
and reports failure. Metrics getters expose refill count, failure count,
and maximum measured service time.

`wp02_bgm_failure_snapshot()` returns a 24-byte frozen first-failure record.
It includes the service gap, decoded source position, completed refills,
sampled monitor word, unfinished fill bytes, physical slot target, and
reason. Copy it before `wp02_audio_stop()` resets the audio module. The
normal scene's fatal handler preserves it in RAM and displays four packed
hexadecimal words on screen; the delivery's LEEME explains those words.

## Validation and limits

R02's portable extended suite is:

```sh
python source/verification/r02_audio/run_tests.py
```

Run it from the delivery root. It verifies every original BGM sample at
59.826 Hz, 29.913 Hz, mixed missed-refresh intervals, 45 Hz, and 50 Hz,
plus frozen diagnostics and the focused duration cases. The R01 fixture's
29.913 Hz failure is an expected failing regression. These simulated
audio-service cadences do not claim equivalent physical rendering speed or
PAL scene support. Optional SH-2 compilation uses `YAUL_INSTALL_ROOT`.

Run the isolated host tests with:

```sh
python tests/run_host.py
```

They exercise the real adapter and unchanged sequencer with hardware I/O
replaced at the boundary. They verify sample/bank/volume routing, concurrent
cues, F1 suppression, tail completion after scene freeze, no playback-time CD
access, worst-roster memory fit, native PCM16 descriptor units and ownership,
all sample-volume mappings and signed gain rounding, and exact scene-clock
conversion. Timer regressions cover delayed interrupts, 16-bit wrap, getter
reads before pending IRQ service, zero-valued wrapped OCRA, and reloads.
The codec test compares all 1,278,900 background samples against the original
body. The stream test simulates 1700 NTSC display frames, comparing every
played sample across transport wraps, checking that only inactive halves
are written, and verifying monitor settling, lost-slot/late-service errors,
and cached reloads with no runtime CD reads.

The separate `verify_pcm16.py` audit uses the supplied Test255 C decoder's
internal 12-bit signal before PCM8 rounding, and compares every exported
PCM16 sample to it. The Python conversion's arithmetic PCM8 reduction also
matches all 14 original A16 pack entries byte-for-byte. A16 and Test255 differ
in PCM8 rounding; preserving PCM16 avoids either loss.

The driver, command tables, and sample origin are independently checked.
Host/source checks do not establish real Saturn output quality or timing.
An actual Mednafen 1.29.0 capture of the native WP02 disc was subsequently
compared against the original-ROM MAME capture. In `reference-v4.wav`, all
13 expected cue onsets were detected with normalized waveform correlations
of 0.759..0.955. Their matched native/original level differences were
-1.42..-0.24 dB after the volume correction. No clipping was observed.
These results validate the required cue delivery and correct the former
large relative-volume errors. That capture preceded background integration.
Analysis data and exact whole-capture onset
times are in `../verification/recording_cue_audit_v4.json`.

The isolated original background was independently compared against the
full original arcade recording. At 82.8..83.86 seconds, its zero-lag
correlation is 0.996563 and matched level differs by only 0.04338 dB. Main
commands/timestamps and the order of all 106,978 non-timer-control YM writes
match; small sound-CPU timing differences from suppression are documented.
See `../background_audit/isolated_capture_audit.json`.

The final native-disc `reference-v7.wav` validates the background
transport in Mednafen. It traverses 36 complete ring cycles with **zero sample
lag or accumulated drift** in all 54 half-second comparison windows. The
clean final music interval has normalized correlation **0.9999999992**, a
uniform native/source gain of **0.83716 (-1.544 dB)**, and residual RMS
**0.00000485** after accounting for that gain. The asset is not normalized to
hide this output-path difference. Scene channels are identical and no
clipping occurs. Music starts at 32.143401 seconds of the whole capture;
silence begins at 59.511224 seconds, following the scene-exit stop and the
driver's command latency.

With background removed only in the analysis, all **13 expected cue onsets**
are detected with correlations **0.796..0.987**. Their matched native/arcade
level differences are **-0.801..-0.170 dB**. No required cue is dropped or
substituted. Runtime metrics record **1166 refill blocks, zero stream
failures, and a 4812 microsecond maximum service call**. An earlier decoder
version missed its refill deadline; the optimized version is the one covered
by these passing measurements. Detailed results are
`../verification/background_output_v7.json` and
`../verification/recording_cue_audit_v7_foreground.json`; the native runtime
metrics and launch command are in `../../verification/reference-v7.json`.

The native driver's VBlank command latency, SCSP pitch/attenuation granularity,
analog output, and physical oscillator tolerance remain transport/hardware
properties. Real-hardware listening has not been performed by this audit.

## Primary evidence

- Supplied ROM: `01-wwfwfest.zip`, member `31j10.ic73`.
- Supplied A16 authority: `02-WWF-Saturn-Audio-Complete-Integration-1-.zip`,
  `audio-integration-files/` and `audio-docs/AUDPCM-INDEX-A15.csv`.
- Supplied C decoder: `13-WWF-Saturn-Test255-WIP160-source-2-.zip`,
  `wf_oki_decode.c`, inspected before its final PCM8 quantization.
- Supplied presentation audit: WIP011..014; the fresh ROM scene audit
  supersedes older prose where actor/group branch descriptions differed.
- The final audio handoff and WP24 reserve existing poneSound/M68K ownership.
- Yaul's upstream `scsp-ponesound-pcm8/ponesound.h` defines PCM16 type and
  playsize units. The included driver matches Git blob
  `e62e4ade1691f766af9d04b58d72b723079694d1` exactly.
- Upstream poneSound `jo_demo/pcmsys.c`, `load_16bit_pcm`, supplies the
  corresponding 16-bit byte-rate convention. Historical commit
  `71dac9d944d1e4156cb586526251bc759151e433`, `PROJ/main.c`, implements the
  bundled driver's direct-send volume and semi-protected restart behavior.
  Its `PROJ/sdrv.bin` is byte-identical to the supplied driver. The proof
  with hashes is `../upstream/PINNED-DRIVER-VOLUME-PROOF.json`.
- Official Mednafen 1.29.0 and 1.32.1 sources `src/ss/ss.cpp` and
  `src/ss/smpc.cpp` establish the same NTSC master clock and 320-mode divisor.
  Release archive hashes and source provenance are saved under
  `../upstream/mednafen-1.29.0/` and `../upstream/mednafen-1.32.1/`.

Public source links:

- https://github.com/yaul-org/libyaul-examples/tree/develop/scsp-ponesound-pcm8
- https://github.com/ponut64/SCSP_poneSound
- https://github.com/ponut64/SCSP_poneSound/tree/71dac9d944d1e4156cb586526251bc759151e433/PROJ
- https://mednafen.github.io/releases/files/mednafen-1.29.0.tar.xz
- https://mednafen.github.io/releases/files/mednafen-1.32.1.tar.xz
