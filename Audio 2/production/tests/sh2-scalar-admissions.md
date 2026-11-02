# Exact scalar admissions and selected SH2 compilation

The selected change admits nine existing original single-instruction helpers in
the native fast dispatchers: `01`, `04`, `0E`, `38`, `56`, `82`, `90`, `BA` and
`C6`. It compiles `wf_music_seq.c` with `-Os` while keeping `z80-native.c` at
`-O2`. The instruction bodies, cycle table, interrupt tail, I/O ordering and
service bound remain unchanged. Static, dynamic and sequence-test build tools
use the same choices.

This is a measured partial performance improvement. It does **not** resolve the
native `1F` transition failure or establish PCM continuity, waveform fidelity,
final game cadence or Saturn hardware acceptance. No extra block lift or 24 KiB
memory profile was selected.

## Independent semantic gate

Run from the workspace root with the legally supplied complete 64 KiB sound
program and a fresh private output directory:

```bash
python3 "audio2-repo/Audio 2/production/tools/verify_sh2_scalar_admissions.py" \
  --rom audio2-build/sh2-cost-profile/fade-host-profile/original-sound.rom \
  --out audio2-build/sh2-scalar-review \
  --seconds 600
```

The tool validates the complete ROM SHA256
`f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7`
and refuses a changed pre-change oracle. It creates a full command/output report
in `verification.json`; it does not edit production, the original ROM or the
frozen R01 source.

The RED executable uses `tests/oracle/z80_native_before_scalar9.c` and must fail
the specific assertion `RED opcode01 was not admitted, count=0`, returning 2.
An arbitrary compile error or another runtime failure does not count as RED.
The GREEN executable must finish all **1,245,190** comparisons against the
unchanged original scalar Z80 implementation. These compare CPU state, flags,
cycles, refresh register, RAM and ordered I/O, including guarded rejection,
odd/even and boundary PCs, and all 65,536 load addresses.

The second stage compiles the selected sequence object at `-Os` and compares
the serviced candidate with frozen R01 plus the original full scalar Z80. The
completed 600-second run covers all 18 original music commands and another
8 seconds of fade for each: **10,962** full CPU/RAM/timer/event checkpoints,
956,109 calls and 3,151,318 cooperative service callbacks. It then checks live
commands `01,06,12,1F,03,1F,02,01,1F`, including command `00` rejection. The
recorded transition event count is 89,011 and its digest is
`e4e46eb2779d4f80`. The report compares source hashes again after execution.

The completed selected-source run is preserved privately at
`audio2-build/sh2-cost-profile/selected-production-gates/verification.json`.
These are host semantic tests, not a host substitute for a Saturn benchmark.

## Native benchmark and reproduction

The source package now includes the complete frozen diagnostic fixture in
`tests/benchmarks/sh2_checkpoint/`: the 32-counter worker observer, master
observer, audio-only binding and build tools. Its manifest pins 57 source files
totalling 335,062 bytes. It includes no ROM, BIOS, game source/assets, firmware
binary or precompiled object. This legacy benchmark fixture is separate from
the selected production runtime.

`tools/build_sh2_checkpoint_fixture.py` reconstructs the fixture from those
published sources, the supplied ROM ZIP, the restored SDK and the original
WP1979 native source tree. The game tree must include
`src/runtime/audit/runtime18-cd` and the original TAG input tape. Both output
directories below must be fresh and outside the package, game and SDK trees;
replace `/path/to/wp1979/native` with the supplied source location.

```bash
source tools-env/env.sh
python3 "audio2-repo/Audio 2/production/tools/build_sh2_checkpoint_fixture.py" \
  --rom-zip project_sources/01-wwfwfest.zip \
  --game-source /path/to/wp1979/native \
  --sdk tools-env/sh2eb-elf \
  --out audio2-build/sh2-fixture-review
python3 "audio2-repo/Audio 2/production/tools/reproduce_sh2_checkpoint.py" \
  --fixture audio2-build/sh2-fixture-review/fixture.json \
  --sdk tools-env/sh2eb-elf \
  --out audio2-build/sh2-native-review
```

Both commands build only. They require no archived private build or object.
The generator reconstructs legacy firmware10 and the original master, checks
their exact hashes, and generates the capture arguments from the freshly
linked symbols. It requires the pinned base-68000 GCC helper source restored
at `tools-env/src/gcc-lb1sf68.S`; `--runtime-source` can select another path to
the same file. The second command rebuilds both modules and rejects any
payload differing from the actually measured hashes below. The observer,
original game/tape, master and legacy firmware remain the same for both
variants. The old `--profiled-base` plus `--game-build` interface remains
available for existing archives, but is no longer required.

To execute the actual Saturn comparison, add `--run` to the second command,
`--environment-root tools-env`, the original BIOS via `--bios`, and a free
display via `--display`. The tool uses standalone Mednafen 1.32.1, normal
presentation speed and no expansion cartridge. It never writes emulator
memory. Native MOV/WAV/F5, input/source hashes and the complete commands remain
in the private output.

The native observer records the first 65,536 original musical/IRQ events,
including each 64-bit cycle in its digest. Original R01 yields digest
`4d3ef2a3` and final event cycle 108,786,762. Every measured variant matches
that count, digest and cycle. This is a native digest checkpoint, **not** an
exhaustive literal native-event trace; the independent host tests above provide
full state/event comparisons. Measurement stops after the whole sequencer call
containing the checkpoint, and each result is normalized by its actual virtual
duration.

| Variant | Inclusive sequence seconds per virtual second | Change |
|---|---:|---:|
| Original source and flags | 0.851441065 | baseline |
| Nine scalar admissions plus sequence `-Os` | 0.805403475 | −5.4070% |

The timed sequence call includes the interpreter, YM/OKI callbacks, FM service
and bus/memory stalls. This is not a pure Z80 ALU measurement. Clock conversion
comes from the native F5 state: SMPC divisor 65 and FRT divisor 128, giving
`1746818182 / 65 / 128` timer ticks per second. All measured checkpoint runs
report API/worker error 0; legacy firmware10's known PCM/pitch faults prevent
any audio-acceptance claim from this fixture.

The completed clean source-generation preflight rebuilt the firmware, master
and both modules without starting the emulator or reusing archived objects.
The packed module bytes exactly match the originally measured modules:

| Module | SHA256 | Hot code/state |
|---|---|---:|
| Baseline observer | `2a2ae43dd9be511c746e9cb147e4eb7f464197b80916964d9d945da348e466b9` | 18,096 / 18,432 B |
| Selected observer | `bc5a5e0314f4e8a3fd39238fad3a7288fb5393c2de327b79bb3ac16b5dce32b0` | 17,944 / 18,432 B |

Firmware10 SHA256 is
`b5ecee6e3e2a0efe7c13f1c6f4cb89bbc6b18cb96db23a57203b20e4a2b06b66`;
the original master flat binary SHA256 is
`7166824557ed333934556c1dcab7e1db27b110d8ac8d6ad7fb3f7aaf5474caa0`.
All 345 original game files were unchanged and the frozen source manifest
remained stable. ISO timestamps and ELF debug paths can vary; their complete
file identity is not asserted. The machine-readable source-only proof is
`evidence/sh2-checkpoint-source-preflight.json`. Complete private commands and
logs are retained in
`audio2-build/sh2-cost-profile/source-only-fixture-preflight02/fixture.json`
and `audio2-build/sh2-cost-profile/source-only-checkpoint-preflight/checkpoint.json`.
The earlier archived-directory preflight is retained separately. This closes
the fixture reconstruction gap without adding a new native timing result.

## Selection limits

The selected source SHA256 is
`c408e4a8ad5d7b54a55706f4c96fd9f4d39271dcd0fb5e2de14331af138f4fad`;
the exact pre-change oracle is
`983bde9378737eeb6bdcf0ed9ff15a5b535ad55ed61ea87a84516b1613b04939`.
`evidence/sh2-scalar9-selection.json` contains all variant results, identities
and known failures.

An aligned 16-bit load experiment changed measured cost by only −0.020%; native
core `-Os` regressed by 2.770%; sequence `-O3` exceeded the unchanged reservation.
None was adopted. Extra exact fade/block lifts passed host tests but still
failed the native transition fixture and lacked a matched improvement measure.
The authorized private 24 KiB experiment was never executed or adopted.

With the prior work128 firmware, selected scalar9/sequence `-Os` still faults
after the first `1F`: observed worker error 15 at 9.327 seconds, 2 late events,
1 discarded FM event/batch, 0 PCM underruns and 4 of 9 commands accepted. The
source-correct fade performs real work and cannot be treated as another empty
idle loop. Final firmware and full-combat results must be assessed separately.
