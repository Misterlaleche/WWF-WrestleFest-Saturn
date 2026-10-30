#!/usr/bin/env python3
"""Create a self-contained source/evidence ZIP from an explicit path manifest.

All paths in the input JSON are relative to the independent wp1980 directory.
The packer refuses outside paths, symlinks, BIOS copies, and existing outputs.
It hashes the exact streamed bytes and tests the resulting archive.
"""
import argparse
import hashlib
import json
import stat
import zipfile
from pathlib import Path

BIOS_MD5 = "3240872c70984b6cbfda1586cab68dbe"


def package(root, selection, output):
    root = Path(root).resolve()
    names = json.loads(Path(selection).read_text())["files"]
    assert len(names) == len(set(names)), "Duplicate input path"
    assert not output.exists(), "Refusing to overwrite an archive"
    files = []
    for name in sorted(names):
        relative = Path(name)
        assert not relative.is_absolute() and ".." not in relative.parts
        path = root / relative
        assert path.is_file() and not path.is_symlink(), name
        assert path.resolve().is_relative_to(root), name
        assert "system" not in relative.parts, "Execution firmware directory excluded"
        assert path.name.lower() not in {"mpr-17933.bin", "sega_101.bin", "sega_100.bin"}
        if path.stat().st_size == 524288:
            assert hashlib.md5(path.read_bytes()).hexdigest() != BIOS_MD5, "BIOS bytes excluded"
        files.append((name, path))
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest = []
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6, allowZip64=True) as archive:
        for name, path in files:
            info = zipfile.ZipInfo("wp1980/" + name, date_time=(2026, 10, 29, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info._compresslevel = 6
            info.external_attr = (stat.S_IFREG | (path.stat().st_mode & 0o777)) << 16
            digest = hashlib.sha256()
            total = 0
            with path.open("rb") as source, archive.open(info, "w", force_zip64=True) as target:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    target.write(block)
                    digest.update(block)
                    total += len(block)
            manifest.append({"path": name, "bytes": total, "sha256": digest.hexdigest()})
        archive.writestr("wp1980/ARCHIVE_SHA256.json",
                         json.dumps({"files": manifest, "bios_included": False}, indent=2) + "\n")
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None, "ZIP CRC verification failed"
    digest = hashlib.sha256()
    with output.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    result = {"status": "PASS", "archive": str(output.resolve()),
              "sha256": digest.hexdigest(), "archive_bytes": output.stat().st_size,
              "files": len(manifest), "uncompressed_bytes": sum(x["bytes"] for x in manifest),
              "zip_crc_verified": True, "bios_included": False}
    output.with_suffix(output.suffix + ".json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--selection", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(package(args.root, args.selection, args.output), indent=2))


if __name__ == "__main__":
    main()
