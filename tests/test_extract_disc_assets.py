#!/usr/bin/env python3
"""Exercise the disc importer with synthetic metadata and an optional real BIN.

The real-image pass verifies every locked byte but extracts only FONT8.BIN.
It neither regenerates nor copies the music into the source delivery.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import extract_disc_assets as extractor

SECTOR = 2048
SYNC = b"\0" + b"\xff" * 10 + b"\0"
REAL_IMAGE = None


def both(value, width):
    return value.to_bytes(width, "little") + value.to_bytes(width, "big")


def record(name, extent, size, flags=0):
    length = 33 + len(name) + (not len(name) % 2)
    data = bytearray(length)
    data[0] = length
    data[2:10] = both(extent, 4)
    data[10:18] = both(size, 4)
    data[25] = flags
    data[28:32] = both(1, 2)
    data[32] = len(name)
    data[33:33 + len(name)] = name
    return data


def make_iso(files):
    directory = record(b"\0", 20, SECTOR, 2) + record(b"\1", 20, SECTOR, 2)
    at, locations, records = 22, {}, {}
    for name, data in files.items():
        locations[name] = at
        records[name] = 20 * SECTOR + len(directory)
        directory += record((name + ";1").encode("ascii"), at, len(data))
        at += (len(data) + SECTOR - 1) // SECTOR
    assert len(directory) <= SECTOR
    sectors = max(at + 1, 32)
    iso = bytearray(sectors * SECTOR)
    pvd = bytearray(SECTOR)
    pvd[:7] = b"\x01CD001\x01"
    pvd[80:88] = both(sectors, 4)
    pvd[120:124] = both(1, 2)
    pvd[124:128] = both(1, 2)
    pvd[128:132] = both(SECTOR, 2)
    pvd[156:190] = record(b"\0", 20, SECTOR, 2)
    pvd[881] = 1
    iso[16 * SECTOR:17 * SECTOR] = pvd
    iso[17 * SECTOR:17 * SECTOR + 7] = b"\xffCD001\x01"
    iso[20 * SECTOR:20 * SECTOR + len(directory)] = directory
    for name, data in files.items():
        offset = locations[name] * SECTOR
        iso[offset:offset + len(data)] = data
    return iso, locations, records


def make_raw(iso):
    out = bytearray()
    def bcd(value):
        return value // 10 * 16 + value % 10
    for lba in range(len(iso) // SECTOR):
        time = lba + 150
        header = SYNC + bytes((bcd(time // 4500), bcd(time // 75 % 60), bcd(time % 75), 1))
        out += header + iso[lba * SECTOR:(lba + 1) * SECTOR] + bytes(288)
    return out


def lock_data(files):
    return {"format": "wf-disc-asset-lock", "version": 1,
            "total_bytes": sum(map(len, files.values())),
            "files": {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                      for name, data in files.items()}}


class ImportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="wf-disc-test-")
        self.root = Path(self.temp.name)
        self.files = {"A.BIN": bytes(range(251)) * 23,
                      "B.WAV": b"music bytes\0" * 691, "EMPTY.TXT": b""}
        all_files = dict(self.files, **{"0.BIN": b"program", "EXTRA.BIN": b"ignored"})
        self.iso, self.locations, self.records = make_iso(all_files)
        self.image = self.root / "disc.iso"
        self.image.write_bytes(self.iso)
        self.lock = self.root / "asset-lock.json"
        self.lock.write_text(json.dumps(lock_data(self.files)))
        self.output = self.root / "output"

    def tearDown(self):
        self.temp.cleanup()

    def run_import(self, verify_only=False):
        return extractor.extract_assets(self.image, self.lock, self.output, verify_only)

    def bad_image(self, data, raw=False):
        self.image = self.root / ("bad.bin" if raw else "bad.iso")
        self.image.write_bytes(data)
        with self.assertRaises(extractor.AssetError):
            self.run_import()
        self.assertFalse(self.output.exists())

    def test_iso_and_raw_roundtrip(self):
        for raw in (False, True):
            self.image = self.root / ("disc.bin" if raw else "disc.iso")
            source = make_raw(self.iso) if raw else self.iso
            self.image.write_bytes(source)
            self.output = self.root / ("raw-output" if raw else "iso-output")
            result = self.run_import()
            self.assertEqual(result["files"], len(self.files))
            self.assertEqual(result["format"], "mode1-2352" if raw else "iso2048")
            self.assertEqual(sorted(p.name for p in self.output.iterdir()), sorted(self.files))
            for name, payload in self.files.items():
                self.assertEqual((self.output / name).read_bytes(), payload)
            self.assertEqual(self.image.read_bytes(), source)

    def test_verify_only_creates_nothing(self):
        result = self.run_import(True)
        self.assertEqual(result["bytes"], sum(map(len, self.files.values())))
        self.assertFalse(self.output.exists())

    def test_iso_with_raw_looking_system_area(self):
        data = bytearray(self.iso)
        data[:16] = SYNC + bytes((0, 2, 0, 1))
        self.image.write_bytes(data)
        self.assertEqual(self.run_import(True)["format"], "iso2048")

    def test_all_hashes_before_publication_and_hardlink_preserved(self):
        self.output.mkdir()
        target = self.output / "A.BIN"
        target.write_bytes(b"old generation")
        frozen = self.root / "frozen.bin"
        frozen.hardlink_to(target)
        data = lock_data(self.files)
        data["files"]["B.WAV"]["sha256"] = "0" * 64
        self.lock.write_text(json.dumps(data))
        with self.assertRaisesRegex(extractor.AssetError, "SHA-256"):
            self.run_import()
        self.assertEqual(target.read_bytes(), b"old generation")
        self.assertEqual(frozen.read_bytes(), b"old generation")
        self.assertEqual(sorted(p.name for p in self.output.iterdir()), ["A.BIN"])
        self.lock.write_text(json.dumps(lock_data(self.files)))
        self.run_import()
        self.assertEqual(target.read_bytes(), self.files["A.BIN"])
        self.assertEqual(frozen.read_bytes(), b"old generation")
        self.assertNotEqual(target.stat().st_ino, frozen.stat().st_ino)

    def test_bad_lock_names_and_duplicate_keys(self):
        for name in ("../A.BIN", "/A.BIN", "A\\B.BIN", "..", "0.BIN", "0.bin", "a.bin"):
            with self.subTest(name=name):
                self.lock.write_text(json.dumps(lock_data({name: b"x"})))
                with self.assertRaises(extractor.AssetError):
                    self.run_import()
                self.assertFalse(self.output.exists())
        self.lock.write_text('{"format":"wf-disc-asset-lock","version":1,"files":{},"files":{}}')
        with self.assertRaises(extractor.AssetError):
            self.run_import()

    def test_bad_lock_size_and_digest(self):
        for field, value in (("bytes", True), ("bytes", -1), ("bytes", 2**32), ("sha256", "xyz")):
            data = lock_data(self.files)
            data["files"]["A.BIN"][field] = value
            self.lock.write_text(json.dumps(data))
            with self.assertRaises(extractor.AssetError):
                self.run_import()
            self.assertFalse(self.output.exists())

    def test_truncated_images(self):
        for raw in (False, True):
            data = make_raw(self.iso) if raw else self.iso
            stride = 2352 if raw else 2048
            for removed in (1, stride):
                self.bad_image(data[:-removed], raw)

    def test_endian_mismatch(self):
        positions = (16 * SECTOR + 84, 16 * SECTOR + 130,
                     self.records["A.BIN"] + 6, self.records["A.BIN"] + 14,
                     self.records["A.BIN"] + 30)
        for position in positions:
            with self.subTest(position=position):
                data = bytearray(self.iso)
                data[position] ^= 1
                self.bad_image(data)

    def test_extent_out_of_range(self):
        data = bytearray(self.iso)
        at = self.records["A.BIN"]
        data[at + 2:at + 10] = both(len(data) // SECTOR - 1, 4)
        self.bad_image(data)

    def test_invalid_directory_name(self):
        data, _, _ = make_iso({"../A.BIN": b"x"})
        self.bad_image(data)

    def test_unsupported_extents_and_malformed_record(self):
        at = self.records["A.BIN"]
        for offset, value in ((1, 1), (25, 128), (26, 1), (27, 1), (32, 255), (0, 33)):
            data = bytearray(self.iso)
            data[at + offset] = value
            self.bad_image(data)

    def test_missing_volume_terminator(self):
        data = bytearray(self.iso)
        data[17 * SECTOR] = 0
        self.bad_image(data)

    def test_raw_sector_format(self):
        for lba, offset, value in ((0, 15, 2), (16, 15, 2),
                                   (self.locations["A.BIN"], 15, 2),
                                   (self.locations["A.BIN"], 5, 0),
                                   (self.locations["A.BIN"], 12, 0x0a),
                                   (self.locations["A.BIN"], 2068, 1)):
            data = make_raw(self.iso)
            data[lba * 2352 + offset] = value
            self.bad_image(data, True)

    def test_lock_generation_excludes_program_case_insensitively(self):
        source = self.root / "source"
        source.mkdir()
        for name, payload in self.files.items():
            (source / name).write_bytes(payload)
        (source / "0.bin").write_bytes(b"build dependent")
        extractor.write_lock(source, self.lock)
        self.assertEqual(json.loads(self.lock.read_text()), lock_data(self.files))

    def test_real_image_all_assets_and_small_extraction(self):
        if REAL_IMAGE is None:
            self.skipTest("pass --real-image to verify the supplied BIN")
        result = extractor.extract_assets(REAL_IMAGE, ROOT / "asset-lock.json", self.output, True)
        expected = json.loads((ROOT / "asset-lock.json").read_text())
        self.assertEqual(result["bytes"], expected["total_bytes"])
        self.assertEqual(result["files"], len(expected["files"]))
        self.assertFalse(self.output.exists())
        subset = dict(expected)
        subset["files"] = {"FONT8.BIN": expected["files"]["FONT8.BIN"]}
        subset["total_bytes"] = subset["files"]["FONT8.BIN"]["bytes"]
        self.lock.write_text(json.dumps(subset))
        extractor.extract_assets(REAL_IMAGE, self.lock, self.output)
        actual = (self.output / "FONT8.BIN").read_bytes()
        self.assertEqual(hashlib.sha256(actual).hexdigest(), subset["files"]["FONT8.BIN"]["sha256"])
        print("Verified real image:", result)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--real-image", type=Path)
    args, remaining = parser.parse_known_args()
    REAL_IMAGE = args.real_image
    unittest.main(argv=[sys.argv[0], *remaining])
