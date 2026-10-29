# Native R18 audio build

These outputs use the existing R18 native music and sound effect implementation. Production C and assembly are unchanged. Assets come from the user-supplied `project_sources/01-wwfwfest.zip`; no external game data or BIOS was downloaded.

## Disc assets

| File | Bytes | SHA-256 |
|---|---:|---|
| `music-driver/SCSP.ELF` | 53,404 | `bdf84aa656a0a1b03ac6a9a6f11a7d125acbf619f6f72ebf6c53e74e3ece83b4` |
| `music-module/MUSIC.ELF` | 128,316 | `eb3ac0a870a74d996eafec92eccaaa9450dbf78bc3c25dd9c824f11b270621fe` |
| `pack/SFX.DAT` | 440,710 | `dbe749cf4ce13f333ff7318a74a64e30fa2b9af05d953688a0813311cbabd216` |

Copy those three assets to the root of the disc's ISO staging directory. The existing `wf_r18_scsp.c` loads them by these exact uppercase names.

## Rebuild

From `/workspace/scratch/1e822aeaeac7`:

```sh
source wp1979/tooling/env.sh
python3 wp1979/tooling/audio/build_audio_linux.py
python3 wp1979/tooling/audio/reference/tools/audio/native_probe/build_music_module.py \
  --repo wp1979/native \
  --toolchain wp1979/tooling/audio/sh-compat \
  --sdk wp1979/tooling/sh2eb-elf \
  --rom wp1979/tooling/audio/rom/31a11-2.ic42 \
  --out wp1979/tooling/audio/music-module
python3 wp1979/native/tools/audio/native_probe/build_sfx_pack.py \
  --rom wp1979/tooling/audio/rom/31j10.ic73 \
  --manifest wp1979/tooling/audio/ROM_OKI_MANIFEST.csv \
  --model wp1979/source_wip160/wf_audio_sfx.c \
  --out wp1979/tooling/audio/pack
```

The full SCSP firmware has 32 FM operator slots and four decoded PCM voices, with the existing 108-step SCSP DSP mixer. The upstream music build first creates an unused legacy FM diagnostic to obtain its OKI lookup table. `build_audio_linux.py` emits that exact table directly and retains the same production translation units, DSP generator, waveform and linker script. It compiles with Ubuntu GCC 13.3.0 targeting baseline 68000, freestanding and without PIC/PIE.

Ubuntu's prebuilt m68k libgcc `__divsi3` uses a 68020 long BSR. Therefore the three required runtime helpers (`mulsi3`, `udivsi3`, `divsi3`) are rebuilt from the unmodified official GCC 13.3.0 `libgcc/config/m68k/lb1sf68.S` with `-m68000 -fno-pic -fno-pie`. The resulting call is a baseline 68000 JSR. The source URL and SHA-256 are recorded in `music-driver/music-build-report.json`.

The MUSIC module uses the supplied GCC 13.2.0 SH2 compiler and the unchanged upstream build/generator scripts. The `sh-compat` directory supplies the Windows-style executable names expected by those scripts as symlinks to the actual Linux tools. It does not alter the compiler target. The generator produces 128 native control blocks containing 437 instructions. The module reserves the existing VDP1 tail region `0x05c5c000` through `0x05c80000`.

The OKI manifest lists the 73 bank-0 and 41 bank-1 sample entries directly from the two 0x40000-byte banks' eight-byte ROM table entries: the first three bytes give the start and the next three the inclusive end. The pack builder checks all spans and SHA-1 values. The result has 114 entries, 95 distinct encoded samples, 98 command definitions and 165 events. The command model is from the supplied WIP160 source. The pack occupies 440,710 of the 442,368-byte resident allocation.

## Validation

Both firmware objects pass the actual production ELF loaders compiled for the host, including all address, alignment, segment, vector, ABI and header checks:

```sh
gcc -std=c11 -O2 -ffunction-sections -fdata-sections \
  -I wp1979/native/src/audio/native \
  wp1979/tooling/audio/validate_audio_images.c \
  wp1979/native/src/audio/native/wf_native_elf.c \
  wp1979/native/src/audio/native/wf_native_host.c \
  wp1979/native/src/audio/native/wf_music_elf.c \
  -Wl,--gc-sections -o wp1979/tooling/audio/validate_audio_images
wp1979/tooling/audio/validate_audio_images \
  wp1979/tooling/audio/music-driver/SCSP.ELF \
  wp1979/tooling/audio/music-module/MUSIC.ELF
```

SCSP load regions:

| Address | File bytes | Memory bytes | End |
|---|---:|---:|---|
| `0x00000000` | 11,284 | 11,284 | `0x00002c14` |
| `0x0000c100` | 13,632 | 14,976 | `0x0000fb80` |
| `0x00010000` | 9,216 | 9,216 | `0x00012400` |

The build checks undefined symbols, baseline 68000 disassembly and load-region sizes. Per-command compiler logs, symbols, maps, ELF reports and source hashes are retained in the output directories. This establishes build and loader compatibility. Audio playback and its timing still need verification in the final running disc; these checks alone are not a physical hardware result.
