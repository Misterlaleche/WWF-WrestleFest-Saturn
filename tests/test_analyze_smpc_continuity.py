#!/usr/bin/env python3
"""Reject missing/early/incomplete traffic in the preserved real PAL capture."""
import copy
import csv
import gzip
import unittest
from pathlib import Path

from analyze_smpc_continuity import analyze, read_json


class ObserverAuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        evidence = Path(__file__).resolve().parents[1] / 'evidence/smpc-continuity-20261113'
        with gzip.open(evidence / 'pal-tag.csv.gz', 'rt') as handle:
            cls.rows = list(csv.DictReader(handle))
        cls.capture = read_json(evidence / 'pal-tag-capture.json.gz')
        cls.start = 1856
        cls.index = next(i for i, row in enumerate(cls.rows)
                         if row['kind'] == 'C' and row['value'] == '16'
                         and int(row['field']) == cls.start + 10)

    def audit(self, rows):
        return analyze(rows, self.capture, self.start)

    def test_preserved_capture(self):
        result = self.audit(self.rows)
        self.assertTrue(result['pass'])
        self.assertEqual(result['runtime']['fields'], 4200)

    def test_missing_query(self):
        self.assertFalse(self.audit(self.rows[:self.index] + self.rows[self.index + 1:])['pass'])

    def test_stopped_queries_cannot_shorten_scope(self):
        rows = [row for row in self.rows if not (row['kind'] == 'C' and row['value'] == '16'
                and int(row['field']) >= self.start + 10)]
        result = self.audit(rows)
        self.assertFalse(result['pass'])
        self.assertEqual(result['runtime']['end_field'], 6055)

    def test_wrong_protocol(self):
        rows = copy.deepcopy(self.rows)
        rows[self.index]['ireg0'] = '1'
        self.assertFalse(self.audit(rows)['pass'])

    def test_early_write(self):
        rows = copy.deepcopy(self.rows)
        rows[self.index]['delta_master'] = '0'
        self.assertFalse(self.audit(rows)['pass'])

    def test_incomplete_packet(self):
        rows = copy.deepcopy(self.rows)
        next(row for row in rows if row['kind'] == 'B'
             and int(row['field']) == self.start + 11)['sr'] = '111'
        self.assertFalse(self.audit(rows)['pass'])


if __name__ == '__main__':
    unittest.main()
