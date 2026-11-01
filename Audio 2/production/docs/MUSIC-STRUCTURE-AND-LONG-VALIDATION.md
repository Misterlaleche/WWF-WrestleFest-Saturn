# Original score endings, recurrence and fade observations

## What was actually established

The original 65,536-byte sound ROM was executed with the preserved full Z80
core for each of the 18 commands, after its original two-second boot. Each run
continued for 600 seconds, received original command `1F`, then ran another
eight seconds. This is a control-flow observation. It neither replaces the
native engine with PCM nor certifies a physical arcade or Saturn recording.

Three commands enter the original global end handler: `05`, `08`, and `09`.
Twelve other commands repeat the complete RAM and visible musical register
state within the window. For `0B`, `0C`, and `11`, only pointer equality was
found. **No command demonstrated equality of the complete CPU, RAM, visible
registers, IO and relative timer state; no PCM loop is proven by this test.**
The finite observation window does not establish completeness for the remaining
commands. Backward original `FD` branches were observed for all 15 ongoing
commands, demonstrating repetition in their executed streams but not a
sample-exact period for the complete synthesis state.

All 18 original `1F` fades clear the driver's active/current-music fields within
2.071614–5.239817 seconds of command delivery. Eight seconds therefore covers
the measured **control** fade in this experiment. The audible envelope and PCM
tail must be measured separately; control completion is not digital silence.

## Per-command evidence

Command identifiers below are decimal / hexadecimal. The FB time is entry to
the ROM end routine, not the last audible sample. “Visible state interval” is
the first measured interval between byte-identical complete RAM, all visible
YM registers/key masks and IO state at acknowledged timer-service boundaries.
It is not an asserted duration for a complete composition or PCM loop.

| Command | Natural global end, before fade | First equal visible state interval | `1F` to active flag clear |
|---|---:|---:|---:|
| 01 / `01` | No FB end observed | 59.777150 s | 3.491363 |
| 02 / `02` | No FB end observed | 42.182724 s | 5.239817 |
| 03 / `03` | No FB end observed | 35.591695 s | 3.931906 |
| 04 / `04` | No FB end observed | 166.973262 s | 2.071614 |
| 05 / `05` | FB entry 2.351523 s | Not applicable after natural end | 4.367883 |
| 06 / `06` | No FB end observed | 49.213174 s | 3.080565 |
| 07 / `07` | No FB end observed | 31.637054 s | 3.152854 |
| 08 / `08` | FB entry 1.783564 s | Not applicable after natural end | 3.491345 |
| 09 / `09` | FB entry 6.607835 s | Not applicable after natural end | 4.367883 |
| 10 / `0A` | No FB end observed | 56.243641 s | 3.491368 |
| 11 / `0B` | No FB end observed | Pointer equality only | 3.931903 |
| 12 / `0C` | No FB end observed | Pointer equality only | 4.800410 |
| 13 / `0D` | No FB end observed | 21.091365 s | 5.239816 |
| 14 / `0E` | No FB end observed | 28.121817 s | 3.491366 |
| 15 / `0F` | No FB end observed | 75.577368 s | 4.697143 |
| 16 / `10` | No FB end observed | 6.591039 s | 4.367877 |
| 17 / `11` | No FB end observed | Pointer equality only | 3.054243 |
| 18 / `12` | No FB end observed | 38.640016 s | 2.968718 |

For the three finite commands, the corrected 30-second board-clock reference
windows have last nonzero samples at approximately 2.638571 s (`05`),
2.129229 s (`08`), and 7.003560 s (`09`). These are separate digital-reference
observations; the ROM handler time above does not include their audible tails.
The historical A53 render has a doubled YM clock and different PCM format, so
its audible end times are not substituted here.

## Detection and observer validation

`probe_music_state.c` includes the immutable A53 capture context and uses the
original full `z80.c`. It observes rather than changes the original memory
callbacks. The handler guards are fixed to the verified original sound ROM:
`FB` at PC `0E80`, channel-stop `F0` at `0D2D`, and backward `FD` at `0E17`.
Every guard also checks the actual byte at the current stream pointer.
The original `FB` routine clears `C006/C007`. An observed fade clear is a write
of zero to `C006` after sending `1F`; it is recorded independently of natural
termination.

Canonical snapshots require PC `003A` **and an acknowledged Timer B**. This
avoids comparing idle-loop states before the pending music tick was serviced.
The strict comparison preserves all CPU registers, including refresh, flags
and interrupt state, the complete 2 KiB RAM, all IO fields, every visible YM
register and key mask, and the relative next timer deadline. Only absolute
cycle accumulation and host function/context pointers are normalized. The
last arbitrary final-instruction snapshot is excluded from recurrence tests.
Every candidate match is confirmed with complete component bytes after hashing.
A match also requires additional positive key writes, excluding idle silence.

The model retains the original oracle's MMIO assumptions, including its Timer B
model and OKI status read of zero. Hidden YM/OKI phase, envelope, feedback and
sample position are not present in these snapshots. Equality at any score
level therefore cannot establish equality of hidden synthesis state.

`test_music_state_probe.c` ran all 18 commands for four seconds followed by
`1F` and eight more seconds, comparing the observed execution with the same
unobserved original CPU. CPU/RAM/IO/timer state matched at every 4,096-instruction
checkpoint and each phase boundary, and the complete original MMIO write logs
were byte-identical. The separate Python counterexamples reject pointer-only,
RAM-only and visible-state-only equality as complete control-state proof, and
reject silence as evidence of an original end handler.

## Reproduce privately

Supply the original sound ROM extracted from the user's ZIP. No ROM state
snapshot, raw trace or audio is stored in this source package.

```sh
cc -O3 -std=c99 -I src/reference_driver tests/test_music_state_probe.c \
  src/reference_driver/z80.c -o /private-output/test_music_state_probe
/private-output/test_music_state_probe /private-input/SOUND.ROM
python3 tests/test_music_structure.py
python3 tools/reference/analyze_music_structure.py \
  --rom /private-input/SOUND.ROM --out /private-output/music-structure \
  --seconds 600 --tail-seconds 8 --jobs 2
```

The detector validates ROM and core SHA-256 values before execution and records
all relevant source identities before/afterward. Full state bytes are written
only to the explicit private output and retained as gzip captures. The source
package carries only the metadata evidence
`tools/reference/evidence/original-music-structure-600.json`.

## Prolonged native test is a distinct gate

`tools/run_music_matrix.py --score-seconds 600 --tail-seconds 8 --transitions`
builds all selected commands from one immutable source/input snapshot. Each
music case receives its original command at sample zero and `1F` at the requested
600-second SCSP timestamp. The separate transition case sends
`01,06,12,1F,03,1F,02,01,1F` at three-second intervals and retains the last eight
seconds. Only original command IDs are used.

The diagnostic harness admits API calls on video-frame service boundaries.
The runner reads each actual word, SCSP timestamp, video frame and return code
from native RAM. It records admission delay and applies a declared 1,475-sample
admission tolerance separately from the existing firmware transport-lateness
gate. This is not a claim of cycle-exact Z80 delivery. The final two seconds of
unaltered stereo PCM are measured for peak, RMS and exact digital silence;
that observation does not by itself determine fidelity acceptance.

One to four explicitly selected displays are supported. Optional upstream
`--presentation-speed 4` changes presentation pacing, not emulated hardware
clocks. Its use is based on the separate same-executable test with 2,646,000
identical stereo sample values over 30 seconds. Every native result must still
identify its exact firmware/source snapshot and maintain the distinction
between runtime stability, control-sequence equivalence, digital audio fidelity
and physical hardware approval.
