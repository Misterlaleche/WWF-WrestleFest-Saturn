#!/usr/bin/env python3
"""Behavior tests for exact restoration, without any private game assets."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).with_name("restore_resources.py")


class RestoreTests(unittest.TestCase):
    def setUp(self):
        self.assertTrue(SCRIPT.is_file(), "The restoration command has not been implemented")
        self.temp = tempfile.TemporaryDirectory(prefix="royal-restore-test-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.root = self.base / "modo Royal"
        (self.root / "tools").mkdir(parents=True)
        shutil.copyfile(SCRIPT, self.root / "tools/restore_resources.py")
        self.archive = self.base / "private.zip"
        self.payloads = {
            "wp1980/work/hwfix01/native/src/sample.c": b"int sample(void) { return 7; }\r\n",
            "wp1980/work/hwfix01/native/audit/sample.bin": b"\x00\x11\xff\x80",
            "wp1980/native/src/old.c": b"int old(void) { return 3; }\n",
        }
        self.write_archive()
        self.manifest = {
            "schema_version": 1,
            "package": {"sha256": hashlib.sha256(self.archive.read_bytes()).hexdigest()},
            "scopes": {"build": list(self.payloads)[:2], "all": "all files"},
            "files": [
                {"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                for name, data in self.payloads.items()
            ],
        }
        self.write_manifest()

    def write_archive(self):
        with zipfile.ZipFile(self.archive, "w") as bundle:
            for name, data in self.payloads.items():
                bundle.writestr(name, data)

    def write_manifest(self):
        (self.root / "RECURSOS.json").write_text(json.dumps(self.manifest), encoding="utf-8")

    def run_restore(self, *args):
        return subprocess.run(
            [sys.executable, str(self.root / "tools/restore_resources.py"), *map(str, args)],
            capture_output=True, text=True,
        )

    def test_build_restores_exact_bytes_and_keeps_identical_sources(self):
        """Catches rewriting present source bytes or restoring unrelated old inputs."""
        source = self.root / next(iter(self.payloads))
        source.parent.mkdir(parents=True)
        source.write_bytes(next(iter(self.payloads.values())))
        before = source.stat().st_mtime_ns
        run = self.run_restore(self.archive, "--scope", "build")
        self.assertEqual(run.returncode, 0, run.stderr)
        for name in list(self.payloads)[:2]:
            self.assertEqual((self.root / name).read_bytes(), self.payloads[name])
        self.assertEqual(source.stat().st_mtime_ns, before)
        self.assertFalse((self.root / "wp1980/native/src/old.c").exists())
        again = self.run_restore(self.archive, "--scope", "build")
        self.assertEqual(again.returncode, 0, again.stderr)
        verify = self.run_restore("--scope", "build", "--verify")
        self.assertEqual(verify.returncode, 0, verify.stderr)
        self.assertEqual(json.loads(again.stdout)["created"], 0)

    def test_all_restores_original_baseline_as_well(self):
        """Catches a scope selector that silently omits old-reference inputs."""
        run = self.run_restore(self.archive, "--scope", "all")
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual((self.root / "wp1980/native/src/old.c").read_bytes(), b"int old(void) { return 3; }\n")
        self.assertEqual(self.run_restore("--scope", "all", "--verify").returncode, 0)

    def test_changed_bundle_is_refused_without_writes(self):
        """Catches accepting a different ZIP even if its selected entries still match."""
        with self.archive.open("ab") as stream:
            stream.write(b"tampered")
        run = self.run_restore(self.archive, "--scope", "build")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("archive SHA-256", run.stderr)
        self.assertFalse((self.root / "wp1980").exists())

    def test_changed_existing_file_is_refused_before_any_restoration(self):
        """Catches replacing user edits or partially restoring before finding a conflict."""
        source = self.root / next(iter(self.payloads))
        source.parent.mkdir(parents=True)
        source.write_bytes(b"user edit\n")
        run = self.run_restore(self.archive, "--scope", "build")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("existing file differs", run.stderr)
        self.assertEqual(source.read_bytes(), b"user edit\n")
        self.assertFalse((self.root / list(self.payloads)[1]).exists())

    def test_verify_does_not_repair_missing_or_changed_files(self):
        """Catches a verification command that mutates files or ignores corruption."""
        missing = self.run_restore("--scope", "build", "--verify")
        self.assertNotEqual(missing.returncode, 0)
        self.assertFalse((self.root / "wp1980").exists())
        self.assertEqual(self.run_restore(self.archive, "--scope", "build").returncode, 0)
        asset = self.root / list(self.payloads)[1]
        asset.write_bytes(b"changed")
        changed = self.run_restore("--scope", "build", "--verify")
        self.assertNotEqual(changed.returncode, 0)
        self.assertEqual(asset.read_bytes(), b"changed")

    def test_member_hash_is_checked_before_writing(self):
        """Catches trusting only the archive envelope and ignoring the file manifest."""
        self.manifest["files"][1]["sha256"] = "0" * 64
        self.write_manifest()
        run = self.run_restore(self.archive, "--scope", "build")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("member hash", run.stderr)
        self.assertFalse((self.root / "wp1980").exists())

    def test_unsafe_manifest_path_is_refused(self):
        """Catches escaping the chosen restoration folder through path traversal."""
        self.manifest["files"][0]["path"] = "../escape.c"
        self.manifest["scopes"]["build"][0] = "../escape.c"
        self.write_manifest()
        run = self.run_restore(self.archive)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("unsafe path", run.stderr)
        self.assertFalse((self.base / "escape.c").exists())

    def test_symlink_ancestor_is_refused(self):
        """Catches following a preexisting link into another working directory."""
        outside = self.base / "outside"
        outside.mkdir()
        (self.root / "wp1980").symlink_to(outside, target_is_directory=True)
        run = self.run_restore(self.archive)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("symlink", run.stderr)
        self.assertEqual(list(outside.iterdir()), [])

    def test_directory_replaced_with_symlink_during_creation_cannot_escape(self):
        """Catches a link swap after preflight but before a new file is opened."""
        outside = self.base / "outside"
        outside.mkdir()
        target = self.root / "wp1980/work/hwfix01/native/src"
        driver = r'''
import os
from pathlib import Path
import runpy
import sys
target, outside, script, archive = map(Path, sys.argv[1:])
original_mkdir = os.mkdir
swapped = False
def mkdir_and_swap(path, mode=0o777, *, dir_fd=None):
    global swapped
    original_mkdir(path, mode=mode, dir_fd=dir_fd)
    if not swapped and target.is_dir():
        swapped = True
        target.rename(target.with_name('src-before-swap'))
        target.symlink_to(outside, target_is_directory=True)
os.mkdir = mkdir_and_swap
sys.argv = [str(script), str(archive), '--scope', 'build']
try:
    runpy.run_path(str(script), run_name='__main__')
finally:
    print('SWAP_EXECUTED=' + str(swapped), file=sys.stderr)
'''
        run = subprocess.run(
            [sys.executable, "-c", driver, str(target), str(outside),
             str(self.root / "tools/restore_resources.py"), str(self.archive)],
            capture_output=True, text=True,
        )
        self.assertIn("SWAP_EXECUTED=True", run.stderr)
        self.assertNotEqual(run.returncode, 0)
        self.assertEqual(list(outside.iterdir()), [], "Restoration wrote through a substituted symlink")


if __name__ == "__main__":
    unittest.main(verbosity=2)
