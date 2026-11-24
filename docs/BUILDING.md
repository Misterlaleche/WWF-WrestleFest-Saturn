# Building P1-03B

[Español](BUILDING.es.md) · [Project overview](../README.md)

These instructions describe the current source checkout. They replace the R03-only recovery instructions that were inherited by later source ZIPs. They do not claim a new SH-2 build or physical-console validation during the repository refresh.

## 1. Required inputs

Use GNU/Linux x86-64 with GNU Make 4.2 or newer, Python 3, a host C/C++ compiler and `genisoimage` (or compatible `mkisofs`). The documented target compiler is the SH-2 GCC 8.2 from the pinned Jo Engine 2026.3 archive. The bundled Linux compiler is not a native macOS executable. Other platform branches exist in the makefile, but they have not been revalidated by this update.

| External input | Identity / requirement |
| --- | --- |
| `joengine-2026.3.zip` | SHA-256 `341209b0216f2e2247f26946a192cfe295d69a38df99c6639c86ee524eed5383` |
| Prepared game assets | All 34 files must match the names, sizes and SHA-256 hashes in [`asset-lock.json`](../asset-lock.json) |
| Asset recovery source | A matching P1-03/P1-03B BIN or ISO data image; the extractor checks each required payload |

The SDK ZIP is 141,364,905 bytes in the recorded toolchain. A newer or differently packaged SDK with the same filename is not the pinned input. Compiler executables, the matching disc and protected game assets must be supplied separately.

R03 and FIX2R music files do **not** satisfy the current P1-03 audio hashes. Do not replace the lock file to make an older disc pass. `source_assets/` is not included; the original ROM alone is insufficient to regenerate every current prepared resource using this checkout.

## 2. Obtain the source and install the compiler

```sh
git clone https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private.git
cd WWF-WrestleFest-Saturn-Private
python3 tools/setup_compiler.py /path/to/joengine-2026.3.zip
```

Repository access is required while the repository is private. The installer verifies the complete SDK ZIP before extracting `sdk/Compiler/LINUX/` and `sdk/Compiler/WINDOWS/`. It refuses to overwrite existing compiler directories.

Keep the tracked `sdk/jo_engine/` and `sdk/Compiler/COMMON/` exactly as supplied by this project. COMMON includes the adapted make rules, linker script and 6,144-byte Jo `IP.BIN` boot template. That template is not a Saturn BIOS. [Provenance](PROVENANCE.md) records its restoration from the verified R03 package.

## 3. Restore and check the data files

Run from the repository root, replacing the example path with your matching disc:

```sh
python3 tools/extract_disc_assets.py /path/to/WrestleFest_Saturn_P1_03B_BOOTFIX.bin --output-dir cd
python3 tools/check_raw_disc_assets.py --cd cd --manifest asset-lock.json
python3 tools/generate_music_headers.py --verify --cd cd
```

The extractor accepts ISO2048 and single-track MODE1/2352 data images starting at LBA 0. It validates the selected entries against the lock before replacing destination files. It deliberately does not import `0.BIN`: the executable is built from source. A BIOS is unnecessary for extraction or compilation.

Alternatively, place already prepared matching files in `cd/` and run the final two commands. The 34 locked files total **632,644,966 bytes**. Allow additional disk space for the ISO, raw-sector BIN and compiler.

The current WAV headers and music metadata are matched to those exact assets. Verify them; do not regenerate metadata from an arbitrary older soundtrack to bypass a mismatch.

## 4. Build the normal frontend

```sh
make -j2 WF_FRONTEND=1 WF_AUTORUN= WF_CAPTURE=0 WF_PROFILE=0 MKISOFS=genisoimage
make sh2-memory-margin WF_FRONTEND=1 WF_AUTORUN= WF_CAPTURE=0 WF_PROFILE=0 MKISOFS=genisoimage
```

Use the same flags for the build and memory check. `WF_AUTORUN=` leaves automatic test input disabled. `WF_CAPTURE=0` and `WF_PROFILE=0` disable the capture/profiling variants. Omitting `WF_FRONTEND=1` selects a diagnostic entry point rather than the original frontend route.

Expected outputs are `game.elf`, `game.raw`, `game.map`, `game.iso`, `game.bin` and `game.cue`. Disc packaging runs the asset and music-header checks. The separate memory gate requires at least **8,192 bytes** before the reserved `0x060DC000` boundary. The recovered P1-03B build recorded 8,208 bytes, so small code changes can matter.

Preserve these build constraints:

- `-fno-peephole2` at compile and link, for the recorded SH-2 GCC code-generation issue.
- The adapted linker, heap/stack and audio-memory reservations.
- The selected cold-object `-Os` policy; **Adler-32 must remain outside that list** for this candidate.
- The current LTO configuration and dual-buffer/video ownership contract.

The original build's legacy disc-header version/date are separate from the P1-03B source checkpoint identity. See [provenance](PROVENANCE.md) for the original executable hash. An ISO/BIN hash may also change with packaging timestamps; a successful source build does not by itself prove byte-identical disc reproduction.

## 5. Tests and hardware

Several host checks work without game assets:

```sh
python3 tests/test_boot_asset_build_policy.py
python3 tests/test_jo_controls.py --sanitize
python3 tests/test_music_stream.py
python3 tests/test_music_deadline.py
python3 tests/test_save.py
python3 tests/test_save_adapter.py
```

The broader asset-dependent suite requires the restored `cd/`:

```sh
python3 tests/run_initial_load_tests.py --source . --assets cd --output build/initial-load-tests
```

That runner originated with R03. Its result covers only the checks it actually executes; it is not a replacement for the later P0/P1 regressions, full campaign comparisons, emulator runs or hardware acceptance. ROM-oracle tests additionally require `machine68k==0.5.1` and the appropriate original inputs.

For physical testing, put the BIN and CUE together, open the CUE in MODE, and use the required 4 MB RAM expansion. Record the first boot result before testing gameplay or audio. [STATUS.md](STATUS.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) describe the remaining acceptance work and useful reports.

## Optional asset generation and historical utilities

The generators are retained for development. Graphics generation has a pinned dependency in [`requirements-assets.txt`](../requirements-assets.txt); music generation uses the host Z80/YM/OKI reference and FFmpeg with libsoxr. Both need additional original inputs omitted from the source repository. The Z80/YM tools are host-side tools, not the current Saturn music engine.

`tools/verify_checkpoint.py` checks manifests from original R03-style frozen packages. Its historical `source-manifest.json` is not a manifest of this refreshed Git checkout. Use the current Git commit, [source provenance](PROVENANCE.md), and the included import record for this repository's identity.
