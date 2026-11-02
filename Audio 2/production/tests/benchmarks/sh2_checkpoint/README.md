# Frozen native SH2 checkpoint fixture

This directory contains the complete source fixture used to reproduce the
archived baseline and scalar9/sequence-`-Os` native benchmark. The manifest
pins 57 files, 335,062 bytes: the own Audio2 worker and master observers, the
audio-only game binding, required runtime sources and build tools. The
upstream Z80 license remains included. No ROM, BIOS, game source/assets,
firmware binary, PCM payload or precompiled object is bundled here.

These sources intentionally preserve legacy firmware10 and the pre-change
baseline. They are diagnostic inputs and are not selected by ordinary
production builds. Their known PCM and pitch failures remain limitations of
the archived benchmark; rebuilding them establishes reproducibility, not
audio acceptance.

## Rebuild from supplied inputs

From the workspace root, with the restored SDK and the supplied original
WP1979 native source tree, run:

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

Replace the game-source placeholder with the supplied tree, including its
`src/runtime/audit/runtime18-cd` assets. Outputs must be new private
directories. The generator checks the original tape, all frozen sources and
the pinned base-68000 helper source at `tools-env/src/gcc-lb1sf68.S`. It rebuilds
all required objects, firmware and master; it then derives observer addresses
and capture arguments from the new linked symbols. No archived private
fixture, absolute observer address, binary or object is needed.

Both commands build only. An explicit `--run` on the second command, with
`--environment-root`, `--bios` and an available `--display`, starts the same
standalone Mednafen 1.32.1 read-only benchmark. It uses normal presentation
speed and no cartridge. See `../../sh2-scalar-admissions.md` for the complete
measurement protocol, event digest, host semantic gates and native limits.

## Verified source reconstruction

The clean preflight regenerated the archived firmware10 and original master
byte for byte, then reproduced both measured packed modules. The identities
are pinned in `source-manifest.json` and checked by the tools:

| Artifact | SHA256 |
|---|---|
| Firmware10 | `b5ecee6e3e2a0efe7c13f1c6f4cb89bbc6b18cb96db23a57203b20e4a2b06b66` |
| Original master flat binary | `7166824557ed333934556c1dcab7e1db27b110d8ac8d6ad7fb3f7aaf5474caa0` |
| Baseline packed module | `2a2ae43dd9be511c746e9cb147e4eb7f464197b80916964d9d945da348e466b9` |
| Selected packed module | `bc5a5e0314f4e8a3fd39238fad3a7288fb5393c2de327b79bb3ac16b5dce32b0` |

All 345 supplied game files and all frozen source hashes remained unchanged.
No emulator was started for this preflight. New ISO timestamps and ELF debug
paths are not claimed to match. The public evidence summary is
`../../../evidence/sh2-checkpoint-source-preflight.json`; the successful
commands and logs remain in the private output manifests named there. The
first failed preflight is retained too: it had already reproduced the three
fixture binary identities, then stopped on an incorrect master-observer
symbol name. The corrected clean run used the actual linked symbol and
completed both stages.
