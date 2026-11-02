"""Reference producers must preserve every existing output artifact.

The direct-C tests require WF_REFERENCE_CAPTURE and WF_REFERENCE_INPUTS; inputs
are generated privately from the user's ROM, never embedded in this fixture.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/reference'))
from reproduce_full_board_references import prepare_and_build, run_case

SENTINEL = b'PRESERVE ORIGINAL ARTIFACT\n'


class WrapperOwnership(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def test_existing_prepare_directory_is_untouched(self):
        marker = self.root / 'capture_board_replay'
        marker.write_bytes(SENTINEL)
        with patch('reproduce_full_board_references.subprocess.run') as child:
            with self.assertRaises(FileExistsError):
                prepare_and_build(self.root, self.root / 'unused.zip', None)
            child.assert_not_called()
        self.assertEqual(marker.read_bytes(), SENTINEL)

    def test_existing_case_is_untouched(self):
        work = self.root / 'cases/music-05'
        work.mkdir(parents=True)
        marker = work / 'trace.csv'
        marker.write_bytes(SENTINEL)
        with patch('reproduce_full_board_references.subprocess.run') as child:
            with self.assertRaises(FileExistsError):
                run_case(self.root, 'music-05', [{'sample': 0, 'word': 5}], 1)
            child.assert_not_called()
        self.assertEqual(marker.read_bytes(), SENTINEL)
        self.assertEqual(sorted(p.name for p in work.iterdir()), ['trace.csv'])


@unittest.skipUnless(os.environ.get('WF_REFERENCE_CAPTURE') and os.environ.get('WF_REFERENCE_INPUTS'),
                     'Set the private original-ROM executable/input paths for direct producer tests')
class DirectProducerOwnership(unittest.TestCase):
    def test_each_existing_output_is_preserved_before_any_write(self):
        for index in range(4):
            with self.subTest(output_index=index), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                schedule = root / 'schedule.csv'
                schedule.write_text('0,5\n')
                outputs = [root / name for name in ('trace.csv', 'audio.pcm', 'capture.json', 'deliveries.csv')]
                outputs[index].write_bytes(SENTINEL)
                inputs = Path(os.environ['WF_REFERENCE_INPUTS'])
                result = subprocess.run([os.environ['WF_REFERENCE_CAPTURE'], str(inputs / 'SOUND.ROM'),
                    str(inputs / 'SAMPLES.ROM'), str(schedule), '1', *map(str, outputs), 'full'],
                    capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0, 'Direct replay overwrote an existing output')
                self.assertEqual(outputs[index].read_bytes(), SENTINEL)
                for position, path in enumerate(outputs):
                    if position != index:
                        self.assertFalse(path.exists(), 'A new output was opened before detecting the existing one')


if __name__ == '__main__':
    unittest.main()
