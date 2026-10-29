# WP01 R02 native audio: supplied implementation inventory

## Scope and provenance

Read-only inspection of supplied audio packs 02, 10, 25 and the historical
Test255 source. Existing work was not modified. All new files from this audit
are under `wp01_audio_r02/research_inventory/`.

The normal stage-zero aisle entrance requests `$310E` (music command `0x0E`),
as recovered from the main ROM in
`wp01_work/research_rom/ROM_ENTRANCE_FINDINGS.md`, line 25.

## What already exists

- Pack 02/25 `wf_audio_sfx.c` is a native C implementation of 98 recovered
  SFX commands, `0x20..0x81`, including priorities, bank policies, delay
  counts and cleanup. Three logical SFX voices are modelled. The fourth
  original OKI voice is used by the music subsystem.
- Pack 25 `wf_audio_saturn.c` sends those events to individually decoded
  samples in SCSP RAM through PoneSound. It is a real per-sample backend,
  but uses **PCM8**, an approximately mapped PoneSound volume, and a 1 ms
  SH-2 FRT interrupt feeding the original Timer-A rate.
- Pack 25/02 `assets/SDRV.BIN` is the supplied 8,732-byte PoneSound 68000
  binary. Its source/IRQ implementation is not in these packs. The
  supplied wrapper uses a shared control block at sound RAM
  `0x408 + 47*1024`; its slot commands are serviced by the binary driver.
- Pack 10/A53 `capture_full_audio_a50.c` is a **host Z80 emulator harness**
  which runs the original sound program and records YM/OKI writes.
  `render_combined_a50.c` is a **host Nuked-OPM renderer**, not a Saturn
  SCSP FM engine. Its full combined output is prerecorded PCM.
- The Test255 music backend is CD-DA. It does not solve native FM.
- No `m68k-elf-gcc`, `m68k-linux-gnu-gcc`, m68k assembler, `vasmm68k_mot`,
  or `clang` was present in PATH. The supplied `sh2eb-elf/bin` toolchain
  contains SH-2 tools only.

## Important correction to the supplied OKI decoder

`wp01_work/research_base/test255/wf_oki_decode.c` contains the usual OKI
49-step decoder, but initializes signal to **-2** and then quantizes the
result to signed 8 bits. This is not identical to the MAME 0.264 reference
used by WP01.

The exact `mame0264` primary source was downloaded for verification:

| File | SHA-256 |
|---|---|
| `mame0264/okiadpcm.cpp` | `a49dbd1089b3163a3a38480f6d877462c506b9f85e66a0cf5ea015b2a193dcd8` |
| `mame0264/okim6295.cpp` | `7baaaf9cb6b89edbe486dd3737b96eef83a0e372eadeb5807bd8095e901c5f42` |
| `mame0264/okim6295.h` | `810c1c33a4ad68a6345e8f133312fa0e2931430f1bf9fd4df5fb69861ba27bbe` |
| `mame0264/okiadpcm.h` | `28f70d023aa2f18a0287b1290ac108692b5f270ce91622c636742af2e8909ab4` |

Source URL pattern:
`https://raw.githubusercontent.com/mamedev/mame/mame0264/src/devices/sound/FILE`.
These files retain their BSD-3-Clause copyright headers.

Verified details:

- `okim6295.h:75`: the 6295 uses `oki_adpcm_state`.
- `okiadpcm.cpp:43`: that state resets signal and step to **0**.
- `okiadpcm.cpp:160`: **-2** belongs to `oki_adpcm2_state`, a different
  decoder whose step-index deltas also differ.
- `okim6295.cpp:278`: count is `2 * (stop - start + 1)`, inclusive.
- `okim6295.cpp:355`: the high nibble is decoded first.
- The signal is clamped to `[-2048,2047]`. Preserve it as `signal * 16`
  in signed PCM16BE rather than repeating the historical PCM8 quantization.
- Volume numerators are `{32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0}` / 32.
  A write to a still-busy voice is ignored. The two-byte pending-command
  state and independent stop masks must be preserved.

## Practical isolated entrance implementation

The events investigation identified original bank-zero samples 1 and 18.
The original tables give 901 and 16,009 ADPCM bytes, yielding 1,802 and
32,018 decoded samples. Both fit resident in SCSP RAM as 67,640 bytes of
PCM16, before guards/alignment. This avoids CD access during the scene.

A direct SCSP player with fixed slots and original timestamped events is
more suitable than integrating the old PoneSound SFX subsystem: the
latter owns its own FRT timer, voice allocation and master settings and
would conflict with a separate native FM engine unless explicitly
redesigned. The original event streams, sample identities and amplitudes
can be reused; the hardware transport should have one owner.

The FM investigator/event investigator have established six audible FM
channels (0,1,2,3,4,7), totaling 24 four-operator slots, plus original OKI
voices 0 and 2. This fits the 32 SCSP slots for this scene. FM synthesis,
envelopes, modulation, feedback and timing still require implementation
and comparison; the supplied packs do not already contain that engine.

## Implemented native OKI player

`wp01_oki.c/.h` now implement the raw original two-byte OKI command protocol
with four logical voices, busy-start rejection, independent stop masks,
sample-derived natural expiration and a bank-zero-only contract. Calls
write only SCSP slots 24..27 through a supplied callback. All timestamps
are integer 44,100 Hz sample times; scheduling/interrupt ownership belongs
to the caller. No global register, IRQ, CD, heap or FM slot is touched.

`build_oki_bank.py` builds a 67,680-byte resident bank at SCSP RAM `0x44000`,
outside the two guarded FM sine banks (which occupy `0..0x40FFF`). There is
one zero interpolation guard per source sample, with 32-byte alignment.

The constant original volume is folded into each separate sample:

- sample1: `11/32`, then reference board gain `9/10`;
- sample18: `16/32`, then reference board gain `9/10`.

The final signed PCM16 rounding is ties-to-even and has a maximum error
of 0.5 PCM16 LSB. Production voices use SDIR and bypass SCSP envelope/TL
attenuation. The generic module also supports raw samples with nearest
TL values, but that quantized path is not used by these two production
assets. The bank contains two reusable individual sounds, not a scene mix.

The closest SCSP pitch for 8,000 Hz is `0x69CE`, giving
`7,999.5849609375 Hz` (-51.8798828125 ppm). Original hardware busy duration
is modelled at exactly 8,000 Hz; this tiny playback-pitch quantization is
explicitly not claimed to be bit-identical output.

### Board-mix correction

The pinned MAME 0.264 WrestleFest source uses YM L/R gains of **0.45 each**
and OKI gain **0.90**, in `mame0264/ddragon3.cpp:925–928`. The supplied
A53 renderer's 0.33/0.66 values do not match that pinned reference.
`ddragon3.cpp` SHA-256:
`3c766308ee511828e6d3f2e264e04d769aea98f87e849dd014e8122f69d6633f`.
The source URL and these calibration values are recorded in the generated
bank manifest. Mixer calibration is a reference-emulator board model,
not a value recovered from the program ROM.

### Verification performed

- An intentionally empty player failed the first protocol test assertion.
- The completed host player passed with `-std=c11 -O2 -Wall -Wextra -Werror`.
- Protocol tests cover pending bytes, busy rejection, original stop masks,
  simultaneous independent voices, exact logical expiry, silent-volume
  busy flags, unsupported bank, missing/mismatched assets, out-of-bounds
  sound-RAM assets, slot ownership and the optional raw TL path.
- Replay of all 748 captured original audio writes produced 19 accepted
  OKI starts (17 sample1 and two sample18), 16 natural ends, one explicit
  active-voice stop, zero ignored-busy starts and zero asset/bank errors.
  Only original voices0 and2 remain active at the end of the scene.
- The source compiles for the supplied SH-2 target with
  `-m2 -mb -std=c11 -Os -Wall -Wextra -Werror -ffreestanding`.
- This is host/protocol/target-compilation validation. Integrated audible
  emulation and physical Saturn playback remain the caller's next gates.
