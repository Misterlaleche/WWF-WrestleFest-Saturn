"""Hardware-format regressions independent of the historical MAME DSP decoder."""
import importlib.util,os,unittest
from pathlib import Path
path=Path(os.environ.get('WF_DSP_ASSEMBLER',Path(__file__).resolve().parents[3]/'tools/audio/native_probe/build_dsp_mixer.py'))
spec=importlib.util.spec_from_file_location('assembler',path);assembler=importlib.util.module_from_spec(spec);spec.loader.exec_module(assembler)
class DSPFormat(unittest.TestCase):
 def test_linear_memory_flag_uses_hardware_bit8(self):
  instruction=assembler.instruction(nofl=1)
  self.assertEqual(instruction[3]&0x0100,0x0100)
  self.assertFalse(instruction[3]&0x8000,'word3 bit15 is reserved')
 def test_clock_write_and_all_pcm_reads_are_linear(self):
  program=assembler.build_program();linear=[w for w in program if w[2]&0x6000 and w[3]&0x0100]
  self.assertEqual(len(linear),9)
  self.assertEqual(sum(bool(w[2]&0x4000) for w in linear),1)
  self.assertTrue(all(not w[3]&0x8080 for w in program),'reserved memory-format bits must be zero')
if __name__=='__main__':unittest.main()
