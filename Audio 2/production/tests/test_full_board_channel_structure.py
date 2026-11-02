"""Synthetic score cycles: musical service time is distinct from global/PCM time."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('channel_structure',ROOT/'tools/reference/analyze_full_board_channel_structure.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)


def timeline():
    return [(50*i,i) for i in range(10)]


def channel_a():
    boundaries=[dict(cycle=100*i,tick=2*i) for i in range(4)]
    events=[(10,0,40,50),(60,0,48,0),(111,0,40,50),(161,0,48,0),
            (212,0,40,50),(263,0,48,0)]
    return events,boundaries


class ChannelCycleEvidence(unittest.TestCase):
    def test_own_channel_cycles_do_not_require_global_or_external_cycle_equality(self):
        events,boundaries=channel_a()
        a=module.sequence_evidence(events,boundaries,timeline())
        b_events=[(15,0,41,60),(65,0,49,0),(115,0,8,1),
                  (167,0,41,60),(216,0,49,0),(266,0,8,1),
                  (319,0,41,60),(369,0,49,0),(419,0,8,1)]
        b=module.sequence_evidence(b_events,[dict(cycle=150*i,tick=3*i) for i in range(4)],timeline())
        self.assertTrue(a['score_cycle_observed']);self.assertTrue(b['score_cycle_observed'])
        self.assertEqual(a['cycle_timer_b_services'],2)
        self.assertEqual(b['cycle_timer_b_services'],3)
        self.assertFalse(a['three_internal_external_cycle_sequences_equal'])
        self.assertFalse(b['three_internal_external_cycle_sequences_equal'])
        result=module.classify_channels([a,b],'POINTER_RECURRENCE_ONLY')
        self.assertEqual(result['classification'],'PER_CHANNEL_SCORE_CYCLES_OBSERVED')
        self.assertEqual(result['global_observation'],'POINTER_RECURRENCE_ONLY')
        self.assertFalse(result['global_period_proven']);self.assertFalse(result['pcm_period_proven'])

    def test_changed_value_in_one_of_three_cycles_rejects_complete_repetition(self):
        events,boundaries=channel_a();events[2]=(111,0,40,51)
        result=module.sequence_evidence(events,boundaries,timeline())
        self.assertFalse(result['score_cycle_observed'])
        self.assertFalse(result['all_value_sequences_equal'])

    def test_same_values_at_different_music_services_are_not_the_same_score_cycle(self):
        events,boundaries=channel_a();events[3]=(149,0,48,0)
        result=module.sequence_evidence(events,boundaries,timeline())
        self.assertTrue(result['all_value_sequences_equal'])
        self.assertFalse(result['all_service_sequences_equal'])
        self.assertFalse(result['score_cycle_observed'])

    def test_two_complete_intervals_are_insufficient_for_three_cycle_claim(self):
        events,boundaries=channel_a()
        result=module.sequence_evidence(events,boundaries[:3],timeline())
        self.assertEqual(result['complete_cycles'],2)
        self.assertFalse(result['score_cycle_observed'])

    def test_internal_timing_ignores_checkpoint_offset_but_keeps_real_event_deltas(self):
        boundaries=[dict(cycle=100*i,tick=2*i) for i in range(4)]
        events=[(10,0,40,50),(60,0,48,0),(111,0,40,50),(161,0,48,0),
                (212,0,40,50),(262,0,48,0)]
        result=module.sequence_evidence(events,boundaries,timeline())
        self.assertTrue(result['three_internal_external_cycle_sequences_equal'])
        self.assertFalse(result['three_anchor_external_cycle_sequences_equal'])

    def test_public_export_is_an_allowlist_without_original_state_or_event_values(self):
        private={'schema_version':1,'tool_version':'1.0.0','passed':True,
            'source_sha256':{'tool.py':'a'*64},'input_sha256':{'capture_report':'b'*64},
            'raw_ram':'DO_NOT_EXPORT','trace_records':['DO_NOT_EXPORT'],
            'tracks':[{'command':11,'classification':'PER_CHANNEL_SCORE_CYCLES_OBSERVED',
                'global_observation':'POINTER_RECURRENCE_ONLY','FD_source':'DO_NOT_EXPORT',
                'channels':[{'channel':0,'kind':'FM','score_cycle_observed':True,
                    'cycle_timer_b_services':2,'complete_cycles':3,'events_per_cycle':2,
                    'first_mismatch':{'register':'DO_NOT_EXPORT','value':'DO_NOT_EXPORT'},
                    'raw_state':'DO_NOT_EXPORT'}]}]}
        all_music={'tracks':[{'command':11,'classification':'POINTER_RECURRENCE_ONLY',
                             'natural_global_end':None,'recurrences':{'visible_score':None}}]}
        public=module.public_summary(private,all_music)
        text=json.dumps(public)
        self.assertNotIn('DO_NOT_EXPORT',text)
        self.assertNotIn('trace_records',text);self.assertNotIn('raw_ram',text)
        self.assertEqual(public['tracks'][0]['classification'],'PER_CHANNEL_SCORE_CYCLES_OBSERVED')

    def test_existing_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            out=Path(temporary)/'previous';out.mkdir();sentinel=out/'sentinel';sentinel.write_text('keep')
            with self.assertRaises(FileExistsError):module.prepare_output(out,ROOT)
            self.assertEqual(sentinel.read_text(),'keep')


if __name__=='__main__':unittest.main()
