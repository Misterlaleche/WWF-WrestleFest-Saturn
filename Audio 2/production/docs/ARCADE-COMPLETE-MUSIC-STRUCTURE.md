# Full-board music structure: all 18 commands

The coupled original-ROM execution now supplies bounded structural evidence
for all 18 music commands: **three natural FB endings, 12 complete visible
score-state recurrences, and three verified sets of independent channel
cycles**. The latter commands, `0B`, `0C` and `11`, use channels with different
cycle lengths. Their music does not require an invented short global loop.

The [public metadata](../tools/reference/evidence/full-board-complete-music-structure.json)
contains the classifications, measurements, source/input SHA-256 identities
and versioned result. It contains no ROM, original state arrays, traces or PCM.
The model is the original full Z80 coupled to Timer A+B, busy/status/IRQ and
ROM-addressed OKI. These are host software observations.

## Per-command evidence

An FB timestamp is entry to the original global end handler, before forced
fade. A visible interval separates byte-identical complete RAM, visible YM
register/key state and MMIO checkpoints with additional positive key writes.
It is not an asserted whole-composition or PCM period.

| Command | Observed structure | FB entry or visible-state interval |
|---|---|---:|
| `01` | Visible score-state recurrence | 59.777149889 s |
| `02` | Visible score-state recurrence | 42.182700595 s |
| `03` | Visible score-state recurrence | 35.591644748 s |
| `04` | Visible score-state recurrence | 166.973261965 s |
| `05` | Original natural FB ending | 2.351460591 s |
| `06` | Visible score-state recurrence | 49.213166478 s |
| `07` | Visible score-state recurrence | 31.637067560 s |
| `08` | Original natural FB ending | 1.783529192 s |
| `09` | Original natural FB ending | 6.607803226 s |
| `0A` | Visible score-state recurrence | 56.243650240 s |
| `0B` | Independent channel cycles verified | See channel service counts below |
| `0C` | Independent channel cycles verified | See channel service counts below |
| `0D` | Visible score-state recurrence | 21.091362450 s |
| `0E` | Visible score-state recurrence | 28.121811850 s |
| `0F` | Visible score-state recurrence | 75.577385394 s |
| `10` | Visible score-state recurrence | 6.591049421 s |
| `11` | Independent channel cycles verified | See channel service counts below |
| `12` | Visible score-state recurrence | 38.640038329 s |

## Independent channel cycles in `0B`, `0C` and `11`

Each unit below is **one acknowledged original Timer B service**. Boundaries
are canonical checkpoints after successive original FD returns, before `1F`.
The tool checks the FD token/target against the pinned ROM, constant return
pointers, and the controller that decrements `C066 + channel`. In the chosen
equal-pointer witnesses, the remaining RAM differences are FM note countdowns:
all eight for `0B`, channel 1 for `0C`, and channels 4 and 6 for `11`. Those
witnesses have equal visible YM/MMIO state and no remaining stack or modulator
differences that explain the global RAM mismatch.

| Channel | `0B` services per cycle | `0C` services per cycle | `11` services per cycle |
|---|---:|---:|---:|
| FM 0 | 4704 | 4608 | 6144 |
| FM 1 | 4708 | 4598 | 6144 |
| FM 2 | 4704 | 4608 | 6144 |
| FM 3 | 4704 | 4608 | 6144 |
| FM 4 | 4704 | 4608 | 6148 |
| FM 5 | 4704 | 4608 | 6144 |
| FM 6 | 4734 | 4608 | 6143 |
| FM 7 | 4710 | Original F0 channel end | 6144 |
| PCM 8 | 4728 | 192 | 6144 |

All complete **register/value sequences and relative Timer B service
positions** match byte for byte for the 23 active FM channels: 11 complete
FD-to-FD windows per channel in `0B` and `11`, and nine in `0C`. The PCM
channel's OKI/PLAY/STOP/BANK/F1SKIP sequences and service positions also match:
11, 247 and 11 windows respectively. No additional global musical YM writes
were omitted between these returns; the other writes control the timers.

The measured Timer B register values are `DC`, `D4` and `E4`. Normal OPM mode
is checked from the captured state. With the 3,579,545 Hz external clock:

`Timer B period = (256 - register12) × 16 × 32 × 2 external cycles`.

These values give 36,864, 45,056 and 28,672 external cycles per timer period.
This specifies the musical service unit; the tool separately tests the actual
external-cycle timestamps of every write.

**Register/value/external-ΔT sequences do not repeat exactly three times.**
The comparison has zero tolerance and also excludes the first
checkpoint-relative delta in a separate test. Internal differences remain.
Thus the verified claim is repetition of each channel's observed score-control
cycle, including its musical service positions. It is not sample-exact PCM
periodicity or a universal claim about future playback. A finite capture
duration, silence after `1F`, CPU refresh equality and a short global period
are not prerequisites or substitutes for this musical evidence.

## Reproduce from existing captures

Tool version **1.0.1** is
[`analyze_full_board_channel_structure.py`](../tools/reference/analyze_full_board_channel_structure.py).
It performs no new CPU/audio replay and needs no private audit scripts.
Supply the frozen observer output (`music-structure.json` and compressed
per-command states), the corresponding full-board reference directory
(`inputs/SOUND.ROM`, `deps/opm.h`, and the existing traces), Python 3 and a C
compiler. The tiny compiled helper derives structure offsets from the pinned
headers; it does not execute the sound program.

```sh
python3 tools/reference/analyze_full_board_channel_structure.py \
  --structure-dir /private/full-board-structure \
  --reference-root /private/full-board-references \
  --out /private/channel-structure-v1

python3 tests/test_full_board_channel_structure.py
```

The default command set is `0B 0C 11`; `--commands` accepts explicit original
IDs. Outputs require a new private directory outside the source package.
`channel-structure.json` and per-command reports retain detailed private
evidence. `public-summary.json` is produced through an explicit field
allowlist and is the only output intended for public metadata distribution.
Optional `--prior-summary` compares with the earlier private exploratory
supplement. The reproduced version matched its 23 FM + three PCM results.

The new synthetic regressions distinguish independent channel periods,
musical service timing, external-cycle timing and global state. They also
reject changed values, changed service positions and fewer than three
complete cycles, and prevent private state/event fields from entering the
public export. The complete Python suite passed **74 tests** for this version.

## Evidence identities and limits

- Frozen all-18 report SHA-256:
  `b70888205820d7c35f47502cb212b697c7c92ec87be77ab616c38d9520204785`.
- Earlier private channel supplement SHA-256:
  `e53108b5358532dfb7c265235e6a3b1106300677fd39bd6217946fa61ac1e7d9`.
- Reproduced v1.0.1 detailed report SHA-256:
  `553c40a8fe90343cc879615ad0159633317db52befc1f4d3fa10f2395645034a`.

The four frozen observer/analyzer/test sources retain their recorded hashes.
The new tool preserves its version, source hashes, capture/trace identities,
exact mismatch evidence and comparison with the prior result. The original
global classifications remain recorded: the supplement explains the three
pointer-only cases with stronger per-channel evidence. Native synthesis
fidelity, transport/deadline acceptance and physical hardware validation
remain separate questions.
