# WP01 R02 — original audio events and individual sample assets

This directory observes the unmodified arcade ROM with the same input sequence
used for WP01 R01. No source/repository outside this directory was modified.

## Inputs and reproduction

- ROM: `project_sources/01-wwfwfest.zip`, main 68000 SHA-1
  `a726c46f350723ea73158bc78055d17d418b14e2`.
- Original MAME 0.264 executable and launch configuration are recorded in
  `capture/launch.json`; `capture_audio.lua` installs write taps only.
- `capture/audio_writes.csv` contains exact machine timestamps for YM2151
  register writes, OKIM6295 writes, bank changes, and main CPU sound commands.
- `export_events.py` converts the scene to sample timestamps at 44,100 Hz.
- `decode_oki.py` decodes individual original ADPCM samples to PCM16BE and
  compares every output byte to the unmodified MAME 0.264 `oki_adpcm_state`
  implementation compiled independently in `verify_oki/`.

The new capture's full mixed reference audio from 22.108160 to 29.210624 seconds
is byte-identical to the reference used for R01. It remains reference material;
it is not the native implementation's audio asset.

## Scene events

The scene interval is 408 arcade updates / 7.102464 seconds / 313,219 output
samples when rounded to 44.1 kHz. At original frame1271, main commands are
`0x3100`, `0x3120`, `0x310E`; at frame1300, `0x3180` occurs twice.
Music command0E therefore starts fresh. In this capture the duplicate crowd
command does not create two overlapping copies: the Z80 writes actual sample18
start commands to OKI voice2 at 22.638690112848 and 26.103995340189 seconds.

`export/entrance_audio_events.h` supplies 748 events:

- 689 YM writes, including every explicit key write.
- 58 OKI byte writes.
- One bank write (bank0).

Clock/timer-only YM register writes are removed; their resulting musical events
are already in the stream. Same-value ordinary register writes are removed.
Seed the register shadow from `wp01_audio_initial_ym`, without keying voices on,
before playing the events. Do not assume that key-on snapshots alone contain
the final pitch: the original driver sometimes writes KC/KF after key-on.

The register snapshot cannot reconstruct pre-scene envelope or phase state.
The first ~17.6 ms of the full reference includes the preceding selection
music, before command00 silences it; native reproduction must not claim that
this small inherited tail is sample-exact solely from a register snapshot.

## FM allocation

The six music channels that receive full four-operator key-on masks are
`0,1,2,3,4,7`. Channels5/6 receive partial initialization keys only and no
carrier notes in this entrance. A static map of six four-operator voices needs
24 SCSP slots.

- Channel0: algorithm2, feedback7.
- Channels1–4: algorithm4, feedback7.
- Channel7: algorithm3, feedback7.

`entrance_note_patches.json` contains operator parameter snapshots in YM
register order **M1, M2, C1, C2**, not algorithm execution order. PMS/AMS and
LFO depths are zero for the played notes; noise is off. The exact per-register
stream remains the authoritative event representation.

## Individual OKI samples

Both are bank0, clocked at 8,000 samples/s. They are original sample assets,
not separately rendered musical stems or a recorded scene mix.

| Sample | Original voice | Samples | PCM16 bytes | Volume nibble / gain |
|---|---:|---:|---:|---:|
| 1 | 0 | 1,802 | 3,604 | 3 / 11/32 |
| 18 | 2 | 32,018 | 64,036 | 2 / 16/32 |

The ROM start/end tables are inclusive. OKIM6295's MAME 0.264 decoder starts
at signal0/index0, processes high nibble first, and clamps to signed12 bits.
The output is `signal*16`, preserving every signal bit. The older project
PCM8 decoder starts at -2 and quantizes; that path was deliberately not reused.

MAME's WrestleFest mono OKI route gain is0.90 (MAME0.264 `ddragon3.cpp`). Asset amplitudes are unscaled;
volume and mixing belong to the native playback implementation. JSON files
provide SHA-256 identities and the verification results.
