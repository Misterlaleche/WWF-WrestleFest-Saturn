"""Focused sequence checks: repeated native images must not hide omissions."""
import unittest
from verify_mednafen_capture import hash_runs, align_runs, timing_analysis


class CaptureVerifierTest(unittest.TestCase):
    def test_refresh_repetitions_preserve_every_visual_occurrence(self):
        reference=hash_runs(list('AABBBAACC'))
        actual=hash_runs(list('AAAABBBBBAAAACCCC'))
        result=align_runs(reference,actual)
        self.assertTrue(result['exact_visual_sequence'])
        self.assertEqual(result['missing_reference_runs'],[])
        self.assertEqual(result['extra_actual_runs'],[])

    def test_repeated_hash_does_not_conceal_missing_visual_state(self):
        result=align_runs(hash_runs(list('ABAC')),hash_runs(list('ABC')))
        self.assertFalse(result['exact_visual_sequence'])
        self.assertEqual(len(result['missing_reference_runs']),1)
        self.assertTrue(result['monotonic_known_progression'])

    def test_reversed_known_states_are_not_monotonic(self):
        result=align_runs(hash_runs(list('ABCD')),hash_runs(list('ACBD')))
        self.assertFalse(result['monotonic_known_progression'])
        self.assertFalse(result['exact_visual_sequence'])

    def test_natural_refresh_quantization_is_tolerated(self):
        import math
        reference=hash_runs(list('AAABBBBCCCCCD'))
        actual=[];pts=[]
        for frame in range(14):
            update=min(12,int(frame*58/60))
            actual.append('AAABBBBCCCCCD'[update])
            pts.append(5+frame/60)
        runs=hash_runs(actual)
        result=timing_analysis(reference,runs,pts,58,1/60,1e-6)
        self.assertTrue(result['all_visual_hold_durations_within_refresh_tolerance'])
        self.assertLessEqual(result['transition_phase_range_seconds'],1/60+1e-6)


if __name__=='__main__':unittest.main()
