#!/usr/bin/env python3
"""Package tiny synthetic fixtures; never copy the project's full music/image."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import package_release as packager
from test_extract_disc_assets import make_iso, make_raw, lock_data


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="wf-package-test-")
        self.base = Path(self.temp.name)
        self.project = self.base / "project"
        self.variant = self.project / "build/variants/frozen"
        self.assets = self.base / "canonical"
        self.output = self.base / "release"
        files = {
            "core/src/logic.c": b"/* frozen logic */\n",
            "port/main.c": b"/* frozen platform */\n",
            "sdk/jo_engine/core.c": b"/* frozen Jo */\n",
            "sdk/jo_engine/jo_options.txt": b"-O2 -DWF_JO_FRONTEND=1 -fno-peephole2\n",
            "sdk/Compiler/COMMON/jo_engine_makefile": b"# build rules\n",
            "sdk/Compiler/COMMON/IP.BIN": b"boot template, not BIOS",
            "makefile": b"# frozen makefile\n", "core_sources.mk": b"# source list\n",
        }
        for name, data in files.items():
            path = self.variant / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        identity = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}
        identity["sdk/jo_engine/jo_options.txt"] = "0" * 64  # Pre-build generated options differ.
        (self.variant / "source-identity.json").write_text(json.dumps(identity))
        for name in ("core/src/logic.o", "core/src/logic.d", "port/BIOS.BIN", "port/session.mcs0"):
            (self.variant / name).write_bytes(b"exclude me")
        compiler = self.variant / "sdk/Compiler/LINUX"
        compiler.symlink_to(self.base, target_is_directory=True)
        for name in ("tools/helper.py", "tests/check.c", "docs/guide.md", "README.md", "requirements-assets.txt"):
            path = self.project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("support source\n")
        for name in ("tools/duplicate.wav", "tools/cache.pyc", "tests/state.mcs0", "docs/BIOS.ROM"):
            (self.project / name).write_bytes(b"exclude me")
        (self.project / "tools/link.py").symlink_to(self.variant / "port/main.c")
        live = self.project / "core/src/logic.c"
        live.parent.mkdir(parents=True, exist_ok=True)
        live.write_text("/* live tree differs from frozen variant */\n")
        self.assets.mkdir()
        for name in packager.CANONICAL:
            (self.assets / name).write_bytes(b"canonical " + name.encode())
        (self.assets / "MUSIC.ELF").write_bytes(b"not needed")
        (self.assets / "SCSP.ELF").write_bytes(b"not needed")
        (self.assets / "BIOS.BIN").write_bytes(b"excluded")
        locked = {"FONT8.BIN": (self.assets / "FONT8.BIN").read_bytes(), "SFX.DAT": b"short sound"}
        (self.project / "asset-lock.json").write_text(json.dumps(lock_data(locked)))
        self.program = b"tiny executable fixture"
        iso, _, _ = make_iso(dict(locked, **{"0.BIN": self.program}))
        (self.variant / "game.bin").write_bytes(make_raw(iso))
        (self.variant / "game.raw").write_bytes(self.program)
        (self.variant / "game.cue").write_text('FILE "game.bin" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n')
        evidence = self.project / "evidence"
        evidence.mkdir()
        (evidence / "small.json").write_text('{"status":"measured"}\n')
        (evidence / "state.mcs0").write_bytes(b"emulator state")

    def tearDown(self):
        self.temp.cleanup()

    def package(self, **kwargs):
        return packager.package_release(self.project, self.variant, self.assets, self.output,
                                        "WrestleFest_Test", [Path("evidence/small.json")], **kwargs)

    def test_small_pair_contents_hashes_and_determinism(self):
        result = self.package()
        source = self.output / result["source_archive"]["filename"]
        disc = self.output / result["disc_archive"]["filename"]
        prefix = "WrestleFest_Test_source/"
        with zipfile.ZipFile(source) as archive:
            self.assertIsNone(archive.testzip())
            names = archive.namelist()
            self.assertEqual(archive.read(prefix + "core/src/logic.c"), b"/* frozen logic */\n")
            self.assertIn(prefix + "sdk/Compiler/COMMON/IP.BIN", names)
            self.assertIn(prefix + "source_assets/SPRITES.PAK", names)
            self.assertIn(prefix + "evidence/small.json", names)
            for name in names:
                self.assertFalse(any(token in name for token in ("build/variants", "Compiler/LINUX", "BIOS", ".wav", ".mcs0", ".pyc", "link.py")), name)
                self.assertNotIn(Path(name).suffix, (".o", ".d", ".elf", ".raw"))
            manifest = json.loads(archive.read(prefix + "source-manifest.json"))
            self.assertEqual(manifest["build_options_verbatim"], (self.variant / "sdk/jo_engine/jo_options.txt").read_text())
            self.assertNotIn("production_ready", manifest)
            self.assertNotIn("date", manifest)
            for name, row in manifest["files"].items():
                data = archive.read(prefix + name)
                self.assertEqual(len(data), row["bytes"])
                self.assertEqual(hashlib.sha256(data).hexdigest(), row["sha256"])
        with zipfile.ZipFile(disc) as archive:
            self.assertEqual(set(archive.namelist()), {"WrestleFest_Test.bin", "WrestleFest_Test.cue", "disc-manifest.json"})
            cue = archive.read("WrestleFest_Test.cue").decode()
            self.assertIn('FILE "WrestleFest_Test.bin" BINARY', cue)
            self.assertEqual(archive.read("WrestleFest_Test.bin"), (self.variant / "game.bin").read_bytes())
        hashes = (source.read_bytes(), disc.read_bytes())
        self.package()
        self.assertEqual(hashes, (source.read_bytes(), disc.read_bytes()))

    def test_dry_run_writes_no_archive(self):
        result = self.package(dry_run=True)
        self.assertGreater(result["source_bytes"], 0)
        self.assertFalse(self.output.exists())

    def test_frozen_source_mismatch_rejected(self):
        (self.variant / "core/src/logic.c").write_text("changed after freeze")
        with self.assertRaises(packager.PackageError):
            self.package()
        self.assertFalse(self.output.exists())

    def test_missing_frozen_source_rejected(self):
        (self.variant / "port/main.c").unlink()
        with self.assertRaises(packager.PackageError):
            self.package(dry_run=True)
        self.assertFalse(self.output.exists())

    def test_unsafe_member_names_rejected(self):
        for name in (r"safe\..\..\escape.py", "C:drive.py", "bad\x01.py"):
            path = self.project / "tools" / name
            path.write_text("unwanted member")
            with self.assertRaises(packager.PackageError):
                self.package(dry_run=True)
            path.unlink()
        self.assertFalse(self.output.exists())

    def test_wrong_program_or_cue_rejected(self):
        (self.variant / "game.raw").write_bytes(b"another executable")
        with self.assertRaises(packager.PackageError):
            self.package()
        (self.variant / "game.raw").write_bytes(self.program)
        (self.variant / "game.cue").write_text('FILE "../outside.bin" BINARY\n TRACK 01 MODE1/2352\n')
        with self.assertRaises(packager.PackageError):
            self.package()
        self.assertFalse(self.output.exists())

    def test_emulator_evidence_rejected(self):
        with self.assertRaises(packager.PackageError):
            packager.package_release(self.project, self.variant, self.assets, self.output,
                                     "Safe", [Path("evidence/state.mcs0")])
        self.assertFalse(self.output.exists())

    def test_terminal_actor_evidence_selected_checked_and_hashed(self):
        names = ("tag-terminal-actors.bin", "royal-terminal-actors.bin",
                 "frontend-tag-terminal-actors.bin", "frontend-royal-terminal-actors.bin")
        requested = []
        for index, name in enumerate(names):
            path = self.project / "evidence" / name
            path.write_bytes(bytes([index]) * 2412)
            requested.append(Path("evidence") / name)
        # Actor dumps are only admitted as an explicit evidence selection.
        result = self.package()
        prefix = "WrestleFest_Test_source/"
        with zipfile.ZipFile(self.output / result["source_archive"]["filename"]) as archive:
            self.assertFalse(any(name.endswith("-terminal-actors.bin") for name in archive.namelist()))
        result = packager.package_release(self.project, self.variant, self.assets, self.output,
                                          "WrestleFest_Test", requested)
        with zipfile.ZipFile(self.output / result["source_archive"]["filename"]) as archive:
            manifest = json.loads(archive.read(prefix + "source-manifest.json"))
            for relative in requested:
                name = relative.as_posix()
                actual = archive.read(prefix + name)
                self.assertEqual(actual, (self.project / relative).read_bytes())
                self.assertEqual(manifest["files"][name], {
                    "bytes": 2412, "sha256": hashlib.sha256(actual).hexdigest(),
                    "origin": "selected_evidence"})

    def test_terminal_actor_evidence_wrong_name_or_size_rejected(self):
        cases = (("BIOS.bin", 2412), ("WorkRAMH-state.bin", 2412),
                 ("terminal-actors.bin", 2412), ("tag-terminal-actors.BIN", 2412),
                 ("tag-terminal-actors.bin", 2411), ("tag-terminal-actors.bin", 2413),
                 ("tag-terminal-actors.bin", 32768))
        for name, size in cases:
            with self.subTest(name=name, size=size):
                relative = Path("evidence") / name
                (self.project / relative).write_bytes(b"x" * size)
                with self.assertRaises(packager.PackageError):
                    packager.package_release(self.project, self.variant, self.assets, self.output,
                                             "Safe", [relative], dry_run=True)
                self.assertFalse(self.output.exists())

    def test_terminal_actor_evidence_boundaries(self):
        relative = Path("evidence/tag-terminal-actors.bin")
        path = self.project / relative
        path.write_bytes(b"x" * 2412)
        linked = self.project / "evidence/link-terminal-actors.bin"
        linked.symlink_to(path)
        outside = self.base / "outside-terminal-actors.bin"
        outside.write_bytes(b"x" * 2412)
        for requested in (linked, outside):
            with self.subTest(requested=str(requested)):
                with self.assertRaises(packager.PackageError):
                    packager.package_release(self.project, self.variant, self.assets, self.output,
                                             "Safe", [requested], dry_run=True)
        with mock.patch.object(packager, "MAX_EVIDENCE_BYTES", 2411):
            with self.assertRaisesRegex(packager.PackageError, "budget"):
                packager.package_release(self.project, self.variant, self.assets, self.output,
                                         "Safe", [relative], dry_run=True)
        self.assertFalse(self.output.exists())

    def test_terminal_actor_size_rechecked_after_selection(self):
        relative = Path("evidence/tag-terminal-actors.bin")
        path = self.project / relative
        path.write_bytes(b"x" * 2412)
        original = packager.identity
        def changed(source):
            if source == path:
                source.write_bytes(b"x" * 2413)
            return original(source)
        with mock.patch.object(packager, "identity", side_effect=changed):
            with self.assertRaisesRegex(packager.PackageError, "changed size"):
                packager.package_release(self.project, self.variant, self.assets, self.output,
                                         "Safe", [relative], dry_run=True)
        self.assertFalse(self.output.exists())

    def test_unlisted_disc_payload_rejected(self):
        locked = {"FONT8.BIN": (self.assets / "FONT8.BIN").read_bytes(), "SFX.DAT": b"short sound"}
        iso, _, _ = make_iso(dict(locked, **{"0.BIN": self.program, "BIOS.BIN": b"unlisted payload"}))
        (self.variant / "game.bin").write_bytes(make_raw(iso))
        with self.assertRaisesRegex(packager.PackageError, "outside asset-lock"):
            self.package()
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
