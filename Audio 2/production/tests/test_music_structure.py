"""Counterexamples for musical-state closure claims; no ROM fixtures."""
import importlib.util
from pathlib import Path
import unittest

path=Path(__file__).resolve().parents[1]/'tools/reference/analyze_music_structure.py'
spec=importlib.util.spec_from_file_location('music_structure',path)
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def state(**changes):
    fields=dict(ram=b'complete original RAM',visible=b'YM registers and keys',
                cpu=b'CPU registers including R',io=b'IO latch and timer control',
                timer=b'relative next timer deadline',pointers=b'original pointers')
    fields.update(changes)
    return fields

class ClosureClaims(unittest.TestCase):
    def test_ram_match_different_timer_is_not_exact_control_loop(self):
        a,b=state(),state(timer=b'different relative deadline')
        self.assertEqual(module.recurrence_key(a,'ram'),module.recurrence_key(b,'ram'))
        self.assertNotEqual(module.recurrence_key(a,'exact_control'),module.recurrence_key(b,'exact_control'))

    def test_visible_score_match_different_cpu_is_not_exact_control_loop(self):
        a,b=state(),state(cpu=b'different refresh register')
        self.assertEqual(module.recurrence_key(a,'visible_score'),module.recurrence_key(b,'visible_score'))
        self.assertNotEqual(module.recurrence_key(a,'exact_control'),module.recurrence_key(b,'exact_control'))

    def test_pointer_match_different_countdown_is_only_pointer_match(self):
        a,b=state(),state(ram=b'one changed original countdown byte')
        self.assertEqual(module.recurrence_key(a,'pointers'),module.recurrence_key(b,'pointers'))
        self.assertNotEqual(module.recurrence_key(a,'ram'),module.recurrence_key(b,'ram'))

    def test_long_silence_is_not_an_original_end_handler(self):
        self.assertFalse(module.original_end_observed(global_end_count=0,music_active=1))
        self.assertFalse(module.original_end_observed(global_end_count=0,music_active=0))
        self.assertTrue(module.original_end_observed(global_end_count=1,music_active=0))

if __name__=='__main__':unittest.main()
