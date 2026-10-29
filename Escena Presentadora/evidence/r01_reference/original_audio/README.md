# WP02 original intro background: identification and source audit

## Result

The original intro has background music already running before the first
prematch cue. The actual main-CPU trace identifies it as command **0x310E**.
The production source selected by the parent integration is the actual-session
MAME isolation capture, not the separate reset-state Nuked-OPM research render.

The isolated capture contains original YM output plus original OKI percussion
on **physical voice 0, mask 1**. It matches the original full recording's quiet
tail at **zero measured lag**, normalized correlation **0.996563**, with a
matched level difference of **−0.04338 dB** (full / isolated). This is strong
evidence for the original background's content, phase and level. The captures
are not claimed to be bit-identical: their output rates differ and suppressing
SFX starts slightly changes CPU branch/IRQ timing.

## Authoritative times and physical voices

Source:
`/workspace/scratch/8e35fd0cc989/wp02/verification/mame/audio_trace/writes.csv`.
Its SHA256 and the original ROM identities are in
`confirmed_background_report.json`.

| Event | Main trace frame | Time from boot |
|---|---:|---:|
| Music command 0x310E | 2834 | 49.334618083 s |
| Intro entry cue 0x3132 | 3248 | 56.550597916 s |
| First displayed intro frame | 3249 | 56.558592000 s |

Music phase at the entry **sound command** is **7.215979833 s**. The displayed
frame boundary is later, so these are distinct alignment references.

| Physical OKI voice | Start mask | Observed intro role |
|---:|---:|---|
| 0 | 0x1 | Music percussion, sample 1, volume nibble 3 (11/32) |
| 1 | 0x2 | No intro start observed |
| 2 | 0x4 | Common reaction samples 17 / 18 |
| 3 | 0x8 | Announcer names and phrases 23, 24, 27, 31, 37, 38, 39 |

There are 79 music sample-1 starts from 0x310E through the original recording's
end: 24 while bank 0 is selected, 55 after bank 1 is selected. The sample's
original ADPCM payload is byte-identical in both ROM banks. It has 901 ADPCM
bytes / 1802 decoded samples / 0.22525 seconds at 8 kHz.

## Identification measurements

`match_background.py` searches every valid original ROM sample represented in
the A16 bank: 73 entries in bank 0 and 41 in bank 1. Direct inspection of both
ROM tables confirms this covers all valid entries. It searches both sample
heads and arbitrary phases within circularly extended samples. No convincing
single sample accounts for the persistent tonal bed. Common reaction samples
17 and 18 have maximum absolute correlations only 0.04664 and 0.03577 in that
search.

The original recording's 82.0–83.86 s background matches the earlier
53.87825–55.73825 s window with correlation **0.944558**, at a recurrence of
**28.12175 s**. The narrower 82.8–83.86 s window gives correlation **0.936942**
at the same recurrence. This lies within one 8 kHz analysis sample of the
archived 0x0E visible-state period, **28.121816879 s**. The actual 0x310E trace
then confirms the candidate independently.

The archived period is a driver / chip-visible state recurrence, **not a proven
PCM loop**. The archive explicitly marks `pcm_loop_proven=False`; no waveform
loop or shorter motif has been substituted here.

Measurement details and source hashes are in:

- `initial_match.json`
- `confirmed_background_report.json`
- `summarize_background.py`

## Actual-session isolated capture

Production source prepared by root:

`/workspace/scratch/8e35fd0cc989/wp02/verification/mame/audio_music/music.wav`

| Property | Value |
|---|---|
| Sample rate | 44,100 Hz |
| Channels | Stereo, both channels bit-identical |
| Representation | Signed PCM16 WAV |
| Frames | 3,991,932 |
| Duration | 90.52 s |
| WAV bytes | 15,967,772 |
| Clipped values | 0 |
| SHA256 | `1bf99c064837cdf60b91bedd69cbfa089c489e9a5eb225ee829ac9b3d8829e5c` |

The supplied MAME executable reports version `0.264 (unknown)`; its binary
SHA256 is recorded in `render_provenance.json`.

Root's `audio_music.lua` leaves YM and bank writes intact and changes only
the second byte of each OKI start command to `data & 0x1f`. This retains
physical voice 0 and the original volume nibble while suppressing other
physical voice starts. Existing stop writes are left intact. It holds the
main CPU at frame 4817, before gameplay starts a different music command,
while sound CPU / YM / OKI continue uninterrupted through frame 5200. This
extra tail is a source-extraction extension, not a claim about normal gameplay.

### Comparison with original 48 kHz full capture

For comparison only, both captures are converted to a common 48 kHz analysis
rate and filtered with a third-order, zero-phase 100–10,000 Hz Butterworth
bandpass. No delivery asset is resampled by this audit. Each window searches
within ±20 ms for alignment.

| Original window | Lag | Normalized correlation | Full / isolated matched gain | Full / isolated raw RMS |
|---|---:|---:|---:|---:|
| 49.4–49.8 s, before intro | 0 | 0.948741 | −0.46897 dB | 0.1395381 / 0.1395372 |
| 82.05–82.75 s | 0 | 0.997713 | −0.02636 dB | 0.1519893 / 0.1520209 |
| 82.8–83.8 s | 0 | 0.996601 | −0.04112 dB | 0.1641863 / 0.1642132 |
| 82.8–83.86 s | 0 | 0.996563 | −0.04338 dB | 0.1650994 / 0.1651185 |

The short pre-intro window begins after 0x3100 / 0x310E stopped the preceding
effects and ends before the first reaction start at 49.847439828 s. Its lower
band-limited correlation is reported rather than concealed; total RMS is
essentially equal. The long quiet tail is the stronger verification window.

### Trace equivalence and limits

Before frame 4817:

- All 25 main sound commands and their timestamps match exactly.
- All 106,978 YM writes excluding timer-control register 0x14 have identical
  ordered register/value pairs. Their median timestamp difference is
  5.028 microseconds; the maximum is 403.404 microseconds.
- The net three fewer YM writes are entirely timer acknowledgments: three
  pairs of 0x14=0x2F and 0x14=0x1F become three combined 0x14=0x3F writes.
- All three bank values and their order match, with a maximum timestamp
  difference of 19.555 microseconds.
- Every filtered OKI start command has a physical voice mask of zero or one.

The script changes no YM register/value pair directly, but suppressing SFX
starts changes the OKI busy state seen by the sound CPU. The small resulting
branch / IRQ timing variation is measured above. This is why the report uses
both waveform evidence and register comparisons instead of calling the
isolation bit-exact to the full session.

Full measurements, source script/log hashes, and the reproduction script:

- `isolated_capture_audit.json`
- `audit_isolated_capture.py`

The parent will select one identical source channel, crop the 29-second body
from the entry sound-command time 56.550597916 s, and encode it losslessly.
This audit does not choose a native playback gain, alter the PCM, or implement
that transport.

## Independent continuous renderer: research output only

`FM0E-60s-44100-mono-s16be.pcm` is a **continuous 60-second** original-driver
render from a reset sound-chip state, retaining original YM sequencing and
sample-1 percussion. It is not the production source selected above.

| Property | Value |
|---|---|
| Format | 44,100 Hz, mono, signed PCM16 big-endian |
| Samples / bytes | 2,646,000 / 5,292,000 |
| Peak magnitude | 14,464 |
| Clipped samples | 0 |
| SHA256 | `5c5a61d7b1212ac407d9599c0be4d29546daad562f1e8551d715fd7eb005dc58` |

The WAV with the corresponding name is a lossless byte-order/header wrapper
of the same data. `render_comparison.json` documents that a reset-state render
only reaches correlation 0.469095 against the actual-session quiet tail under
the tested phase search. Its absolute internal phase/state therefore cannot
be treated as the original session's waveform. No arbitrary timing shift,
short loop, resampling, or replacement sound was introduced to hide that.

### Dependencies and precision correction

Nuked-OPM is pinned to commit
`f209e6ed3712032b641d53ce8fb24824eae6adc3`, with verified Git blobs:

- `opm.c`: `19a3fdc9c4a82d2c9bb0b8564cdc12206035d304`
- `opm.h`: `3d0e6c4b2c225b7bc1b46a0ac7bd729758fd0367`

Superzazu Z80 is pinned to primary-upstream commit
`d64fe10a2274e5e40019b1086bf7d8990cbc5f23` dated 2021-11-10. The original
handoff names the dependency without supplying its historical commit. The
chosen source reproduces the unchanged archived A53 FM0E 12-second output
**exactly**, SHA256
`21cd49ea2e515b887ea3ff38d4a63a14037fbff882d3c114eb849f83436df9c7`.
The downloaded source identities are recorded in `vendor/z80/PROVENANCE.json`.

`render_combined_pcm16.c` is a local copy of A50 with only PCM input precision
and final-sample holding corrected. It reads signed PCM16BE instead of PCM8,
applies the same original OKI gain table, and holds the last decoded sample
for its complete 8 kHz interval. The YM implementation, original register
trace, clock and reference mix contract are retained. Mix coefficients remain
the handoff's Q15 approximations to YM L×0.33 + YM R×0.33 + OKI×0.66.

`prepare_music_pcm16.py` extracts original sample 1 directly from `31j10.ic73`.
All 1802 decoded 12-bit values were checked against the independent recovered
C decoder before its old PCM8 conversion. Upper eight bits reproduce the A16
pack exactly. Raw signed PCM16 is the original signal multiplied by 16.

`test_render_pcm16.c` failed on the old input path (expected 4656, received 18)
and passes on the adapted path. It checks sign, retained low bits, gain and
the duration of the final sample. No integration files were edited.

### Minimal reproduction sequence

Use the source-local paths shown in this directory and the read-only handoff:

1. Compile `capture_full_audio_a50.c` with `vendor/z80/z80.c`.
2. Run `capture_full_audio ORIGINAL_31A11_ROM 0E 60 FM0E-60s.csv`.
3. Run `prepare_music_pcm16.py` to extract and verify the original percussion.
4. Compile `render_combined_pcm16.c` with `vendor/nuked-opm/opm.c`.
5. Run `render_combined_pcm16 FM0E-60s.csv MUSIC0E-PCM16.PAK
   MUSIC0E-PCM16-INDEX.csv FM0E-60s-44100-mono-s16be.pcm`.

The archived 28.121816879-second recurrence is intentionally unused by this
pipeline. Continuous synthesis runs for the complete 60-second duration.
