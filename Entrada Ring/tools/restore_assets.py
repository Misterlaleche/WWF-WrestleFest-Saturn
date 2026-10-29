#!/usr/bin/env python3
"""Restore only hash-verified R02 resources from the owner's approved ZIP."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def destination(relative):
    path = Path(relative)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"Unsafe manifest path: {relative}")
    target = ROOT / path
    if not target.resolve().is_relative_to(ROOT):
        raise ValueError(f"Destination escapes module: {relative}")
    return target


def valid(data, expected):
    return len(data) == expected["bytes"] and digest(data) == expected["sha256"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path, nargs="?", help="Approved R02 source ZIP")
    parser.add_argument("--verify", action="store_true", help="Only verify installed resources")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "ASSETS-MANIFEST.json").read_text())
    resources = manifest["files"]
    if args.verify:
        failures = [name for name, expected in resources.items()
                    if not destination(name).is_file()
                    or not valid(destination(name).read_bytes(), expected)]
        if failures:
            raise ValueError("Missing or modified resources: " + ", ".join(failures))
        print(json.dumps({"status": "PASS", "verified_resources": len(resources)}))
        return
    if args.package is None:
        parser.error("Provide the approved source ZIP, or use --verify")
    package = args.package.read_bytes()
    if digest(package) != manifest["package_sha256"]:
        raise ValueError("Source ZIP SHA256 does not match the approved R02 package")
    prepared = []
    with zipfile.ZipFile(io.BytesIO(package)) as archive:
        for name, expected in resources.items():
            data = archive.read(manifest["archive_prefix"] + name)
            if not valid(data, expected):
                raise ValueError(f"Invalid archived resource: {name}")
            target = destination(name)
            if target.exists():
                if not target.is_file() or not valid(target.read_bytes(), expected):
                    raise FileExistsError(f"Refusing to overwrite different content: {target}")
            else:
                prepared.append((target, data))
    for target, data in prepared:
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(data)
    assert all(valid(destination(name).read_bytes(), expected)
               for name, expected in resources.items())
    print(json.dumps({"status": "PASS", "restored_resources": len(prepared),
                      "verified_resources": len(resources)}))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
