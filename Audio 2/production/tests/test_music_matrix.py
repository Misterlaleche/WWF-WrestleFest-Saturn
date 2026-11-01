"""Control-schedule and tail-observation tests; no ROM/audio fixtures."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import run_music_matrix as matrix

class LongMatrixChecks(unittest.TestCase):
    def test_long_cases_include_original_fade_and_full_tail(self):
        cases=matrix.make_cases(1,18,35,600,8,True)
        self.assertEqual(len(cases),19)
        self.assertEqual(cases[0]['seconds'],608)
        self.assertEqual(cases[0]['events'],[{'sample':0,'word':1},{'sample':600*44100,'word':0x1f}])
        self.assertEqual(cases[-1]['seconds'],32)
        self.assertEqual([e['word'] for e in cases[-1]['events']],[1,6,0x12,0x1f,3,0x1f,2,1,0x1f])

    def test_default_short_matrix_schedule_is_unchanged(self):
        cases=matrix.make_cases(1,18,35)
        self.assertEqual(len(cases),18)
        self.assertTrue(all(c['seconds']==35 and c['events'] is None for c in cases))

    def test_accepted_count_does_not_hide_wrong_word_or_early_command(self):
        events=[{'sample':0,'word':1},{'sample':44100,'word':0x1f}]
        good=matrix.inspect_replay_rows([(1,0,60,0),(0x1f,44120,120,0)],events)
        self.assertTrue(good['passed'])
        self.assertEqual(good['maximum_admission_delay_samples'],20)
        self.assertFalse(matrix.inspect_replay_rows([(1,0,60,0),(2,44120,120,0)],events)['passed'])
        self.assertFalse(matrix.inspect_replay_rows([(1,0,60,0),(0x1f,44099,120,0)],events)['passed'])

    def test_busy_or_unbounded_admission_is_not_complete(self):
        events=[{'sample':0,'word':1},{'sample':44100,'word':0x1f}]
        self.assertFalse(matrix.inspect_replay_rows([(1,0,60,0),(0x1f,44100,120,1)],events)['passed'])
        self.assertFalse(matrix.inspect_replay_rows([(1,0,60,0),(0x1f,46000,120,0)],events)['passed'])

if __name__=='__main__':unittest.main()
