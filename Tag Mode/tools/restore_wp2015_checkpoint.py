#!/usr/bin/env python3
"""Restore source-only WP2015 checkpoint 06 to a NEW directory."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path, PurePosixPath
import sys
import zipfile

EXPECTED_SHA256 = "321c23f6aebb576edb33effa8f71da3d61e45570405e97fc628b5de4f9aab405"
PREFIX = "wp2015/work/native/"
EXTENSIONS = {".c", ".h", ".s", ".sx", ".inc", ".py", ".ld", ".md", ".txt", ".json", ".sh", ".patch"}
MAKEFILES = {"Makefile", "Makefile.wp1979", "Makefile.wp2015", "Makefile.saturn-disc", "Makefile.saturn-link"}

def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for part in iter(lambda: f.read(1024 * 1024), b""):
            h.update(part)
    return h.hexdigest()

def is_source(rel: PurePosixPath) -> bool:
    raw = rel.as_posix()
    if not raw.startswith(("src/", "tools/", "validation/")):
        return False
    if "/audit/runtime18-cd/" in raw or raw.endswith(".orig"):
        return False
    return rel.suffix.lower() in EXTENSIONS or rel.name in MAKEFILES

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("checkpoint", type=Path, help="wp2015-checkpoint-06.zip")
    ap.add_argument("destination", type=Path, help="NEW, non-existing output directory")
    args = ap.parse_args()
    actual = digest(args.checkpoint)
    if actual != EXPECTED_SHA256:
        raise ValueError(f"ZIP hash mismatch: {actual}")
    target = args.destination.resolve()
    if target.exists():
        raise FileExistsError(f"Refusing to overwrite existing path: {target}")
    prepared = []
    with zipfile.ZipFile(args.checkpoint) as archive:
        for info in archive.infolist():
            if info.is_dir() or not info.filename.startswith(PREFIX):
                continue
            name = info.filename[len(PREFIX):]
            rel = PurePosixPath(name)
            if not name or rel.is_absolute() or ".." in rel.parts:
                raise ValueError(f"Unsafe archive entry: {info.filename}")
            if is_source(rel):
                prepared.append((str(rel), archive.read(info)))
    if not prepared:
        raise ValueError("Archive has no source files")
    target.mkdir(parents=True, exist_ok=False)
    for name, data in prepared:
        path = target / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as f:
            f.write(data)
    print(f"PASS: {len(prepared)} source/build/validation files restored in {target}")
    print("NOT a R6 recovery; ROM-derived binaries and the Saturn SDK are NOT included")

if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
