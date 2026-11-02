"""Counterexamples preventing stronger claims than the coupled evidence shows."""
import importlib.util
from pathlib import Path
import unittest

PATH=Path(__file__).resolve().parents[1]/'tools/reference/analyze_full_board_music_structure.py'
spec=importlib.util.spec_from_file_location('full_board_music_structure',PATH)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def state(**changes):
    ram=bytearray(2048);ram[6]=1;ram[7]=1
    fields=dict(canonical=1,cycle=100,tick=1,ym=1,oki_events=0,keyons=1,
        last_audio_cycle=90,global_ends=0,channel_ends=(0,)*10,back_jumps=(0,)*10,
        first_end_cycle=0,fade_sent_cycle=0,fade_clear_cycle=0,
        ram=bytes(ram),visible=bytes(264),cpu=b'CPU R=0',io=bytes(8),
        phases=bytes(24),opm=b'envelope and oscillator phase A',
        oki=b'ADPCM position A',output=bytes(8),pointers=bytes(20))
    fields.update(changes)
    return fields


class CoupledClaimCounterexamples(unittest.TestCase):
    def test_fb_plus_inactive_before_fade_is_a_natural_end(self):
        s=state(global_ends=1,first_end_cycle=50,fade_sent_cycle=200)
        ram=bytearray(s['ram']);ram[6]=0;s['ram']=bytes(ram)
        self.assertTrue(module.natural_end_observed(s))

    def test_silence_or_fb_after_fade_does_not_prove_natural_end(self):
        ram=bytes(2048)
        self.assertFalse(module.natural_end_observed(state(ram=ram)))
        self.assertFalse(module.natural_end_observed(state(ram=ram,global_ends=1,
            first_end_cycle=300,fade_sent_cycle=200)))

    def test_cpu_refresh_does_not_block_visible_score_evidence(self):
        a,b=state(),state(cpu=b'CPU R=1')
        self.assertEqual(module.recurrence_key(a,'visible_score'),module.recurrence_key(b,'visible_score'))
        self.assertNotEqual(module.recurrence_key(a,'full_digital'),module.recurrence_key(b,'full_digital'))

    def test_pointer_equality_does_not_hide_a_changed_countdown(self):
        ram=bytearray(state()['ram']);ram[0x100]=1
        a,b=state(),state(ram=bytes(ram))
        self.assertEqual(module.recurrence_key(a,'pointers'),module.recurrence_key(b,'pointers'))
        self.assertNotEqual(module.recurrence_key(a,'ram'),module.recurrence_key(b,'ram'))

    def test_visible_equality_does_not_hide_synth_phase_or_adpcm_position(self):
        a=state()
        for changes in ({'opm':b'phase B'},{'oki':b'ADPCM position B'},
                        {'phases':b'different output sample phase'}):
            with self.subTest(changes=changes):
                b=state(**changes)
                self.assertEqual(module.recurrence_key(a,'visible_score'),module.recurrence_key(b,'visible_score'))
                self.assertNotEqual(module.recurrence_key(a,'full_digital'),module.recurrence_key(b,'full_digital'))

    def test_backward_fd_alone_is_only_backward_branch_evidence(self):
        result=module.analyze_states([state(back_jumps=(1,)+(0,)*9)],fade_cycle=1000)
        self.assertEqual(result['classification'],'BACKWARD_FD_BRANCH_OBSERVED')
        self.assertFalse(result['complete_composition_loop_proven'])
        self.assertFalse(result['pcm_loop_proven'])

    def test_idle_duplicate_without_additional_key_writes_is_not_recurrence(self):
        result=module.analyze_states([state(),state(cycle=200)],fade_cycle=1000)
        self.assertIsNone(result['recurrences']['visible_score'])

    def test_arbitrary_final_and_fade_tail_states_cannot_supply_recurrence(self):
        for second in (state(canonical=0,cycle=200,keyons=2),
                       state(cycle=1200,keyons=2),state(cycle=200,keyons=2,fade_sent_cycle=150)):
            with self.subTest(canonical=second['canonical'],cycle=second['cycle']):
                result=module.analyze_states([state(),second],fade_cycle=1000)
                self.assertIsNone(result['recurrences']['visible_score'])

    def test_equal_visible_score_with_new_notes_survives_refresh_difference(self):
        result=module.analyze_states([state(),state(cycle=500,tick=5,keyons=2,cpu=b'CPU R=1')],fade_cycle=1000)
        match=result['recurrences']['visible_score']
        self.assertIsNotNone(match)
        self.assertEqual(match['period_cycles'],400)
        self.assertEqual(match['positive_key_writes_between'],1)
        self.assertIsNone(result['recurrences']['full_digital'])
        self.assertEqual(result['classification'],'VISIBLE_SCORE_STATE_RECURRENCE')
        self.assertFalse(result['complete_composition_loop_proven'])
        self.assertFalse(result['pcm_loop_proven'])


if __name__=='__main__':unittest.main()
