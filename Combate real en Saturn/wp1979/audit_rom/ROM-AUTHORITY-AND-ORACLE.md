# wp1979 original-ROM authority and live MAME oracle

## Verified result

The supplied World program reconstructs to SHA1
`a726c46f350723ea73158bc78055d17d418b14e2`.
The animation, animation-interpreter, physics, actor scheduler, roster, TAG,
prematch and attract audit tools all pass against that exact program. Their
stdout is preserved in `verified/`.

A live MAME 0.264 run completed **1,040 consecutive original gameplay updates**
with Hulk Hogan and Ultimate Warrior versus Ted DiBiase and Mr. Perfect.
`oracle_verified/actors.csv` contains all four actors on every update, for
4,160 records. A separate observer of the original 68000 movement routine
recorded **634 integrations, with zero fixed-point delta mismatches**.
`oracle_verified/validation.json` independently verifies the frame sequence,
identities, measured deltas, snapshot dimensions and binary sizes, and includes
SHA256 digests of the evidence files.

This is a **controlled original-program test fixture**: the normal attract-state-2
lineup is `[0,1,6,7]`; a read tap supplies byte `9` instead of byte `6` at ROM
address `0xF48` before original actor and palette initialization. The two original
reads of that byte are logged. No gameplay instruction, input port, actor
callback, physics operation, animation timer, CPU decision or RNG operation is
replaced. This is not an unmodified attract recording and is not a paid-match
initialization. Attract state 2 itself gives Hogan stamina `0x7000` and the other
three wrestlers `0x003C`; those original attract values are retained. A native
paid match should use its original paid setup, not import this attract health.

## Identity and exact ordinary movement

| Character | Actor identity (`+02` and `+57`) | Walk magnitude | Run magnitude | Walk X/tick | Walk Y/tick | Run X/tick | Run Y/tick |
|---|---:|---:|---:|---:|---:|---:|---:|
| Hulk Hogan | 0 | 27 | 47 | 2.109375 | 1.6875 | 3.671875 | 2.9375 |
| Ultimate Warrior | 1 | 29 | 53 | 2.265625 | 1.8125 | 4.140625 | 3.3125 |
| Ted DiBiase | 9 | 27 | 47 | 2.109375 | 1.6875 | 3.671875 | 2.9375 |
| Mr. Perfect | 7 | 30 | 56 | 2.34375 | 1.875 | 4.375 | 3.5 |

These are cardinal, unmodified world-coordinate deltas in pixels per original
logic update. The tables are program bytes at `0x116AE` (walk) and `0x11D4A`
(run). `build_reference.py` reconstructs the program, validates its SHA1 and
extracts these values into `wp1979-rom-reference.json`.

The actual mode-1 integrator at `0x222C` reads magnitude `+2B` and angle `+2D`.
The X and Y positions are 16.16 LONGs at `+06` and `+0A`. The quarter-wave
coefficient tables at `0x2378` and `0x23FA` are used directly, with quadrant
reflection/sign changes. Cardinal horizontal delta is `magnitude * 0x1400` and
cardinal depth delta is `magnitude * 0x1000`, each added directly to 16.16
position. The distinct X/Y coefficients are intentional. Diagonals use the
actual table entries; no normalized trigonometry is equivalent.

The live trace includes these independently observed cardinal cases:

| Case | Original-ROM observations | Absolute X delta, 16.16 |
|---|---:|---:|
| Hogan walking | 77 | 138240 |
| Hogan running | 181 | 240640 |
| Warrior walking | 31 | 148480 |
| DiBiase walking | 14 | 138240 |
| DiBiase running | 160 | 240640 |
| Perfect walking | 22 | 153600 |

The same trace also covers intermediate headings, alternate speeds, action
motion, rebound and braking. Warrior/Perfect run speeds are proven by the ROM
tables and static/native audits; this particular live fixture did not happen to
produce their running cardinal samples.

### Original modifiers remain part of the behavior

At `0x1174C`, `+32.bit3` selects the alternate walk table, which is 12 for all
characters. If `+56.bit7` and `+33.bit5` are both set, the low-byte magnitude
receives `+6`. If `+74.bit7` is set, the magnitude WORD receives `-8`. The mode-1
consumer masks the result to its low byte. State 3 braking subtracts WORD 5 per
callback and exits only on the original negative-result branch. Those predicates
must be preserved when comparing actual CPU/player speeds.

## Cadence and animation

The ROM proves one gameplay iteration per frame-ready/VBlank-acknowledgment
envelope, starting at `0xF9A`. Ready and acknowledgment are bits 0 and 1 of
`0x1C006F`; `0x1C0080` increments once after acknowledgment. It does not encode a
physical refresh frequency.

The MAME configuration uses a 7 MHz pixel clock, 448 total horizontal clocks,
272 total scanlines, and visible rectangle X 0–319, Y 8–247. Thus its reference
cadence is `7,000,000 / (448*272) = 57.444852941176... Hz`. The live 0.264 Lua
API reported period `0.017407999999983` seconds, or `57.444852941234 Hz` at the
API's precision. The MAME source explicitly labels HTOTAL/VTOTAL as **guessed**;
this is an emulator reference, not a measured original-PCB frequency. Current
primary source is preserved in `mame_ddragon3.cpp` (Git blob
`6c307aeb136b21fe152e706eab2a60bdcb3d3415`).

The common animation interpreter is `0x1C0C8`:

- Stored finite timer `N` means **N+1 interpreter ticks**.
- `0xFF00` is an indefinite callback-controlled hold, not 65,280 ticks.
- Descriptor kind 2 restarts at entry 0 when the schedule ends.
- Walk descriptor `0x115F2`, callback `0x11652`: poses 1,2,3,4, each 11 ticks;
  complete cycle 44 ticks.
- Run descriptor `0x11C62`, callback `0x11C82`: poses 9,10,11,12,13,14, each
  7 ticks; complete cycle 42 ticks.

A 1:1 native Saturn update must retain these motion and animation constants.
Renderer cost must not reduce the number of logic updates. Any higher display
rate must be described relative to the emulator reference above, unless an
independent PCB cadence measurement is supplied.

## Actor layout and scheduling

There are nine wrestler slots beginning at `0x1C05B0`, stride `0x10C`.
The referee is separate, at `0x1C11F4`. TAG partners use the LONG at `+86`,
with reciprocal pairs 0↔1 and 2↔3. Legal-role ownership is `+33.bit0`;
the reserve/non-legal gate is `+32.bit0`.

Important fields are exported as numeric offsets in the JSON reference.
The raw actor memory should retain byte/word width semantics: high-WORD
corrections to X/Y/Z preserve the fractional low word.

The authoritative actor-turn ordering is:

1. Clear contact bytes `+36/+37`.
2. Resolve contacts at `0x280DC` and apply high-WORD corrections.
3. Run the selected state/action callback at `0x1C03E`.
4. Advance the selected animation at `0x1C0C8`.
5. Project coordinates at `0x247C` and queue the object at `0x27B8`.
6. Integrate motion at `0x2208`.

The motion produced by a callback is integrated in that actor's same turn, but
the turn's already-queued projection precedes that integration. A visual sample
and a post-integration world sample therefore differ by this deliberate pipeline
position. Collision sees the integrated world coordinates. The later `0x24090`
pass invalidates changed collision profiles; branch A then performs collision
`0x24062` and damage `0x24E58`. Branch B omits those latter two calls.

Actor slots are processed in ascending order with a live active set. Mutating a
later linked slot can affect its callback in the same pass; mutating an earlier
slot waits for its next callback. A snapshot-update-commit replacement changes
this behavior. CPU decisions precede callbacks; the referee also updates before
the normal wrestler callback pass.

## Using the live evidence

`oracle_verified/` contains:

- `summary.json`: oracle version, program identity, fixture alteration and totals.
- `validation.json`: independent audit and file hashes.
- `actors.csv`: all four actor states on every original completed logic frame.
- `motion.csv`: original mode-1 operand/write observations with expected and
  observed fixed-point deltas.
- `events.log` and `capture.log`: original startup, frame-entry and capture log.
- Nine `frame_NNNN.png` native 320×240 screenshots, plus corresponding raw
  work RAM, sprite RAM, foreground/background/text maps, palette and scroll.

Captures are taken at the original explicit sprite-buffer write to `0x140008`.
The sprite/list snapshot is therefore associated with a real original display
commit. PNG capture follows in MAME's frame callback. Raw sprite RAM uses hardware
raster coordinates; its Y origin differs from the cropped PNG by 8 scanlines.
Apply that crop when feeding raw captured MMIO to a visible-raster comparison,
and do not crop assets that have already been converted to canonical 320×240.

The trace is an independent oracle for native behavior. Replaying it would
validate rendering throughput but would not establish native interactive
gameplay. The intended Saturn runtime should execute its native callbacks,
state transitions, collision and player input, with this capture used as
comparison evidence.

## Reproduction

With the supplied MAME toolchain available:

```bash
bash wp1979/audit_rom/run_oracle.sh
python wp1979/audit_rom/analyze_oracle.py wp1979/audit_rom/oracle
python wp1979/audit_rom/build_reference.py project_sources/01-wwfwfest.zip wp1979/audit_rom/wp1979-rom-reference.json
```

`WF_MAME` may select another binary. `WF_ORACLE_OUT` selects a distinct output
directory. `WF_ORACLE_FIXTURE=original` captures the untouched state-2 lineup
instead. Keep output directories distinct when running multiple MAME processes.
MAME 0.264 reports the PC as `CURPC`/`rPC`, frame number as a method, and frame
period as a numeric seconds value; the scripts accommodate those API details.

The launcher defaults to the complete executable in `/tmp/wp1979-mame-package`;
the large workspace copy had been truncated during synchronization and cannot
be assumed executable. Re-extraction or a valid `WF_MAME` path resolves that
tooling issue without changing any game behavior.
