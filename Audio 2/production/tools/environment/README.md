# Reproduce the Audio 2 native tool environment

This directory contains source scripts, templates, pinned public package
manifests, and license texts. It contains no executables, BIOS, game ROMs,
or private toolchain archive.

## Host prerequisites

Use Ubuntu 24.04 on x86_64, with Python 3.12 or newer, a C/C++ build toolchain
(`gcc`, `g++`, `make`, `nm`), `curl`, and `dpkg-deb`. The verified native
compiler is GCC 13.3.0. The installer unpacks the pinned public SDL, FLAC,
X11/Xvfb, xkb-data, m68k GCC/binutils, ISO tools, and their runtime dependency
closure into the requested output directory. It does not install system
packages or change files outside that directory.

## One command

From the `Audio 2` directory:

```bash
python production/tools/environment/restore_environment.py \
  --out /absolute/path/to/audio2-environment \
  --sdk-archive /absolute/path/to/13-sh2eb-elf-yaul-1-.zip \
  --build
```

`--out` and `--sdk-archive` are required. The output path must not contain whitespace because the supplied Yaul SDK rejects such paths. The SH-2 SDK must be the supplied
archive with SHA-256
`8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`.
Every public download is pinned by filename and SHA-256 in `manifests/`.
No BIOS or game data is fetched.

The generated environment contains all necessary environment-specific
scripts: `env.sh`, `build-mednafen.sh`, `bin/Xvfb`, `bin/mednafen`, the small
Xvfb path-relocation source/library, cross-tool wrappers, source trees,
license texts, package archives, and build logs.

To reuse previously downloaded package/source archives, add
`--cache /path/to/downloads`. Add `--offline` to prohibit network access.
Omit `--build` to restore dependencies first, then run the generated
`build-mednafen.sh` explicitly.

## Use the tools and capture

```bash
source /absolute/path/to/audio2-environment/env.sh
python production/tools/capture_mednafen.py \
  --environment-root /absolute/path/to/audio2-environment \
  --cue /absolute/path/to/AUDIO2.cue \
  --bios /absolute/path/to/user-supplied/mpr-17933.bin \
  --out /absolute/path/to/new-capture \
  --seconds 60 --dump-wram
```

Native capture uses the unmodified standalone Mednafen 1.32.1 source release,
with the Saturn module and debugger enabled. The Xvfb wrapper relocates only
its ancillary keymap compiler/data lookup; it never preloads anything into
Mednafen. This execution environment needs an authorized unsandboxed call
for local X11 sockets. BIOS and disc files are supplied separately.

Cross-compiler checks and source/package hashes are recorded in
`logs/restore-report.json`; Mednafen compilation and version logs are under
`logs/mednafen-*.log`. Compiler/debug path differences can change the native
executable hash across output locations; each actual capture records its
own executable and disc-image hashes.

## Baseline 68000 runtime helpers

The installer fetches the unmodified GCC 13.3.0 `lb1sf68.S` to
`src/gcc-lb1sf68.S`. Its pinned SHA-256 is
`851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00`.
Pass that path to the firmware builder's `--runtime-source` argument.

The source carries GPLv3-or-later plus GCC Runtime Library Exception 3.1.
Both official texts are included in `licenses/`. Source and license URLs
are the official GCC release mirror. The firmware build must explicitly
use `-m68000 -msoft-float -fno-pic -fno-pie` when compiling the needed helpers;
distribution libgcc defaults are not a baseline-68000 compatibility proof.

## Recreate the state-analysis layout

The native state analysis helper requires a layout produced by the actual
m68k compiler for the exact firmware header. From the `Audio 2` directory:

```bash
source /absolute/path/to/audio2-environment/env.sh
m68k-linux-gnu-gcc -m68000 -msoft-float -ffreestanding -fno-pic -fno-pie \
  -I production/src -c production/tools/environment/src/sfx-layout.c \
  -o /absolute/path/to/audio2-environment/verification/sfx-layout.o
m68k-linux-gnu-objcopy -O binary -j .rodata \
  /absolute/path/to/audio2-environment/verification/sfx-layout.o \
  /absolute/path/to/audio2-environment/verification/sfx-layout.bin
python production/tools/analyze_audio_capture.py \
  --capture /absolute/path/to/capture \
  --firmware /absolute/path/to/exact/SCSP.ELF \
  --symbols /absolute/path/to/exact/symbols.json \
  --layout /absolute/path/to/audio2-environment/verification/sfx-layout.bin
```

The helper verifies a firmware prefix against the captured SCSP RAM before
labeling the firmware's counters. Rebuild the layout after changing the
firmware structures; preserve the matching firmware and symbol manifest.
