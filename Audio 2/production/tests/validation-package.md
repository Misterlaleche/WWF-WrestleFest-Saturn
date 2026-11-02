# Private validation artifact packaging

`tools/package_validation_artifacts.py` creates a private ZIP directly from
explicitly selected original files, without copying directories or materializing
a second source tree. It does not launch an emulator or certify the candidate.
Every archive is marked **NOT PRODUCTION READY / HARDWARE PENDING**.

The local plan is JSON with `schema: 1`, optional descriptive `metadata`, and a
`files` list. Every entry must provide an existing absolute `source`, a relative
POSIX archive `member`, a `kind`, and the expected lowercase 64-character `sha256`.
The allowed kinds are `elf`, `disc` (ISO9660), `cue`, `state` (native Mednafen F5),
`ram` (named capture dumps), `layout` (the game/SFX offset records) and `text`
(reports, maps, configuration, commands and logs). A CUE must refer to an ISO
selected beside it in the archive. Keep original reports unchanged; record any
preserved-path relocation in `metadata` and select the actual preserved file.

The allowlist rejects original ROM/BIOS files, WAV/MOV and other media, objects,
source files and nested archives even when explicitly requested in the plan.
Executables, ISOs, RAM and F5 can themselves contain original game data: the ZIP
remains private and must never be committed or uploaded to GitHub.

Example invocation, from `production` with private input/output locations:

```bash
python3 -B tools/package_validation_artifacts.py \
  --plan /private/validation-artifacts-plan.json \
  --out /private/new-validation/artifacts \
  --archive-name AUDIO2-VALIDACION-ARTIFACTOS.zip
```

The output directory must be new and outside the source package/repository.
The default reserve is 2 GiB of available disk space in addition to the total
uncompressed selected inputs; no temporary copy of those inputs is created.
Do not reduce the reserve to conceal a full disk during an active capture.

The tool hashes every selected input before writing, hashes the bytes while
streaming them into the ZIP, closes the archive, verifies every member's CRC,
and independently verifies every SHA256 and the exact member set. It then
renames `.incomplete` to the final filename. A failed run retains its incomplete
archive and cannot be mistaken for a verified final ZIP. Existing outputs are
never replaced. `MANIFEST.json`, `SHA256SUMS` and `ARCHIVE-AUDIT.json` are also
written beside the ZIP. The audit includes the archive's own SHA256 and the
identity of every member, including both internal manifests.

For a large delivery, create separate artifact/report and F5 archives with
distinct names and fresh output directories. Retain one top-level delivery
manifest identifying both ZIP SHA256 values and both internal manifest hashes;
add companion-archive identities to the external audits after both close. This
avoids a circular requirement for one ZIP to contain the hash of the other.
Extract the two archives into separate folders to preserve both root manifests.
The source-to-member maps and the delivery manifest identify the matching F5.
WAV comparisons are delivered separately and remain necessary to rerun gates
that explicitly require waveform files. A report-only archive cannot replace
those gates or an actual Saturn reproduction.

Read-only verification, including CRC and all member digests:

```bash
python3 -B tools/package_validation_artifacts.py \
  --verify /private/new-validation/artifacts/AUDIO2-VALIDACION-ARTIFACTOS.zip
python3 -B tests/test_validation_package.py
```

The synthetic tests require no ROM, SDK, BIOS, private fixture or emulator. They
check exclusion, input identity, tampered archive data with a valid ZIP CRC,
destination preservation, safe member/download names and explicit selection.
The manifest records Python/zlib versions and deterministic ZIP member dates;
payload identity is verified. Byte-identical ZIP output across different Python
versions or relocated absolute provenance paths is not claimed.
