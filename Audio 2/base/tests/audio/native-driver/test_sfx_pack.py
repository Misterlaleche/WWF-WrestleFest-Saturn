from pathlib import Path
import sys,tempfile,unittest,csv
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools/audio/native_probe'))
from build_sfx_pack import build
class PackTests(unittest.TestCase):
    def test_bad_coverage_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t);(p/'rom').write_bytes(b'\0'*0x80000)
            (p/'manifest').write_text('bank,index\n0,1\n')
            with self.assertRaises(ValueError):build(p/'rom',p/'manifest',p/'missing-model',p/'out')
            self.assertFalse((p/'out').exists())
if __name__=='__main__':unittest.main()
