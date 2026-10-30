#!/usr/bin/env python3
"""Restore the exact private HW1 bundle; never replace a different file.

The checked-in RECURSOS.json is the trust anchor. This command needs only the
Python standard library. It does not download files, run game code, or build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import stat
import sys
import zipfile


HANDOFF = Path(__file__).absolute().parents[1]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def checked_relative(name):
    if not isinstance(name, str) or not name or "\\" in name:
        raise ValueError("unsafe path in manifest")
    path = PurePosixPath(name)
    if path.is_absolute() or str(path) != name or any(part in (".", "..") for part in path.parts):
        raise ValueError("unsafe path in manifest: " + name)
    return Path(*path.parts)


def check_ancestors(path):
    """Do not follow existing links, including a link used as the root."""
    current = Path(path.anchor)
    for part in path.parts[1:]:
        current = current / part
        if current.is_symlink():
            raise ValueError("symlink is not allowed in restoration path: " + str(current))
        if current.exists() and current != path and not current.is_dir():
            raise ValueError("non-directory ancestor: " + str(current))


def load_manifest(scope):
    manifest = json.loads((HANDOFF / "RECURSOS.json").read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1:
        raise ValueError("unsupported resource manifest version")
    files = {}
    for record in manifest["files"]:
        name = record["path"]
        checked_relative(name)
        if name in files:
            raise ValueError("duplicate manifest path: " + name)
        if not isinstance(record["bytes"], int) or record["bytes"] < 0:
            raise ValueError("invalid file size: " + name)
        digest = record["sha256"]
        if len(digest) != 64 or any(c not in "0123456789abcdef" for c in digest):
            raise ValueError("invalid file hash: " + name)
        files[name] = record
    names = list(files) if scope == "all" else manifest["scopes"][scope]
    if len(names) != len(set(names)) or any(name not in files for name in names):
        raise ValueError("invalid restoration scope")
    return manifest, files, {name: files[name] for name in names}


def inspect_destination(root, records, verify):
    existing = 0
    missing = []
    check_ancestors(root)
    if root.exists() and not root.is_dir():
        raise ValueError("restoration root is not a directory")
    for name, record in records.items():
        path = root / checked_relative(name)
        check_ancestors(path)
        if not path.exists():
            missing.append(name)
            continue
        if not path.is_file() or path.stat().st_size != record["bytes"] or file_sha256(path) != record["sha256"]:
            raise ValueError("existing file differs; refusing to overwrite: " + str(path))
        existing += 1
    if verify and missing:
        raise ValueError("verification failed: missing " + str(len(missing)) + " files; first: " + missing[0])
    return existing, missing


def read_archive(archive, manifest, all_records, selected):
    if file_sha256(archive) != manifest["package"]["sha256"]:
        raise ValueError("archive SHA-256 does not match the exact private HW1 bundle")
    buffers = {}
    with zipfile.ZipFile(archive) as bundle:
        entries = bundle.infolist()
        names = [entry.filename for entry in entries]
        if len(names) != len(set(names)) or set(names) != set(all_records):
            raise ValueError("archive member list does not match the manifest")
        for entry in entries:
            mode = entry.external_attr >> 16
            if entry.is_dir() or stat.S_ISLNK(mode):
                raise ValueError("archive contains a directory or symlink member")
            if entry.filename not in selected:
                continue
            data = bundle.read(entry)
            record = selected[entry.filename]
            if len(data) != record["bytes"] or sha256(data) != record["sha256"]:
                raise ValueError("archive member hash or size mismatch: " + entry.filename)
            buffers[entry.filename] = data
    return buffers


def open_child_directory(parent_fd, name):
    try:
        os.mkdir(name, mode=0o755, dir_fd=parent_fd)
    except FileExistsError:
        pass
    return os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=parent_fd)


def create_missing(root, missing, buffers):
    """Anchor writes to directory descriptors, including while creating root.

    A link substituted after preflight must never redirect a file creation.
    This POSIX operation deliberately has no weaker pathname-only fallback.
    """
    if not missing:
        return
    if not hasattr(os, "O_NOFOLLOW") or not hasattr(os, "O_DIRECTORY"):
        raise ValueError("restoration requires POSIX directory descriptors (use Linux or WSL)")
    root_fd = os.open(root.anchor, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        for part in root.parts[1:]:
            child_fd = open_child_directory(root_fd, part)
            os.close(root_fd)
            root_fd = child_fd
        for name in missing:
            parts = checked_relative(name).parts
            parent_fd = os.dup(root_fd)
            try:
                for part in parts[:-1]:
                    child_fd = open_child_directory(parent_fd, part)
                    os.close(parent_fd)
                    parent_fd = child_fd
                flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW
                file_fd = os.open(parts[-1], flags, 0o644, dir_fd=parent_fd)
                with os.fdopen(file_fd, "wb") as stream:
                    stream.write(buffers[name])
            finally:
                os.close(parent_fd)
    finally:
        os.close(root_fd)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", nargs="?", type=Path, help="Exact wp1980_hw1_fuentes_y_pruebas.zip")
    parser.add_argument("--scope", choices=("build", "all"), default="build")
    parser.add_argument("--verify", action="store_true", help="Read-only check of all files in the selected scope")
    parser.add_argument("--root", type=Path, default=HANDOFF, help="Restoration folder; defaults to modo Royal")
    args = parser.parse_args()
    if args.verify and args.archive:
        parser.error("--verify checks restored files; omit the archive argument")
    if not args.verify and args.archive is None:
        parser.error("provide the exact source archive, or use --verify")
    try:
        manifest, all_records, selected = load_manifest(args.scope)
        root = Path(os.path.abspath(args.root))
        buffers = {} if args.verify else read_archive(args.archive, manifest, all_records, selected)
        existing, missing = inspect_destination(root, selected, args.verify)
        # Hash every selected member and preflight every destination BEFORE any
        # writes. Exclusive creation also refuses a file introduced meanwhile.
        create_missing(root, missing, buffers)
        inspect_destination(root, selected, True)
        print(json.dumps({
            "status": "PASS", "operation": "verify" if args.verify else "restore",
            "scope": args.scope, "root": str(root), "files": len(selected),
            "bytes": sum(record["bytes"] for record in selected.values()),
            "existing_unchanged": existing, "created": len(missing),
            "archive_sha256": manifest["package"]["sha256"],
        }, indent=2))
        return 0
    except (OSError, ValueError, KeyError, TypeError, zipfile.BadZipFile) as error:
        print("ERROR: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
