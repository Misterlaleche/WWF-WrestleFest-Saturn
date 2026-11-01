"""Reproduce the mixed-source matrix failure and enforce a frozen profile."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    'audio2_source_snapshot', Path(__file__).resolve().parents[1] / 'tools/source_snapshot.py')
snapshot = importlib.util.module_from_spec(spec)
spec.loader.exec_module(snapshot)


class SourceSnapshotTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'package'
        (self.source / 'src').mkdir(parents=True)
        (self.source / 'src/engine.c').write_text('old source\n')

    def test_subsequent_source_edit_does_not_change_frozen_profile(self):
        frozen = self.root / 'frozen'
        identity = snapshot.snapshot_package(self.source, frozen)
        (self.source / 'src/engine.c').write_text('new source\n')
        self.assertEqual(identity, snapshot.source_identity(frozen))
        self.assertNotEqual(identity, snapshot.source_identity(self.source))

    def test_edit_during_copy_prevents_experiment(self):
        copytree = snapshot.shutil.copytree

        def editing_copy(*args, **kwargs):
            result = copytree(*args, **kwargs)
            (self.source / 'src/engine.c').write_text('concurrent edit\n')
            return result

        with patch.object(snapshot.shutil, 'copytree', side_effect=editing_copy):
            with self.assertRaisesRegex(RuntimeError, 'Source changed'):
                snapshot.snapshot_package(self.source, self.root / 'frozen')

    def test_recursive_destination_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'outside'):
            snapshot.snapshot_package(self.source, self.source / 'generated')


if __name__ == '__main__':
    unittest.main()
