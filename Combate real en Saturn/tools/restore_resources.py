#!/usr/bin/env python3
"""Restore the exact WP1979 resources from the user's approved source ZIP."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def sha_file(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def target_for(name):
    rel = PurePosixPath(name)
    if rel.is_absolute() or ".." in rel.parts or not rel.parts or rel.parts[0] != "wp1979":
        raise ValueError(f"Unsafe manifest path: {name}")
    target = ROOT.joinpath(*rel.parts)
    if not target.resolve().is_relative_to(ROOT):
        raise ValueError(f"Destination escapes this implementation: {name}")
    return target


def matches(path, entry):
    return (path.is_file() and path.stat().st_size == entry["bytes"]
            and sha_file(path) == entry["sha256"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", nargs="?", type=Path)
    parser.add_argument("--scope", choices=("build", "all"), default="build")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "RECURSOS.json").read_text(encoding="utf-8"))
    entries = [entry for entry in manifest["files"]
               if args.scope == "all" or entry["build_required"]]
    # Validate every destination and every existing file before writing anything.
    pending = []
    for entry in entries:
        target = target_for(entry["path"])
        if target.exists():
            if not matches(target, entry):
                raise FileExistsError(f"Refusing to overwrite different content: {entry['path']}")
        else:
            pending.append((entry, target))
    if args.verify:
        if pending:
            raise ValueError("Missing resources: " + ", ".join(e["path"] for e, _ in pending))
        print(json.dumps({"status": "PASS", "scope": args.scope, "verified_files": len(entries)}))
        return
    if args.package is None:
        parser.error("Provide wp1979-fuentes-pruebas.zip, or use --verify")
    if sha_file(args.package) != manifest["package_sha256"]:
        raise ValueError("Source ZIP SHA-256 does not match the approved WP1979 candidate 4")
    with zipfile.ZipFile(args.package) as archive:
        # Inspect the entire selected input set before creating any output.
        for entry in entries:
            data = archive.read(entry["archive_path"])
            if len(data) != entry["bytes"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
                raise ValueError(f"Archived resource differs: {entry['path']}")
        for entry, target in pending:
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as stream:
                stream.write(archive.read(entry["archive_path"]))
    if not all(matches(target_for(entry["path"]), entry) for entry in entries):
        raise ValueError("Post-restoration verification failed")
    print(json.dumps({"status": "PASS", "scope": args.scope,
                      "restored_files": len(pending), "verified_files": len(entries)}))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
