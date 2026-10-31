"""Recovery acceptance tests. Set WF_ROM to an external wwfwfest.zip.

Expected output digests are copied from the approved asset manifests, never
computed with the decoder under test. The ROM is not a repository fixture.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "production/full_game/tools/recover_graphics.py"
ROM = os.environ.get("WF_ROM")
EXPECTED = {
    "Intro/source/cd/BG0.TEX": "41761350b0e3a4c695cd134818e62ec089a1238ef4e74ff08ab406303c5bf1be",
    "Intro/source/cd/BG1.TEX": "507cb16659dccce557d96bf63ebf5e0c2f9c2f0b8ae5c5f4cd9fa88760291a32",
    "Intro/source/cd/FONT.TEX": "b1d9517ce7de7e3dc39c36d3a6fbc1d55d1e4463279530c2b52eae978a5b3f67",
    "Intro/source/cd/SPR.TEX": "311da8a6c6cdb6c16f2cef10cfad24719b153cfb9652b2eee849d1ea8fa26b84",
    "Entrada Ring/source/assets/BG0.TEX": "48bf18083d1d47baf5d3a496efb950db4ebb9cbc184836e9b866e51589c2813c",
    "Entrada Ring/source/assets/BG1.TEX": "0577ccfef4122c80887611e01e211c9960c4262c77f86c33c16d58415747a265",
    "Entrada Ring/source/assets/FONT.TEX": "a3045c6193d23c334c6af592cc1dfee029dc12eec810d3e2ce353df69580391a",
    "Entrada Ring/source/assets/ENTRANCE.TEX": "7f267a628c9075c30a5b58759f6151a96fed75198eb958feac5f1991891b3841",
}


def run_cli(rom, out, script=SCRIPT):
    return subprocess.run(
        [sys.executable, str(script), "--rom", str(rom), "--out", str(out)],
        capture_output=True, text=True, check=False,
    )


class RecoveryErrors(unittest.TestCase):
    def test_invalid_zip_is_rejected_without_creating_output(self):
        # Removing ROM validation must not turn corrupt input into resources.
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            rom = base / "invalid.zip"
            rom.write_bytes(b"not a ROM archive")
            result = run_cli(rom, base / "result")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("ROM", result.stderr)
            self.assertFalse((base / "result").exists())


@unittest.skipUnless(ROM, "Set WF_ROM to the external canonical wwfwfest.zip")
class CanonicalRecovery(unittest.TestCase):
    def test_recovers_only_verified_textures_and_records_open_resources(self):
        # Swapped atlas IDs, wrong bitplane order, or wrong tile packing fails
        # against independent approved SHA256 values for all eight outputs.
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary) / "recovered"
            result = run_cli(ROM, out)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads((out / "manifest.json").read_text())
            resources = {entry["path"]: entry for entry in report["resources"]}
            recovered = {p for p, entry in resources.items() if entry["status"] == "RECOVERED"}
            self.assertEqual(recovered, set(EXPECTED))
            self.assertEqual(report["source"]["program_sha1"], "a726c46f350723ea73158bc78055d17d418b14e2")
            for path, expected in EXPECTED.items():
                actual = hashlib.sha256((out / path).read_bytes()).hexdigest()
                self.assertEqual(actual, expected, path)
                self.assertEqual(resources[path]["expected_sha256"], expected)
                self.assertEqual(resources[path]["actual_sha256"], expected)
                self.assertTrue(resources[path]["authority"])
            self.assertEqual(len(resources), 19)
            self.assertEqual(resources["Intro/source/cd/SEQ.DAT"]["status"], "OPEN")
            self.assertEqual(resources["Entrada Ring/source/assets/CROWD0.BIN"]["status"], "OPEN")
            for path, entry in resources.items():
                if entry["status"] == "OPEN":
                    self.assertIsNone(entry["actual_sha256"])
                    self.assertTrue(entry["reason"])
                    self.assertFalse((out / path).exists(), path)
            emitted = {str(p.relative_to(out)) for p in out.rglob("*") if p.is_file()}
            self.assertEqual(emitted, set(EXPECTED) | {"manifest.json"})

    def test_different_existing_resource_is_never_overwritten(self):
        # Removing destination protection would destroy an existing resource.
        with tempfile.TemporaryDirectory() as temporary:
            out = Path(temporary) / "existing"
            target = out / "Intro/source/cd/BG0.TEX"
            target.parent.mkdir(parents=True)
            target.write_bytes(b"preserve existing resource")
            result = run_cli(ROM, out)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(target.read_bytes(), b"preserve existing resource")
            self.assertFalse((out / "manifest.json").exists())

    def modified_rom(self, base, member):
        target = base / "modified.zip"
        with zipfile.ZipFile(ROM) as source, zipfile.ZipFile(target, "w") as changed:
            for info in source.infolist():
                data = source.read(info.filename)
                if info.filename == member:
                    data = bytes([data[0] ^ 1]) + data[1:]
                changed.writestr(info.filename, data)
        return target

    def test_wrong_graphics_are_rejected_before_any_resource_is_written(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            result = run_cli(self.modified_rom(base, "31j3.ic9"), base / "result")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("31j3.ic9", result.stderr)
            self.assertFalse((base / "result").exists())

    def test_wrong_program_is_rejected_even_with_matching_graphics(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            result = run_cli(self.modified_rom(base, "31e14-0.ic18"), base / "result")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("program", result.stderr.lower())
            self.assertFalse((base / "result").exists())

    def test_mismatch_with_approved_hash_prevents_all_materialization(self):
        # Valid ROM + wrong approval digest must fail, rather than bless a
        # generated image using its own newly computed digest as authority.
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            fixture = base / "repo"
            paths = [
                "production/full_game/tools/recover_graphics.py",
                "Intro/ASSETS-MANIFEST.json", "Intro/source/native_assets.json",
                "Intro/source/tools/arcade_graphics.py", "Intro/source/tools/native_assets.py",
                "Entrada Ring/ASSETS-MANIFEST.json", "Entrada Ring/source/assets/environment.json",
                "Entrada Ring/source/entrance_assets.h", "Entrada Ring/source/tools/build_environment.py",
                "Escena Presentadora/ASSETS-MANIFEST.json",
            ]
            for relative in paths:
                target = fixture / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / relative, target)
            manifest = fixture / "Intro/ASSETS-MANIFEST.json"
            content = json.loads(manifest.read_text())
            content["files"][0]["sha256"] = "0" * 64
            manifest.write_text(json.dumps(content))
            result = run_cli(ROM, base / "result", fixture / paths[0])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("SHA256", result.stderr)
            self.assertFalse((base / "result").exists())


if __name__ == "__main__":
    unittest.main()
