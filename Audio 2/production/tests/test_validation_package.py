"""Private artifact selection, exclusion and SHA-256 gates; synthetic inputs."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('validation_package', ROOT / 'tools/package_validation_artifacts.py')
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)


class ValidationPackage(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'build'
        self.source.mkdir()
        self.elf = self.source / 'AUDIO2.ELF'
        self.elf.write_bytes(b'\x7fELF' + bytes(60))
        self.report = self.source / 'report.json'
        self.report.write_text('{"passed": false}\n')
        self.plan = {'schema': 1, 'metadata': {'scope': 'synthetic failed run'},
                     'files': [self.row(self.elf, 'music/01/AUDIO2.ELF', 'elf'),
                               self.row(self.report, 'music/01/report.json', 'text')]}

    def row(self, path, member, kind):
        return {'source': str(path), 'member': member, 'kind': kind,
                'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

    def test_explicit_selection_excludes_unlisted_protected_files(self):
        for name in ('SOUND.ROM', 'bios.bin', 'native.wav', 'native.mov', 'temp.o', 'engine.c'):
            (self.source / name).write_bytes(b'excluded synthetic file')
        result = packager.package(self.plan, self.root / 'delivery', reserve_bytes=0)
        verified = packager.verify(Path(result['zip']))
        self.assertEqual(verified['artifact_count'], 2)
        with zipfile.ZipFile(result['zip']) as archive:
            names = set(archive.namelist())
            self.assertIn('music/01/AUDIO2.ELF', names)
            self.assertNotIn('SOUND.ROM', names)
            manifest = json.loads(archive.read('MANIFEST.json'))
            self.assertEqual(manifest['software_status'], 'NOT PRODUCTION READY')
            self.assertEqual(manifest['hardware_status'], 'HARDWARE PENDING')
            self.assertFalse(manifest['audio_waveforms_included'])

    def test_explicit_forbidden_files_are_rejected(self):
        for name in ('SOUND.ROM', 'bios.bin', 'native.wav', 'native.mov', 'temp.o', 'engine.c'):
            with self.subTest(name=name):
                source = self.source / name
                source.write_bytes(b'excluded synthetic file')
                plan = {'schema': 1, 'files': [self.row(source, 'pretend/report.json', 'text')]}
                with self.assertRaisesRegex(ValueError, 'Forbidden'):
                    packager.package(plan, self.root / ('bad-' + name), reserve_bytes=0)

    def test_changed_input_cannot_be_packaged_under_old_hash(self):
        self.elf.write_bytes(self.elf.read_bytes() + b'changed')
        with self.assertRaisesRegex(ValueError, 'SHA256 mismatch'):
            packager.package(self.plan, self.root / 'changed', reserve_bytes=0)
        self.assertFalse((self.root / 'changed').exists())

    def test_archive_hash_audit_detects_changed_payload_with_valid_crc(self):
        result = packager.package(self.plan, self.root / 'valid', reserve_bytes=0)
        modified = self.root / 'modified.zip'
        with zipfile.ZipFile(result['zip']) as source, zipfile.ZipFile(modified, 'w') as target:
            for item in source.infolist():
                data = source.read(item.filename)
                if item.filename == 'music/01/AUDIO2.ELF':
                    data += b'changed'
                target.writestr(item.filename, data)
        with self.assertRaisesRegex(ValueError, 'SHA256 mismatch'):
            packager.verify(modified)

    def test_existing_output_is_preserved(self):
        out = self.root / 'previous'
        out.mkdir()
        sentinel = out / 'sentinel'
        sentinel.write_bytes(b'preserve')
        with self.assertRaisesRegex(ValueError, 'fresh private output'):
            packager.package(self.plan, out, reserve_bytes=0)
        self.assertEqual(sentinel.read_bytes(), b'preserve')
        self.assertEqual(list(out.iterdir()), [sentinel])

    def test_unsafe_or_duplicate_archive_members_are_rejected(self):
        for member in ('../escape.ELF', '/absolute.ELF', 'MANIFEST.json', 'music/01/report.json'):
            with self.subTest(member=member):
                plan = json.loads(json.dumps(self.plan))
                plan['files'][0]['member'] = member
                with self.assertRaises(ValueError):
                    packager.package(plan, self.root / 'unsafe', reserve_bytes=0)
                self.assertFalse((self.root / 'unsafe').exists())

    def test_distinct_download_names_cannot_escape_output(self):
        for name in ('../escape.zip', '/tmp/escape.zip', 'nested/file.zip'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'plain ASCII'):
                packager.package(self.plan, self.root / 'named', reserve_bytes=0, archive_name=name)
        result = packager.package(self.plan, self.root / 'named', reserve_bytes=0,
                                  archive_name='AUDIO2-VALIDACION-ARTIFACTOS.zip')
        self.assertEqual(Path(result['zip']).name, 'AUDIO2-VALIDACION-ARTIFACTOS.zip')


if __name__ == '__main__':
    unittest.main()
