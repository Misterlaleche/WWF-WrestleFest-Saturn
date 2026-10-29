# Logical observation triggers for ordinary controller callbacks

This isolated proposal changes only the capture/test utilities. It does not
change the game, core, replay tape, controller mapping, VDP timing, or emulated
memory. No new Saturn build or capture was performed for this proposal.

## Files to use

- `capture-logical-schedule.patch`: proposed change to
  `wp1979/tooling/capture_yabause.py`, 40 net added lines.
- `capture_yabause-proposed.py`: full proposed file for review. The adjacent
  baseline copy preserves the exact original capture utility.
- `controller-logical-schedule.json`: the same 15 controller events, with
  thresholds derived from the existing actual capture.
- `controller-logical-schedule.provenance.json`: baseline/schedule hashes and
  the exact preceding sample used for every threshold.
- `analyze_takeover-proposed.py`: can be run directly from this directory after
  root integrates the capture patch. `analyzer-logical-schedule.patch` is also
  provided if root wants to update the existing analyzer.
- `derive_schedule.py`, `test_logical_schedule.py`, and `test-result.json`:
  reproducible derivation and clearly labeled synthetic regression evidence.

Neither patch has been applied to the shared production files by this agent.

## Event semantics

Existing frame-only schedules retain their previous stable sort, pad callback
values, and event logs. They perform no additional memory reads.

A logical schedule may contain a neutral frame-zero setup followed by events
such as:

```json
{"observed_game_frame": 64, "port": 0, "buttons": ["left"]}
```

If any event uses `observed_game_frame`, events run in the listed order. Each
event has exactly one clock: `frame` or `observed_game_frame`. An event becomes
eligible when its clock reaches or exceeds its requested value, after all
preceding events have been applied. Multiple eligible events in one iteration
retain list order, just as simultaneous original frame events did.

Before each `retro_run`, the capture utility reads the existing
`wp1979_started` and `game_frame` watches. Logical events wait until
`wp1979_started == 1`; this prevents startup RAM contents from triggering an
event. Both watches must be unique four-byte reads and `--watch-every` must be
one. `run_capture.py` already supplies exactly these watches and cadence, so it
needs no change.

The only input mutation is the original Python `pad[port]` set assignment.
The unchanged `input_state` callback returns that pad state to libretro. The
counter is never assigned, incremented, or forced by this utility.

A logical event log retains both the requested threshold and actual reading:

```json
{
  "observed_game_frame": 64,
  "port": 0,
  "buttons": ["left"],
  "applied_at_frame": 540,
  "trigger_game_frame": 64
}
```

Here `trigger_game_frame` is the requested threshold;
`observed_game_frame` is the value actually read before that `retro_run`.
If observation jumps from 63 to 66, a threshold of 64 fires on 66 and records
both values honestly. The capture also records its own source SHA-256 and the
schedule SHA-256.

## Derivation from the actual baseline

Baseline: `wp1979/measure/candidate-2-controller/report.json`, SHA-256
`fe39b25622210f30929a4e53ca7efe074167a76f0bc05d439094ffbdaa231905`.

The source schedule's neutral frame-zero event stays unchanged. Every other
threshold is `report.samples[event.frame - 1].game_frame`; the derivation also
verifies that the original event occurred at the first eligible observation.

| Buttons | Original emulator frame | Previous observed game frame |
| --- | ---: | ---: |
| Left | 540 | 64 |
| Release | 588 | 111 |
| Up | 600 | 123 |
| Release | 636 | 159 |
| Right | 648 | 171 |
| Release | 696 | 218 |
| Down | 708 | 230 |
| Release | 744 | 266 |
| A | 768 | 290 |
| Release | 776 | 298 |
| B | 804 | 326 |
| Release | 812 | 334 |
| C | 840 | 362 |
| Release | 848 | 370 |

The original first event observes logical counter 64 before emulator frame
540; the native takeover is later observed at emulator frame 541/game frame
66. These are different observations. This proposal never substitutes the
native input frame 66 for the observed trigger 64.

## Actual capture and analysis for root

Integrate the capture patch first. Use the final current ELF/bin and its matching
CUE: `run_capture.py` verifies disc A.BIN against the linked binary. For example,
from `/workspace/scratch/1e822aeaeac7`, with root's final CUE path:

```bash
python wp1979/measure/run_capture.py final-controller-logical \
  --frames 1200 --video --actors --snapshot-every 30 --wall-timeout 300 \
  --core /workspace/scratch/1e822aeaeac7/wp1979/tooling/yabause-scsp-dsp/yabause/src/libretro/yabause_libretro.so \
  --schedule /workspace/scratch/1e822aeaeac7/wp1979/render_opt/takeover/logical_schedule/controller-logical-schedule.json \
  --cue /absolute/path/to/final/wrestlefest-wp1979.cue

python wp1979/render_opt/takeover/logical_schedule/analyze_takeover-proposed.py \
  wp1979/measure/final-controller-logical \
  --schedule /workspace/scratch/1e822aeaeac7/wp1979/render_opt/takeover/logical_schedule/controller-logical-schedule.json \
  --baseline-report wp1979/measure/candidate-2-controller/report.json
```

The 1,200-frame capture budget is a suggestion, not a trigger or a timing
guarantee. The candidate must reach at least baseline game frame 512 for the
full route comparison to pass. If a slower boot prevents that, the analyzer
reports missing coverage.

The analyzer reconstructs every event independently from post-run samples of
the **previous** emulator frame, checks that it fired on its first eligible
iteration, and checks the actual source and schedule hashes. It retains the
existing takeover, physical direction, decoded button, exact walking-step,
fatal-state, and neutral-release checks.

With `--baseline-report`, it additionally requires all 176 watched bytes of
actors 0, 1, 4, and 5 to match at every observed baseline game counter 0..512:
2,052 actor records and 361,152 bytes. Duplicate observations of one logical
counter must agree. Missing records, changing snapshots within one counter, or
different actor bytes fail the comparison, with the first differences reported.

Normal SMPC callback latency remains native behavior. Changing boot/CPU timing
could still change where an input is consumed. The actor comparison detects
that deviation; neither capture nor analyzer adjusts game state to conceal it.

## Verification performed

```bash
python wp1979/render_opt/takeover/logical_schedule/derive_schedule.py
python wp1979/render_opt/takeover/logical_schedule/test_logical_schedule.py
```

The test executes the actual proposed scheduler block and input callback via
Python AST extraction with read-only Python mocks. It does not load ctypes,
libretro, a core, or an emulated machine.

- Original and proposed frame-only paths produce identical 15 events and all
  1,000 frames of callbacks; the proposal adds zero reads on that path.
- Logical triggers replay the original baseline observation trace with exactly
  the original event frames and callbacks.
- A clearly synthetic resampling of the trace changes boot offset and adds
  stalls. LEFT moves from emulator frame 540 to 586 while observing logical
  counter 64. The analyzer passes the resampled fixture and exact actor bytes.
- A real scheduler-code mutation from `<=` to `<` delays events; the analyzer
  rejects it for missing the first eligible observation.
- A changed actor4 byte is rejected by the exact route comparison.
- Uninitialized startup counter contents do not trigger events before the
  started flag. Counter jumps preserve both threshold and actual observation.
- Synthetic fixture reports are rejected unless an internal test explicitly
  opts in; the command-line analyzer has no synthetic opt-in flag.

The fixture's actor records are copied/resampled from the existing trace solely
to test the parser/comparison. They are not a new capture or evidence that an
optimized Saturn build already preserved the route. Root must run the actual
final-image capture above.
