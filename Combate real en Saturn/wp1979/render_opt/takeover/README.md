# Controller takeover verification

Everything here uses ordinary controller levels or reads saved capture JSON.
The root agent runs the actual emulator capture with the final BIN/CUE.

## Capture and analyze the final disc

Run from the workspace root, supplying the final CUE if it is outside the
current native output directory:

```sh
python wp1979/measure/run_capture.py final-controller-takeover \
  --frames 1000 --video --actors --snapshot-every 30 --wall-timeout 240 \
  --core wp1979/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so \
  --schedule wp1979/render_opt/takeover/controller-schedule.json \
  --cue /absolute/path/to/final.cue

python wp1979/render_opt/takeover/analyze_takeover.py \
  wp1979/measure/final-controller-takeover
```

`run_capture.py` verifies the disc's A.BIN against the linked program before
execution and resolves read-only watch addresses from that exact ELF. Use
`--actors`: the analyzer needs actor0 through byte +A9. It also records the
demo-active flag, game frame, fatal state, applied controller events, hashes,
video, and audio. The same final ELF/BIN must still be in the native output
directory when the wrapper copies them.

The schedule leaves player one neutral until emulator frame 540, after the
observed audio-build startup near frame 473. It then holds Left, Up, Right,
and Down in separate intervals with neutral releases, followed by individual
A/B/C pulses. All events go through `retro_set_input_state`, the normal
libretro pad callback. Schedule frame numbers are emulator frames, not game
logic frames. No memory changes are supplied by the schedule.

The analyzer requires demo_active=1 before the first physical event, an
observed transition to zero within eight emulator frames, no later
reactivation, continued native progress, matching decoded directions and
A/B/C levels, and neutral input after release. It also requires at least
twelve exact Hogan walking steps across two direction intervals. Each step
is selected only when Hogan is active, legal, in state 1, and has the expected
held direction. Combat, ropes, or collisions can block movement; such a run
fails the evidence threshold rather than receiving invented movement.
Repeated motion opposite to a requested physical direction fails explicitly.

The output is `takeover-analysis.json`. The analyzer does not load an
emulator; it reads JSON and audits the controller/memory-reader source route.

## Confirmed final input mapping

These are **Saturn button names** in the schedule, despite the differing
libretro face-button identifiers:

| Schedule button | Libretro ID | Native active-low byte | Actor +A8 direction | Actor +A6 action |
|---|---:|---:|---:|---:|
| Right | 7 | FE | 1 | 0 |
| Left | 6 | FD | 2 | 0 |
| Up | 4 | F7 | 8 | 0 |
| Down | 5 | FB | 4 | 0 |
| A | 0, Retro B | EF | 0 | 1 |
| B | 8, Retro A | DF | 0 | 2 |
| C | 11, Retro R | BF | 0 | 4 |
| Start | 3 | 7F | 0 | 100 |

Hexadecimal fields omit the `0x` prefix. Up and Down include the causally
verified adapter correction integrated by the root agent.

## Vertical-direction red/green evidence

The earlier adapter mapped physical Up to active bit 4 and Down to bit 8.
That reversed movement. The actual ROM direction table at $F2D6 maps index
4 to angle $0000, while index 8 maps to $0080. The unchanged native physics
produces positive Y for the former and negative Y for the latter. Native
projection adds world Y to screen Y, so positive Y is south.

The physical route is independently verified in the actual core/SDK:

- Libretro Up ID 4 calls `PerPadUpPressed`, clearing bit $10 of the first
  Saturn pad byte. Yaul inverts the active-low report and constructs $1000,
  matching `PERIPHERAL_DIGITAL_UP` and the `up` bitfield.
- Libretro Down ID 5 clears pad bit $20, becoming Yaul $2000 and `down`.

There is no compensating Up/Down inversion in that route.

`physical-up-down.patch` changes only the two adapter masks and their
explanatory comment in `wp1979_entry.c`. Apply that narrow patch; the complete
entry snapshots are retained as test evidence and are not intended to
overwrite later unrelated root edits.

```sh
python wp1979/render_opt/takeover/run_direction_probe.py
```

The frozen baseline and proposal reproduce:

```text
FAIL baseline UP   physical_low=fb index=4 angle=0000 delta_fixed=(0, 110592)
FAIL baseline DOWN physical_low=f7 index=8 angle=0080 delta_fixed=(0,-110592)
PASS proposed UP   physical_low=f7 index=8 angle=0080 delta_fixed=(0,-110592)
PASS proposed DOWN physical_low=fb index=4 angle=0000 delta_fixed=(0, 110592)
```

Right and Left pass in both versions. This probe reads the actual canonical
ROM and compiles the native input decoder, direction lookup, physics routine,
and physics tables. It checks the direction values against the ROM and the
Hogan walk magnitude against ROM byte $116AE. Results and hashes are in
`direction-red-green.json`. Baseline source SHA-256 was checked against the
recorded pre-integration hash when preserving the snapshot.

## NOINPUT and analyzer regressions

```sh
python wp1979/render_opt/takeover/run_host_test.py
python wp1979/render_opt/takeover/test_analyzer.py
```

The first compiles the current native `wp1979_input.c` twice, normally and
with `WF_R18_REPLAY_NOINPUT`, plus the current input decoder. It extracts the
actual current `controls()` implementation for a host logical-field test.
It verifies replay levels, permanent physical takeover, neutral release,
NOINPUT's neutral startup at tape landmarks, passthrough of all 256 physical
levels, and the final corrected direction/action mapping. The latest run
after the root integrated the direction correction passes; source and tape
hashes are in `host-test-result.json`.

The analyzer regression uses explicitly marked synthetic data only. It
checks a valid result and rejects missing takeover and reversed vertical
movement. These fixtures are not actual capture evidence; the ordinary
analyzer rejects synthetic captures. Results are in `analyzer-test-result.json`.

This verifies an emulator controller-callback path. It does not claim a test
on a physical Saturn or a physical controller connected to hardware.
