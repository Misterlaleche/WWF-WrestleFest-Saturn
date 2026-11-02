"""Original-ROM extraction and corruption tests; no protected fixture is vendored."""
import hashlib
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]

class ResourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location('resources', ROOT/'tools/generate_resources.py')
        cls.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.module)
        with zipfile.ZipFile(os.environ['WF_ROM_ZIP']) as archive:
            cls.sound = archive.read('31a11-2.ic42')
            cls.samples = archive.read('31j10.ic73')

    def test_exact_tables_and_samples(self):
        result = self.module.generate(self.sound, self.samples)
        self.assertEqual(len(result['commands']), 98)
        self.assertEqual(len(result['samples']), 114)
        self.assertEqual(result['distinct_samples'], 95)
        self.assertEqual(hashlib.sha256(result['pcm16be']).hexdigest(),
                         '45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee')
        self.assertLessEqual(len(result['pack']), 0x6c000)

    def test_rom_changes_are_rejected_before_any_resources_exist(self):
        for which in ('sound', 'samples'):
            for at in (0, 32768, len(getattr(self, which))-1):
                bad = bytearray(getattr(self, which)); bad[at] ^= 1
                with self.assertRaisesRegex(ValueError, 'identity'):
                    self.module.generate(bytes(bad) if which == 'sound' else self.sound,
                                         bytes(bad) if which == 'samples' else self.samples)

    def test_truncation_is_rejected(self):
        with self.assertRaises(ValueError):
            self.module.generate(self.sound[:-1], self.samples)
        with self.assertRaises(ValueError):
            self.module.generate(self.sound, self.samples[:-1])

if __name__ == '__main__':
    unittest.main()
