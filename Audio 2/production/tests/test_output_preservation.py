"""Source/output preservation gates with synthetic inputs; no ROM or SDK."""
import contextlib
import hashlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'tools' / filename)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


resources = module('preservation_resources', 'generate_resources.py')
firmware = module('preservation_firmware', 'build_firmware.py')


class OutputPreservation(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.repository = self.root / 'repository'
        self.package = self.repository / 'Audio 2' / 'production'
        (self.package / 'src').mkdir(parents=True)
        (self.repository / '.git').mkdir()
        (self.package / 'src/wf_sfx_start.S').write_text('synthetic startup\n')

    def invoke(self, kind, out):
        if kind == 'resources':
            with mock.patch.object(resources, 'PACKAGE', self.package, create=True), \
                 mock.patch.object(resources.zipfile, 'ZipFile') as archive, \
                 mock.patch.object(resources, 'generate', return_value={
                     'pack': b'synthetic pack', 'sound': b'synthetic sound',
                     'pcm16be': b'synthetic PCM'}), \
                 mock.patch.object(sys, 'argv', ['generate_resources.py',
                     '--rom-zip', str(self.root / 'synthetic.zip'), '--out', str(out)]), \
                 contextlib.redirect_stdout(io.StringIO()):
                archive.return_value.__enter__.return_value.read.return_value = b''
                resources.main()
        else:
            # A fresh permitted build reaches tool discovery; no compiler runs.
            with mock.patch.object(firmware.shutil, 'which', return_value=None):
                firmware.build(self.package, out, 'missing-synthetic-compiler-')

    def assert_rejected_without_writes(self, kind, out):
        before = {str(p.relative_to(self.root)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in self.root.rglob('*') if p.is_file()}
        error = None
        try:
            self.invoke(kind, out)
        except Exception as caught:
            error = caught
        after = {str(p.relative_to(self.root)): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in self.root.rglob('*') if p.is_file()}
        self.assertEqual(before, after, 'Refused output modified prior files')
        self.assertIsInstance(error, ValueError)
        self.assertRegex(str(error), 'fresh private output')

    def test_existing_output_preserves_all_sentinels(self):
        for kind in ('resources', 'firmware'):
            with self.subTest(tool=kind):
                out = self.root / ('previous-' + kind)
                out.mkdir()
                for name in ('SFX.DAT', 'SOUND.ROM', 'PCM16BE.PAK', 'resources.json',
                             'startup.S', 'SCSP.ELF', 'firmware-build.json'):
                    (out / name).write_bytes(('preserved ' + name).encode())
                self.assert_rejected_without_writes(kind, out)

    def test_existing_empty_directory_is_not_a_new_output(self):
        for kind in ('resources', 'firmware'):
            with self.subTest(tool=kind):
                out = self.root / ('empty-' + kind)
                out.mkdir()
                self.assert_rejected_without_writes(kind, out)

    def test_source_and_historical_directories_are_protected(self):
        for kind in ('resources', 'firmware'):
            for parent in (self.package, self.repository / 'Audio 2/base',
                           self.repository / 'docs'):
                with self.subTest(tool=kind, parent=parent):
                    out = parent / ('new-output-' + kind)
                    self.assert_rejected_without_writes(kind, out)
                    self.assertFalse(out.exists())

    def test_archive_without_git_still_protects_historical_audio(self):
        (self.repository / '.git').rmdir()
        for kind in ('resources', 'firmware'):
            with self.subTest(tool=kind):
                out = self.repository / 'Audio 2/base' / ('new-' + kind)
                self.assert_rejected_without_writes(kind, out)
                self.assertFalse(out.exists())

    def test_dangling_output_symlink_is_not_a_new_output(self):
        for kind in ('resources', 'firmware'):
            with self.subTest(tool=kind):
                target = self.root / ('target-' + kind)
                out = self.root / ('link-' + kind)
                out.symlink_to(target, target_is_directory=True)
                self.assert_rejected_without_writes(kind, out)
                self.assertFalse(target.exists())

    def test_fresh_external_output_reaches_generation(self):
        out = self.root / 'fresh-resources'
        self.invoke('resources', out)
        self.assertEqual((out / 'SFX.DAT').read_bytes(), b'synthetic pack')
        out = self.root / 'fresh-firmware'
        with self.assertRaisesRegex(FileNotFoundError, 'missing-synthetic-compiler'):
            self.invoke('firmware', out)
        self.assertEqual((out / 'startup.S').read_text(), 'synthetic startup\n')


if __name__ == '__main__':
    unittest.main()
