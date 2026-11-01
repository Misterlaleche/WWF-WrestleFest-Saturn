# Isolated native TAG integration

This directory binds Audio2 to the existing `wf_r18_scsp_*` audio hooks in a **new private copy** of the supplied WP1979 native game. It does not contain game sources, graphics, ROM data, BIOS data, firmware binaries or discs. Main, WP1979/80 and historical source exports remain unchanged.

**Status: integration path reproduced; audio acceptance blocked.** The frozen firmware10 full TAG comparison preserves the complete game state and original audio producer sequence, but records 19 PCM underruns, 22 late events and a 4.6344% increase in combat time. It also predates the pending ROM-authoritative pitch correction. See `../docs/COMBAT-INTEGRATION-GATE.md`. A successful build or deterministic replay does not imply accepted audio.

## Ownership contract

- The existing VDP1 audio region remains `0x05c5c000..0x05c80000`. Cold code and the 48 KiB sound program occupy the first segment; private BSS begins at `0x05c7c000`; the shared page begins at `0x05c7f000`, accessed uncached at `0x25c7f000`.
- The master linker owns a separate 18 KiB hot code/state window followed by an 8 KiB slave stack. `___end`, the SH2 slave stack symbol and the boot header agree. The reserve is excluded from the SDK user heap.
- The module uses **strict dynamic ABI4**. All four program headers, bounds, fixed windows, entrypoints and the master-owned hot address validate before any destination write. The original ABI2 loader and objects are excluded. The production static-link ABI3 header is not a dynamic loader contract.
- Boot reads the module and full SFX pack from CD into the future LWRAM tile cache. This is an ephemeral pregraphics lease. The game then owns that cache. The adapter supports cold boot only: calling recovery after the game reuses the cache would use invalid pointers and is prohibited.
- The slave SH2 belongs exclusively to the audio module while this profile runs. Audio service runs in the existing normal-context hook; heartbeat runs in the existing actual-VBlank hook.
- The original 64-word game producer queue preserves raw words, order and retries. Unknown commands are recorded and produce no invented sound. Any fault, unsupported producer, skipped word or overflow prevents acceptance.

The shared cadence observer is read-only with respect to the game. It samples the original logical frame counter at the original audio VBlank hook, stores the first combat baseline, and records every observed gap. It does not change scheduling, inputs, logic, drawing or animation. API `max_mail_latency_us` is quantized using nominal 16,667 microseconds per video field; use the native recording timestamps and cadence histogram for measured timing.

## Build a new matched pair

Use the supplied archive's `native` directory and already staged `native/src/runtime/audit/runtime18-cd` assets. The original sound ROM and full SFX pack are local prerequisites. Source the restored tool environment first. The following paths are examples; every output must be a new private directory.

```bash
source /absolute/path/to/tools-env/env.sh
python3 production/integration/build_combat_pair.py \
  --game-source /absolute/path/to/private-wp1979/native \
  --audio2 /absolute/path/to/production \
  --sdk /absolute/path/to/tools-env/sh2eb-elf \
  --resources /absolute/path/to/private-resources \
  --firmware /absolute/path/to/firmware/SCSP.ELF \
  --out /absolute/path/to/new-combat-build
```

`--resources` supplies `SOUND.ROM` and `SFX.DAT`; `--firmware` supplies the matching native three-segment SCSP firmware. The script verifies the supplied sound ROM SHA1 and fails linker bounds if the audio revision no longer fits. Builds run with one core. It records original game source hashes, audio source hashes, all commands, symbols, memory ownership and profile hashes.

The script compiles the gameplay once. `profiles/control/AUDIO2TAG.ELF` differs from the audio profile at exactly one data byte, changing `wf_audio2_enabled` from 1 to 0. Executable bytes, object layout, input tape, renderer, stage assets and both reserved audio windows are identical. The control bypasses audio boot/service and still records original producers/cadence. This measures the cost of enabling the candidate within the same integrated build, rather than comparing unrelated executable layouts.

`--resume` is restricted to an unaccepted private build and verifies the original copied game sources before continuing. Use a fresh output directory for a new accepted evidence revision; keep previous builds and captures.

## Capture and compare

Only the restored **standalone Mednafen 1.32.1** is supported. Its native capture tool sets `ss.cart none` and disables frame skipping. The existing WP1979 game supplies its original deterministic TAG input tape and stops at frame 7372; no debugger memory writes or external gameplay input are used.

```bash
python3 production/integration/capture_combat_pair.py \
  --build /absolute/path/to/new-combat-build \
  --game-layout /absolute/path/to/new-combat-build/game-layout.bin \
  --capture-tool /absolute/path/to/production/tools/capture_mednafen.py \
  --environment-root /absolute/path/to/tools-env \
  --bios /absolute/path/to/provided/mpr-17933.bin \
  --display 96 --seconds 210
```

Choose an unused display. The tool records profiles sequentially and refuses to overwrite existing captures. `--prepare-only` emits the exact command vectors without launching the emulator. `--seconds` is a wall timeout including BIOS/CD startup; it is never a gameplay timing measurement. A timeout is not a successful replay.

Compile the SFX observer layout against the **same frozen header** as the firmware. This records actual m68k ABI offsets, including the real 32-bit underrun counter; mailbox `C05A` is not that counter.

```bash
m68k-linux-gnu-gcc -m68000 -msoft-float -ffreestanding -fno-pic -fno-pie \
  -I /absolute/path/to/new-combat-build/audio2/src \
  -c production/tools/environment/src/sfx-layout.c \
  -o /absolute/path/to/new-combat-build/sfx-layout.o
m68k-linux-gnu-objcopy -j .rodata -O binary \
  /absolute/path/to/new-combat-build/sfx-layout.o \
  /absolute/path/to/new-combat-build/sfx-layout.bin
python3 production/integration/analyze_combat_pair.py \
  --build /absolute/path/to/new-combat-build \
  --game-layout /absolute/path/to/new-combat-build/game-layout.bin \
  --sfx-layout /absolute/path/to/new-combat-build/sfx-layout.bin \
  --firmware /absolute/path/to/firmware/SCSP.ELF \
  --firmware-symbols /absolute/path/to/firmware/symbols.json \
  --capture-analyzer /absolute/path/to/production/tools/analyze_audio_capture.py \
  --input-tape /absolute/path/to/new-combat-build/game/src/runtime/src/wp1979_tape.inc \
  --out /absolute/path/to/new-combat-build/comparison.json
```

The analyzer verifies endpoint/cadence consistency, complete game state and actor equality, the full producer trace, actual loaded module/firmware bytes, counters, stack canary and recording sample timestamps. It rejects missed logical observations rather than inventing precise percentiles. It labels the output as evidence, never `PASS` or a fidelity certificate. At least 1000 complete stable game video intervals are required for the timing report.

## Focused host checks

Build tests into a private scratch directory, keeping the repository source-only. These tests use the actual public Audio2 command/service implementation, the ABI4 validator and the cadence observer.

```bash
cc -std=c11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
  -I production/integration -I production/include -I production/src \
  production/integration/test_game_queue.c \
  production/integration/wf_audio2_game_queue.c production/src/wf_audio2.c \
  -Wl,--gc-sections -o /private/test_game_queue
cc -std=c11 -Wall -Wextra -Werror -I production/integration \
  production/integration/test_cadence.c -o /private/test_cadence
cc -std=c11 -Wall -Wextra -Werror -I production/integration -I production/src \
  production/integration/test_loader.c production/integration/wf_music_elf.c \
  -o /private/test_loader
/private/test_game_queue
/private/test_cadence
/private/test_loader
```

The WP1979 entry/tape described here is TAG-specific. Its normal frontend lacks a staged `ROYSPR.BIN`; its archived generator can reconstruct that file from existing sprite assets. A separate subsequently located WP1980 HW1 release supplies an original deterministic Royal fixture and8474-step endpoint; its integration and matched control are being prepared independently. The absence of a Royal tape in the WP1979 archive is not a global absence of a Royal fixture. Four active sprites alone are not a Royal gameplay comparison.
