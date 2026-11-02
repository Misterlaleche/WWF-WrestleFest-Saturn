"""The coupled capture's ORIGIN does not carry the command ID; use its manifest."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / 'tools/reference/audit_ym_usage.py'


class FullBoardUsage(unittest.TestCase):
    def invoke(self, tamper=False):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        trace = root / 'trace.csv'
        trace.write_text('cycle,kind,a,b\n1,YM_D,15,0\n2,ORIGIN,0,0\n3,YM_D,32,40\n4,YM_D,8,120\n3579547,END,0,0\n')
        identity = hashlib.sha256(trace.read_bytes()).hexdigest()
        report = root / 'reference.json'
        report.write_text(json.dumps({'profile': 'full-board', 'complete': True,
            'tracks': [{'command': 7, 'trace': str(trace), 'output_sha256': {'trace': '0' * 64 if tamper else identity}}]}))
        output = root / 'audit.json'
        result = subprocess.run([sys.executable, str(TOOL), '--full-board-report', str(report), '--out', str(output)],
                                text=True, capture_output=True)
        return result, output

    def test_manifest_command_overrides_empty_origin_field(self):
        result, output = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(output.read_text())
        self.assertEqual(report['profile'], 'full-board')
        self.assertEqual(report['tracks'][0]['command'], 7)
        self.assertEqual(report['tracks'][0]['feedback_at_keyon'], {'5': 1})

    def test_changed_trace_is_rejected(self):
        result, output = self.invoke(tamper=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
