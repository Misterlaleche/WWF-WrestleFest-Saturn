# Audio 2 environment and native capture

The complete source-only installer is included in `environment/`. It regenerates the tools, wrappers, public dependencies, source trees, and build scripts under an explicit output directory. No BIOS, game ROM, compiled executable, or private SDK archive is included in that installer package.

## Reproduce

Use Ubuntu 24.04 x86_64 with Python 3.12+, gcc, g++, make, nm, curl, and dpkg-deb. From the `Audio 2` directory:

```bash
python production/tools/environment/restore_environment.py \
  --out /absolute/path/to/audio2-environment \
  --sdk-archive /absolute/path/to/13-sh2eb-elf-yaul-1-.zip --build
source /absolute/path/to/audio2-environment/env.sh
```

The output path must not contain whitespace, as required by the supplied Yaul SDK. Both arguments are mandatory. The supplied SDK archive is checked against SHA-256 `8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`. Every public package and source file is pinned in `environment/manifests/`. Use `--cache /path/to/downloads --offline` to reproduce without network access.

See `environment/README.md` for the complete instructions, generated file layout, runtime-helper licensing, and m68k layout extraction used by the native-state analyzer.

## Verified tools

| Component | Version |
|---|---|
| SH-2 GCC / binutils | 13.2.0 / 2.40 |
| m68k GCC / binutils | 13.3.0 / 2.42 |
| Standalone Mednafen | 1.32.1 |
| xorrisofs/xorriso | 1.5.6 |
| Xvfb | Ubuntu 21.1.12-1ubuntu1.9 |

Mednafen is built from the unmodified official release tarball, SHA-256 `de7eb94ab66212ae7758376524368a8ab208234b33796625ca630547dbc83832`. The Saturn module and debugger are enabled; unrelated emulation modules are omitted. No emulation source, clock, instruction scheduling, or sound-generation behavior is changed.

`environment/verification.json` records an independent restore into an empty output directory: 127 hash-verified public packages, two pinned sources, successful SH-2 and m68k compilation probes, a local Xvfb display, a successful native Mednafen rebuild, and an actual short native capture with a valid F5 state and 512 KiB SCSP RAM extraction. Build/debug paths can change executable hashes; each capture records the actual binary it ran.

The Xvfb wrapper relocates only its ancillary xkbcomp and keyboard-data lookup. Its small constructor is never preloaded into Mednafen. The managed execution environment requires an authorized unsandboxed call for X11 display sockets.

## Capture

```bash
python production/tools/capture_mednafen.py \
  --environment-root /absolute/path/to/audio2-environment \
  --cue /absolute/path/to/AUDIO2.cue \
  --bios /absolute/path/to/user-supplied/mpr-17933.bin \
  --out /absolute/path/to/new-capture \
  --seconds 60 --dump-wram
```

The runner verifies standalone Mednafen 1.32.1 before starting, records disc-image/executable/BIOS hashes, and preserves the real native MOV, WAV, F9 snapshots, F5 state, and read-only RAM dumps. It never loads a save state or writes emulator memory. The dummy host playback device leaves native audio generation and recording enabled. Native recordings include BIOS startup; measurements of application audio must account for that interval.

Optional `--watch NAME:ADDRESS:BYTES`, `--ready-watch`, `--ready-value`, `--stop-watch`, and `--stop-value` arguments support bounded diagnostic runs. Obtain addresses from each build's symbol manifest. Threshold polling and ordinary Pause input can finish a few emulated frames after a target; the report gives actual observed final values rather than claiming an exact stop frame.

`analyze_audio_capture.py` contains the native state parser and verifies a firmware prefix against SCSP RAM before decoding the actual firmware counters. Its required layout is compiled from the exact firmware header with the real m68k compiler; reproduction instructions and source are included in `environment/`.

## Baseline 68000 helpers

The installer fetches the unmodified official GCC 13.3.0 `lb1sf68.S` to `src/gcc-lb1sf68.S`, with SHA-256 `851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00`. Pass that generated path as the firmware builder's `--runtime-source`. Compile needed helpers with `-m68000 -msoft-float -fno-pic -fno-pie`; do not assume distribution libgcc defaults are baseline-68000-safe.

For the inspected firmware02 ELF, `__mulsi3` and `__udivsi3` are byte-identical to independently rebuilt baseline source objects; `__divsi3` is absent from that ELF. The independently built signed-division object uses absolute JSR opcode 4EB9. The exact firmware/source/object hashes are in `environment/verification-m68k-runtime.json`. GPLv3 and GCC Runtime Library Exception 3.1 texts are included in `environment/licenses/`.

## Diagnostic evidence available at handoff

The region-only probe03 changed exactly the ISO's eight region-header bytes to `JTUBKAEL`, retaining every other byte from the failing probe02, and entered the harness. The extended run reached finished state 4 with 2157 music events, error 0, actual 32-bit SFX underruns 0, and late counter 7.

The separately rebuilt probe04 used the client API and correct region header from its builder. It reached state 4 with APIstate 2/APIerror 0, 2157 music events, actual SFX underruns 0, late counter 7, and zero recorded batch/discard/rejection counters. Its native WAV contains 2,213,618 stereo frames at 44.1 kHz, or 50.1954195 seconds including startup. These are observations from this diagnostic command, not a fidelity, physical Saturn, or full-command-coverage claim.

Historical probe01–03 diagnostic word 9 came from reserved mailbox address C05A and was initially mislabeled as an underrun counter. Treat it as reserved; later harnesses expose the actual 32-bit firmware counter. Canonical-state analysis independently reads that counter using the precise firmware symbols and compiled layout.
