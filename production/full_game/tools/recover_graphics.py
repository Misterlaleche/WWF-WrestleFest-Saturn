#!/usr/bin/env python3
"""Recover approved textures from an explicitly supplied, external arcade ROM.

Only Python's standard library is required. Graphics.tile, pack_4bpp and
pack_vdp2_pattern below translate the bit indexing and packing in the unchanged
Intro/source/tools/arcade_graphics.py without NumPy. The atlas order comes from
native_assets.json, environment.json and entrance_assets.h, as consumed by the
original generators. Every complete output must match its approved manifest.

The output directory must be NEW and outside the repository. Nothing is added
to Git; ROM bytes are never copied to the output. SEQ.DAT, maps, palettes and
Presentadora resources remain OPEN, rather than being inferred from pictures.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import zipfile
import zlib


ROOT = Path(__file__).resolve().parents[3]
PROGRAM_SHA1 = "a726c46f350723ea73158bc78055d17d418b14e2"
# Same order/CRC as SPRITE_ROMS in the original arcade_graphics.py.
SPRITE_ROMS = (
    ("31j3.ic9", 0xE395CF1D), ("31j2.ic8", 0xB5A97465),
    ("31j5.ic11", 0x2CE545E8), ("31j4.ic10", 0x00EDB66A),
    ("31j6.ic12", 0x79956CF8), ("31j7.ic13", 0x74D774C3),
    ("31j9.ic15", 0xDD387289), ("31j8.ic14", 0x44ABE127),
)
AUTHORITIES = (
    "Intro/ASSETS-MANIFEST.json", "Intro/source/native_assets.json",
    "Intro/source/tools/arcade_graphics.py", "Intro/source/tools/native_assets.py",
    "Entrada Ring/ASSETS-MANIFEST.json", "Entrada Ring/source/assets/environment.json",
    "Entrada Ring/source/entrance_assets.h", "Entrada Ring/source/tools/build_environment.py",
    "Escena Presentadora/ASSETS-MANIFEST.json",
)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


class Graphics:
    """Original ROM bitplanes, with scalar indexing equivalent to NumPy."""
    def __init__(self, path):
        self.members = []
        self.cache = {}
        with zipfile.ZipFile(path) as archive:
            if len(archive.namelist()) != len(set(archive.namelist())):
                raise ValueError("ROM archive contains duplicate member names")

            def read(name, size, crc=None):
                if archive.getinfo(name).file_size != size:
                    raise ValueError("Unexpected ROM member size: " + name)
                data = archive.read(name)
                if crc is not None and zlib.crc32(data) != crc:
                    raise ValueError("Unexpected source graphics ROM: " + name)
                self.members.append({"name": name, "bytes": len(data), "sha256": sha256(data)})
                return data

            # Original even/odd 68000 lane order, independent of ZIP packaging.
            even = read("31e14-0.ic18", 0x40000)
            odd = read("31e13-0.ic19", 0x40000)
            program = bytearray(0x80000)
            program[0::2], program[1::2] = even, odd
            self.program_sha1 = hashlib.sha1(program).hexdigest()
            if self.program_sha1 != PROGRAM_SHA1:
                raise ValueError("Unexpected ROM program SHA1: " + self.program_sha1)
            self.sprites = b"".join(read(name, 0x100000, crc) for name, crc in SPRITE_ROMS)
            self.font = read("31e12-0.ic33", 0x20000, 0x06F22615)
            self.bg = read("31j1.ic2", 0x40000, 0x82ED7155) + read("31j0.ic1", 0x40000, 0x8A12B450)

    def tile(self, kind, code):
        key = kind, code
        if key in self.cache:
            return self.cache[key]
        limit = 65536 if kind == "sprite" else 4096
        if not isinstance(code, int) or not 0 <= code < limit:
            raise ValueError("Invalid ROM tile ID: " + repr(key))
        size = 8 if kind == "text" else 16
        tile = []
        for y in range(size):
            row = []
            for x in range(size):
                value = 0
                if kind == "sprite":
                    # unpackbits(...).reshape(4,2,16,8).transpose(0,2,1,3)
                    for p in range(4):
                        offset = p * 0x200000 + code * 32 + (x // 8) * 16 + y
                        value |= ((self.sprites[offset] >> (7 - x % 8)) & 1) << (3 - p)
                elif kind == "text":
                    byte = self.font[code * 32 + (x // 2) * 8 + y]
                    for p in range(4):
                        value |= ((byte >> (7 - (1 - x % 2) - p * 2)) & 1) << (3 - p)
                else:
                    offset = code * 64 + y * 2 + (32 if x >= 8 else 0)
                    for p, plane in enumerate((1, 0, 0x40001, 0x40000)):
                        value |= ((self.bg[offset + plane] >> (7 - x % 8)) & 1) << (3 - p)
                row.append(value)
            tile.append(row)
        self.cache[key] = tile
        return tile


def pack_4bpp(tile):
    return bytes((row[x] << 4) | row[x + 1] for row in tile for x in range(0, len(row), 2))


def pack_vdp2_pattern(tile):
    if len(tile) == 8:
        return pack_4bpp(tile)
    return b"".join(pack_4bpp([row[x:x + 8] for row in tile[y:y + 8]])
                    for y in (0, 8) for x in (0, 8))


def approved_resources(root):
    """Only visual runtime requirements; audio is deliberately outside scope."""
    groups = (
        ("Intro", ["source/cd/" + name for name in ("BG0.TEX", "BG1.TEX", "FONT.TEX", "SPR.TEX", "SEQ.DAT")]),
        ("Entrada Ring", ["source/assets/" + name for name in (
            "BG0.TEX", "BG1.TEX", "FONT.TEX", "CROWD0.BIN", "CROWD1.BIN", "CROWD2.BIN",
            "MAP1.BIN", "REFERENCE_TEXT.BIN", "ENVPAL.C32", "ENTRANCE.TEX", "ENTRPAL.C32")]),
        ("Escena Presentadora", ["source/scene/cd/" + name for name in ("TEX.BIN", "RING.BIN", "CRAM.BIN")]),
    )
    resources = {}
    for folder, paths in groups:
        manifest_path = folder + "/ASSETS-MANIFEST.json"
        files = json.loads((root / manifest_path).read_text())["files"]
        if isinstance(files, list):
            files = {entry["path"]: entry for entry in files}
        for path in paths:
            entry = files[path]
            full_path = folder + "/" + path
            resources[full_path] = {
                "path": full_path, "status": "OPEN", "bytes": entry["bytes"],
                "expected_sha256": entry["sha256"], "actual_sha256": None,
                "authority": manifest_path,
                "reason": "Requires approved external assets or original captured maps/palettes; not reconstructed by this tool.",
            }
    resources["Intro/source/cd/SEQ.DAT"]["reason"] = (
        "Requires the original full-rate arcade device-state capture or approved SEQ.DAT; ROM tile IDs do not contain the scene sequence."
    )
    return resources


def generate_textures(root, gfx):
    intro = json.loads((root / "Intro/source/native_assets.json").read_text())["texture_source_ids"]
    environment = json.loads((root / "Entrada Ring/source/assets/environment.json").read_text())
    jobs = []
    for name, kind, codes in zip(("BG1.TEX", "BG0.TEX", "FONT.TEX", "SPR.TEX"),
                                 ("background", "background", "text", "sprite"), intro):
        jobs.append(("Intro/source/cd/" + name, kind, codes, "Intro/source/native_assets.json"))
    for index in range(2):
        jobs.append(("Entrada Ring/source/assets/BG%d.TEX" % index, "background",
                     environment["background_tile_codes"][index], "Entrada Ring/source/assets/environment.json"))
    jobs.append(("Entrada Ring/source/assets/FONT.TEX", "text", range(4096), "Entrada Ring/source/tools/build_environment.py"))
    header = (root / "Entrada Ring/source/entrance_assets.h").read_text()
    match = re.search(r"wp01_entrance_source_tile\[[^]]+\]\s*=\s*\{([^}]+)\}", header)
    if not match:
        raise ValueError("Missing approved entrance source-tile table")
    codes = [int(token, 0) for token in re.findall(r"0[xX][0-9a-fA-F]+|\d+", match.group(1))]
    if len(codes) != 707:
        raise ValueError("Unexpected entrance source-tile count")
    jobs.append(("Entrada Ring/source/assets/ENTRANCE.TEX", "sprite", codes, "Entrada Ring/source/entrance_assets.h"))
    for path, kind, codes, table in jobs:
        pack = pack_4bpp if kind == "sprite" else pack_vdp2_pattern
        yield path, b"".join(pack(gfx.tile(kind, code)) for code in codes), table


def recover(rom, out, root=ROOT):
    root, rom, out = root.resolve(), rom.resolve(), out.absolute()
    if out.exists() or out.is_symlink():
        raise ValueError("Output directory must be new; existing resources will not be overwritten: " + str(out))
    if root == out.resolve() or root in out.resolve().parents:
        raise ValueError("Output must be outside the repository; generated graphics must remain external to Git")
    if not rom.is_file():
        raise ValueError("ROM archive not found: " + str(rom))
    gfx = Graphics(rom)
    resources = approved_resources(root)
    outputs = {}
    for path, data, table in generate_textures(root, gfx):
        entry = resources[path]
        digest = sha256(data)
        if digest != entry["expected_sha256"] or len(data) != entry["bytes"]:
            raise ValueError("Approved size/SHA256 mismatch for %s: expected %s, actual %s" %
                             (path, entry["expected_sha256"], digest))
        outputs[path] = data
        entry.update(status="RECOVERED", actual_sha256=digest, source_table=table)
        del entry["reason"]
    report = {
        "schema": 1, "scope": "Exact visual texture recovery; incomplete scene resource set; audio disabled",
        "source": {"rom_path": str(rom), "archive_sha256": sha256(rom.read_bytes()),
                   "program_sha1": gfx.program_sha1, "members": gfx.members},
        "decoder": "Scalar translation of Intro/source/tools/arcade_graphics.py; original atlas order and packing",
        "authorities": [{"path": path, "sha256": sha256((root / path).read_bytes())} for path in AUTHORITIES],
        "resources": list(resources.values()),
        "recovered_count": len(outputs), "open_count": len(resources) - len(outputs),
        "recovered_bytes": sum(map(len, outputs.values())),
    }
    # All bytes and hashes are validated before any output resource is written.
    # Build outside the checkout, then publish to a new directory exclusively.
    out.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=".graphics-recovery-", dir=out.parent))
    try:
        for path, data in outputs.items():
            target = staging / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            if sha256(target.read_bytes()) != resources[path]["expected_sha256"]:
                raise OSError("Persisted resource SHA256 mismatch: " + path)
        (staging / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
        # mkdir is exclusive even if another process created out after preflight.
        out.mkdir()
        for child in staging.iterdir():
            os.rename(child, out / child.name)
    finally:
        shutil.rmtree(staging)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True, help="External canonical wwfwfest.zip")
    parser.add_argument("--out", type=Path, required=True, help="New directory outside the repository")
    args = parser.parse_args()
    try:
        report = recover(args.rom, args.out)
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print("ROM graphics recovery failed: " + str(error), file=sys.stderr)
        return 1
    print(json.dumps({key: report[key] for key in ("recovered_count", "open_count", "recovered_bytes")}))
    print("Manifest: " + str(args.out.absolute() / "manifest.json"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
