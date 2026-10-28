#!/usr/bin/env python3
"""Create a one-variable Saturn/MODE probe from the exact delivered R02 disc.

Only SCSP.ELF changes: the native music profile becomes the native SFX-only
profile. The original CDFS extent and file size stay identical, so A.BIN and
all frontend resources are byte-for-byte preserved. This is a diagnostic disc,
not a replacement for the intended music implementation.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from iso_to_bin_exact import sector
from verify_disc_exact import directory_files


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "entregas/WrestleFest-Saturn-frontend-r02-20261026/wrestlefest-runtime18.bin"
FIRMWARE = ROOT / "validation/local-20261026/audio-r02-firmware-underrun/legacy-profile/SCSP.ELF"
OUT = ROOT / "validation/local-20261026/audio-isolation-disc"
SOURCE_SHA = "55baf7c5a9f61a7dd9ff6e230d744e03d371c050d61f45895be98f7bcdb5f126"
FIRMWARE_SHA = "42b33b2e31a1c558f7d2d8ebba09fa7359cbe8197244389b994438733cb54e33"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    raw = SOURCE.read_bytes()
    assert sha(raw) == SOURCE_SHA and len(raw) % 2352 == 0
    sectors = len(raw) // 2352
    iso = bytearray(sectors * 2048)
    for lba in range(sectors):
        old = raw[lba * 2352:(lba + 1) * 2352]
        payload = old[16:2064]
        assert sector(payload, lba) == old, f"source sector {lba} is not exact"
        iso[lba * 2048:(lba + 1) * 2048] = payload
    before = directory_files(iso)
    assert len(before) == 22 and sha(before["A.BIN"]["data"]) == (
        "c9083578d310a70d6eb87e3be5d1ff228e0952299ca6b3a3b673904140543b23"
    )
    alt = FIRMWARE.read_bytes()
    assert sha(alt) == FIRMWARE_SHA
    scsp = before["SCSP.ELF"]
    assert scsp["size"] == 54416 and len(alt) < scsp["size"]
    patch = alt + bytes(scsp["size"] - len(alt))
    offset = scsp["lba"] * 2048
    iso[offset:offset + scsp["size"]] = patch
    after = directory_files(iso)
    for name in before:
        assert before[name]["lba"] == after[name]["lba"]
        assert before[name]["size"] == after[name]["size"]
        assert after[name]["data"] == (patch if name == "SCSP.ELF" else before[name]["data"])
    OUT.mkdir(parents=True, exist_ok=True)
    stem = "wrestlefest-runtime18-audio-isolation"
    iso_path = OUT / (stem + ".iso")
    bin_path = OUT / (stem + ".bin")
    cue_path = OUT / (stem + ".cue")
    for path in (iso_path, bin_path, cue_path):
        if path.exists():
            raise FileExistsError(f"refusing to replace {path}")
    iso_path.write_bytes(iso)
    with bin_path.open("xb") as output:
        for lba in range(sectors):
            payload = iso[lba * 2048:(lba + 1) * 2048]
            output.write(sector(payload, lba))
    cue_path.write_text(
        f'FILE "{bin_path.name}" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n'
    )
    result = {
        "purpose": "isolate native music module from frontend and SFX",
        "source_bin_sha256": sha(raw),
        "alternative_firmware_sha256": sha(alt),
        "output_iso_sha256": sha(iso),
        "output_bin_sha256": sha(bin_path.read_bytes()),
        "sectors": sectors,
        "cdfs_files_unchanged_except_scsp_elf": True,
        "music_elf_present_but_not_booted": True,
        "saturn_hardware_validated": False,
    }
    (OUT / "isolation-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
