import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from platform_evidence import ClockEvidence
class ClockEvidenceTest(unittest.TestCase):
 def sample(self,**kw):
  s=dict(boundary_stable=True,wf_state=1,wf_logic_sequence=2,wf_logic_fields=39168,wf_logic_ticks=37609,wf_logic_pending=0)
  s.update(kw);return s
 def test_late_clock_corruption_fails_after_correct_samples(self):
  c=ClockEvidence();c.add(self.sample())
  with self.assertRaises(ValueError):c.add(self.sample(wf_logic_sequence=4,wf_logic_pending=1))
 def test_count_distinct_active_publications_not_frozen_terminal(self):
  c=ClockEvidence()
  for _ in range(100):c.add(self.sample())
  for _ in range(100):c.add(self.sample(wf_state=5,wf_logic_sequence=4))
  self.assertEqual(c.count,1)
 def test_preserved_debt_and_torn_publication(self):
  c=ClockEvidence();c.add(self.sample(wf_logic_ticks=37607,wf_logic_pending=2))
  c.add(self.sample(wf_logic_sequence=3,wf_logic_pending=100))
  c.add(self.sample(boundary_stable=False,wf_logic_sequence=4,wf_logic_pending=100))
  self.assertEqual((c.count,c.max_pending),(1,2))
