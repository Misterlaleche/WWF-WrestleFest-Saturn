"""Regression: a 2-VBlank gameplay frame must fail the default release gate."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

class GateTests(unittest.TestCase):
    def run_gate(self, vblanks, frames=300, reads=0, extra=()):
        data = dict(frame=600, profile_frames=frames, profile_vblanks=vblanks,
                    fatal_owner=0, fatal_dependency=0,
                    profile_calls=[0]*7+[reads, 0], profile_totals=[0]*9)
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'profile.json'
            path.write_text(json.dumps(data))
            return subprocess.run([sys.executable, str(Path(__file__).with_name('check-runtime18-profile.py')),
                str(path), '--start', '300', '--stop', '600', *extra], capture_output=True, text=True)

    def test_default_rejects_half_rate(self):
        self.assertNotEqual(self.run_gate(600).returncode, 0)

    def test_full_rate_passes(self):
        self.assertEqual(self.run_gate(300).returncode, 0)

    def test_io_rejected_even_at_full_rate(self):
        self.assertNotEqual(self.run_gate(300, reads=1).returncode, 0)

    def test_explicit_diagnostic_threshold(self):
        self.assertEqual(self.run_gate(600, extra=('--max-vblanks-per-frame','2')).returncode, 0)

    def test_incomplete_window_rejected(self):
        self.assertNotEqual(self.run_gate(300, frames=299).returncode, 0)

if __name__=='__main__': unittest.main()
