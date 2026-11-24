#!/usr/bin/env python3
"""Restore locked CD assets from ISO2048 or a single MODE1/2352 data track.

Only the primary ISO9660 root is used. Both-endian numeric fields, extents,
record boundaries, raw sync/mode/MSF headers, sizes and full SHA-256 digests
are checked. Joliet/Rock Ridge names, subdirectory traversal, interleaving,
extended attributes and multi-extent files are not supported. EDC/ECC is not
decoded: locked-file SHA-256 verifies the extracted payload.

Every selected asset is staged and verified before any destination is
replaced. os.replace changes directory entries, preserving other hardlinks.
No executable 0.BIN is imported. No BIOS is needed or included.

ISO field definitions: ECMA-119, 4th edition, sections 7.2.3, 7.3.3, 8.4, 9.1:
https://www.ecma-international.org/wp-content/uploads/ECMA-119_4th_edition_june_2019.pdf
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SECTOR = 2048
RAW_SECTOR = 2352
SYNC = b"\0" + b"\xff" * 10 + b"\0"
LOCK_FORMAT = "wf-disc-asset-lock"
SAFE_NAME = re.compile(r"[A-Z0-9_][A-Z0-9_.-]{0,30}\Z")
HEX_SHA = re.compile(r"[0-9a-f]{64}\Z")
MAX_DIRECTORY_BYTES = 16 * 1024 * 1024
CHUNK_SECTORS = 256


class AssetError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise AssetError(message)


def safe_name(name, locked=False):
    require(isinstance(name, str) and SAFE_NAME.fullmatch(name) is not None,
            f"Invalid root asset name: {name!r}")
    require(not locked or name.upper() != "0.BIN", "0.BIN is a build executable, not an importable asset")
    return name


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"Duplicate JSON key: {key!r}")
        result[key] = value
    return result


def load_lock(path):
    require(path.stat().st_size <= 2 * 1024 * 1024, "Asset lock is too large")
    lock = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object)
    require(isinstance(lock, dict) and lock.get("format") == LOCK_FORMAT and
            type(lock.get("version")) is int and lock["version"] == 1, "Unsupported asset-lock format/version")
    files = lock.get("files")
    require(isinstance(files, dict) and 0 < len(files) <= 4096, "Asset lock must contain 1..4096 root files")
    for name, identity in files.items():
        safe_name(name, locked=True)
        require(isinstance(identity, dict), f"Invalid identity for {name}")
        size, digest = identity.get("bytes"), identity.get("sha256")
        require(type(size) is int and 0 <= size <= 0xffffffff, f"Invalid byte count for {name}")
        require(isinstance(digest, str) and HEX_SHA.fullmatch(digest) is not None,
                f"Invalid SHA-256 for {name}")
    total = lock.get("total_bytes")
    require(type(total) is int and total == sum(v["bytes"] for v in files.values()), "Asset-lock total does not match its files")
    return lock


def both(data, offset, width, field):
    require(offset + 2 * width <= len(data), f"Truncated {field}")
    little = int.from_bytes(data[offset:offset + width], "little")
    big = int.from_bytes(data[offset + width:offset + 2 * width], "big")
    require(little == big, f"Both-endian values disagree in {field}")
    return little


@dataclass(frozen=True)
class Entry:
    identifier: bytes
    extent: int
    size: int
    flags: int


def parse_record(data):
    require(len(data) >= 34 and data[0] == len(data) and not len(data) % 2,
            "Invalid ISO9660 directory-record length")
    require(data[1] == 0, "Extended-attribute records are unsupported")
    require(data[25] & ~3 == 0, "Associated, multi-extent or reserved file flags are unsupported")
    require(data[26] == data[27] == 0, "Interleaved files are unsupported")
    require(both(data, 28, 2, "volume sequence") == 1, "Only volume sequence 1 is supported")
    name_length = data[32]
    minimum = 33 + name_length + (not name_length % 2)
    require(name_length > 0 and minimum <= len(data), "Invalid ISO9660 identifier length")
    if not name_length % 2:
        require(data[33 + name_length] == 0, "Invalid identifier padding")
    return Entry(bytes(data[33:33 + name_length]),
                 both(data, 2, 4, "extent"), both(data, 10, 4, "data length"), data[25])


class DiscImage:
    def __init__(self, path, format_hint="auto"):
        self.path = Path(path)
        self.file = self.path.open("rb")
        self.volume_blocks = None
        try:
            self.image_bytes = os.fstat(self.file.fileno()).st_size
            if format_hint == "auto":
                first = self._peek(0, 16)
                iso_header = self._peek(16 * SECTOR, 7)
                raw_header = self._peek(16 * RAW_SECTOR + 16, 7)
                iso_found = iso_header[1:] == b"CD001\x01"
                raw_found = raw_header[1:] == b"CD001\x01"
                require(not (iso_found and raw_found), "Ambiguous sector format; select --format explicitly")
                if raw_found or not iso_found and first[:12] == SYNC:
                    format_hint = "mode1-2352"
                elif iso_found:
                    format_hint = "iso2048"
                else:
                    raise AssetError("Cannot identify ISO2048 or MODE1/2352 image at LBA 0")
            require(format_hint in ("iso2048", "mode1-2352"), "Unsupported image format")
            self.format = format_hint
            self.stride = RAW_SECTOR if self.format == "mode1-2352" else SECTOR
            require(self.image_bytes >= 18 * self.stride and self.image_bytes % self.stride == 0,
                    "Truncated image or invalid physical sector size")
            self.image_blocks = self.image_bytes // self.stride
            if self.format == "mode1-2352":
                self._check_raw(self._peek(0, RAW_SECTOR), 0, 0)
        except BaseException:
            self.file.close()
            raise

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.file.close()

    def _peek(self, offset, count):
        self.file.seek(offset)
        return self.file.read(count)

    @staticmethod
    def _check_raw(data, offset, lba):
        require(0 <= offset and offset + RAW_SECTOR <= len(data), "Truncated MODE1 sector")
        require(data[offset:offset + 12] == SYNC and data[offset + 15] == 1,
                f"Invalid MODE1 sync/mode at sector {lba}")
        values = data[offset + 12:offset + 15]
        require(all((v & 15) <= 9 and v >> 4 <= 9 for v in values), f"Invalid BCD MSF at sector {lba}")
        minute, second, frame = ((v >> 4) * 10 + (v & 15) for v in values)
        require(second < 60 and frame < 75 and (minute * 60 + second) * 75 + frame == lba + 150,
                f"Unexpected MODE1 address at sector {lba}; image must begin at LBA 0")
        require(not any(data[offset + 2068:offset + 2076]), f"Nonzero MODE1 reserved bytes at sector {lba}")

    def blocks(self, lba, count):
        limit = self.volume_blocks if self.volume_blocks is not None else self.image_blocks
        require(type(lba) is int and type(count) is int and 0 <= lba <= limit and
                0 <= count <= min(CHUNK_SECTORS, limit - lba), "Sector request is outside the image/volume")
        physical = self._peek(lba * self.stride, count * self.stride)
        require(len(physical) == count * self.stride, "Image truncated during read")
        if self.format == "iso2048":
            return physical
        decoded = bytearray(count * SECTOR)
        for index in range(count):
            offset = index * RAW_SECTOR
            self._check_raw(physical, offset, lba + index)
            decoded[index * SECTOR:(index + 1) * SECTOR] = physical[offset + 16:offset + 16 + SECTOR]
        return decoded

    def check_extent(self, extent, size):
        limit = self.volume_blocks if self.volume_blocks is not None else self.image_blocks
        blocks = (size + SECTOR - 1) // SECTOR
        require(0 <= extent <= limit and 0 <= blocks <= limit - extent, "File/directory extent is outside the volume")

    def stream(self, entry):
        self.check_extent(entry.extent, entry.size)
        lba, remaining = entry.extent, entry.size
        while remaining:
            count = min(CHUNK_SECTORS, (remaining + SECTOR - 1) // SECTOR)
            data = self.blocks(lba, count)
            used = min(remaining, len(data))
            yield data[:used]
            remaining -= used
            lba += count

    def root_entries(self):
        primary, ended = None, False
        for lba in range(16, min(self.image_blocks, 16 + 64)):
            descriptor = self.blocks(lba, 1)
            require(descriptor[1:7] == b"CD001\x01", f"Invalid volume descriptor at sector {lba}")
            kind = descriptor[0]
            if kind == 1:
                require(primary is None or descriptor == primary, "Conflicting primary volume descriptors")
                primary = descriptor
                volume = both(descriptor, 80, 4, "volume size")
                require(18 <= volume <= self.image_blocks, "Declared volume exceeds the image")
                require(both(descriptor, 120, 2, "volume-set size") == 1 and
                        both(descriptor, 124, 2, "volume sequence") == 1, "Only a single-volume image is supported")
                require(both(descriptor, 128, 2, "logical block size") == SECTOR, "ISO logical blocks must be 2048 bytes")
                require(descriptor[881] == 1, "Unsupported ISO file-structure version")
                self.volume_blocks = volume
            elif kind == 255:
                ended = True
                break
            else:
                require(kind in (0, 2, 3), "Unsupported volume descriptor type")
        require(primary is not None and ended, "Primary descriptor or volume terminator missing")
        root = parse_record(primary[156:190])
        require(root.identifier == b"\0" and root.flags == 2 and
                0 < root.size <= MAX_DIRECTORY_BYTES and root.size % SECTOR == 0,
                "Invalid or oversized root directory")
        self.check_extent(root.extent, root.size)
        entries, special = {}, []
        for lba in range(root.extent, root.extent + root.size // SECTOR):
            block, at = self.blocks(lba, 1), 0
            while at < SECTOR:
                length = block[at]
                if length == 0:
                    require(not any(block[at:]), "Nonzero bytes after directory padding")
                    break
                require(length >= 34 and at + length <= SECTOR, "Directory record crosses a sector boundary")
                entry = parse_record(block[at:at + length])
                at += length
                self.check_extent(entry.extent, entry.size)
                if entry.identifier in (b"\0", b"\1"):
                    require(not entries and entry.flags == 2 and entry.extent == root.extent and entry.size == root.size,
                            "Invalid root self/parent record")
                    special.append(entry.identifier)
                    continue
                require(special == [b"\0", b"\1"], "Root self/parent records are missing or duplicated")
                try:
                    identifier = entry.identifier.decode("ascii")
                except UnicodeDecodeError as error:
                    raise AssetError("Non-ASCII primary filename is unsupported") from error
                if ";" in identifier:
                    name, version = identifier.rsplit(";", 1)
                    require(version == "1", "Only ISO file version ;1 is supported")
                else:
                    name = identifier
                safe_name(name)
                require(name not in entries, f"Duplicate/ambiguous ISO root name: {name}")
                entries[name] = entry
        require(special == [b"\0", b"\1"], "Invalid root self/parent records")
        return entries


def extract_assets(image_path, lock_path, output_dir, verify_only=False, format_hint="auto"):
    image_path, lock_path, output_dir = map(Path, (image_path, lock_path, output_dir))
    lock = load_lock(lock_path)
    with DiscImage(image_path, format_hint) as image:
        entries = image.root_entries()
        for name, identity in lock["files"].items():
            require(name in entries and not entries[name].flags & 2, f"Missing locked regular file: {name}")
            require(entries[name].size == identity["bytes"], f"Size mismatch for {name}")
        names = sorted(lock["files"])

        def verified_copy(name, destination=None):
            digest, size = hashlib.sha256(), 0
            for chunk in image.stream(entries[name]):
                if destination is not None:
                    require(destination.write(chunk) == len(chunk), f"Short output write for {name}")
                digest.update(chunk)
                size += len(chunk)
            require(size == lock["files"][name]["bytes"], f"Incomplete payload for {name}")
            require(digest.hexdigest() == lock["files"][name]["sha256"], f"SHA-256 mismatch for {name}")

        if verify_only:
            for name in names:
                verified_copy(name)
        else:
            existed = output_dir.exists()
            protected = (image_path.resolve(), lock_path.resolve())
            for name in names:
                target = output_dir / name
                require(target.resolve() not in protected and not target.is_dir(), f"Unsafe output target: {target}")
            output_dir.mkdir(parents=True, exist_ok=True)
            try:
                with tempfile.TemporaryDirectory(prefix=".wf-disc-assets-", dir=output_dir) as temporary:
                    staged = Path(temporary)
                    for name in names:
                        with (staged / name).open("xb") as target:
                            verified_copy(name, target)
                            target.flush()
                            os.fsync(target.fileno())
                    # No destination was replaced before all hashes passed.
                    for name in names:
                        os.replace(staged / name, output_dir / name)
            except BaseException:
                if not existed:
                    try:
                        output_dir.rmdir()
                    except OSError:
                        pass
                raise
        return {"format": image.format, "files": len(names), "bytes": lock["total_bytes"],
                "verify_only": bool(verify_only)}


def write_lock(source_dir, lock_path):
    source_dir, lock_path = Path(source_dir), Path(lock_path)
    files = {}
    for path in sorted(source_dir.iterdir()):
        if path.name.upper() == "0.BIN":
            continue
        name = safe_name(path.name, locked=True)
        require(path.is_file() and not path.is_symlink(), f"Asset source must be a regular root file: {path}")
        digest, size = hashlib.sha256(), 0
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
                size += len(chunk)
        require(size <= 0xffffffff, f"Asset exceeds ISO9660 size field: {name}")
        files[name] = {"bytes": size, "sha256": digest.hexdigest()}
    require(0 < len(files) <= 4096, "Asset lock requires 1..4096 root files")
    lock = {"format": LOCK_FORMAT, "version": 1,
            "total_bytes": sum(v["bytes"] for v in files.values()), "files": files}
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=lock_path.parent,
                                         prefix=".asset-lock-", delete=False) as out:
            temporary = Path(out.name)
            json.dump(lock, out, indent=2)
            out.write("\n")
        os.replace(temporary, lock_path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return lock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", nargs="?", type=Path, help="BIN MODE1/2352 or ISO2048 beginning at LBA 0")
    parser.add_argument("--lock", type=Path, default=ROOT / "asset-lock.json")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "cd")
    parser.add_argument("--verify-only", action="store_true", help="Read and hash all locked assets without writing any output")
    parser.add_argument("--format", choices=("auto", "mode1-2352", "iso2048"), default="auto")
    parser.add_argument("--create-lock-from", type=Path, help="Maintainer mode: generate a lock from regular root files, excluding 0.BIN")
    args = parser.parse_args()
    if (args.image is None) == (args.create_lock_from is None):
        parser.error("Specify either an image or --create-lock-from")
    if args.create_lock_from is not None and args.verify_only:
        parser.error("--verify-only applies to an image, not lock generation")
    try:
        if args.create_lock_from is not None:
            lock = write_lock(args.create_lock_from, args.lock)
            print(f"Locked {len(lock['files'])} assets, {lock['total_bytes']} bytes: {args.lock}")
        else:
            result = extract_assets(args.image, args.lock, args.output_dir, args.verify_only, args.format)
            action = "Verified" if args.verify_only else "Imported"
            print(f"{action} {result['files']} assets, {result['bytes']} bytes from {result['format']}")
    except (AssetError, OSError, UnicodeError, json.JSONDecodeError) as error:
        parser.exit(1, f"Asset import failed: {error}\n")


if __name__ == "__main__":
    main()
