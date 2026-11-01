"""PCM/FM regression using the explicit hardware-format MAME decoder adapter.

The unmodified historical decoder uses reserved NOFL15 and unsigned ADRS, so
must not be used as an oracle for the production hardware-format program.
"""
import ctypes as C
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[3]/'tools/audio/native_probe'))
from build_dsp_mixer import build_program
LIB=None
class Mixer(unittest.TestCase):
    def setUp(self):
        self.d=C.CDLL(str(LIB));self.d.dsp_reset()
        self.d.dsp_ram_read.restype=C.c_uint16
        self.d.dsp_output.restype=C.c_int16
        p=build_program();self.assertLessEqual(len(p),128)
        for i,words in enumerate(p):
            for j,w in enumerate(words):self.d.dsp_program(i*4+j,w)
        self.d.dsp_start()
        self.d.dsp_coef(0,0x8000)
        self.d.dsp_madrs(0,0xb012//2);self.d.dsp_madrs(1,0xb014//2);self.d.dsp_madrs(2,0xb010//2)
        # DSP float16 exact encodings of -256 and -743, checked by the oracle.
        self.d.dsp_ram_write(0xb012,0xe700);self.d.dsp_ram_write(0xb014,0xe519)
        self.d.dsp_ram_write(0xb016,0x8000);self.d.dsp_madrs(7,0xb016//2)
        for v in range(4):self.d.dsp_madrs(3+v,(0x3000+v*8194)//2+2048)
        for _ in range(4):self.d.dsp_step()
        self.assertEqual(self.d.dsp_ram_read(0xb010),1)
        self.d.dsp_coef(9,0x8000)
    def test_four_constant_voices_and_counter(self):
        values=[1234,-4567,12345,-23456]
        for v,s in enumerate(values):
            for n in range(4097):self.d.dsp_ram_write(0x3000+v*8194+n*2,s&65535)
            self.d.dsp_coef(1+v,0x8000);self.d.dsp_coef(5+v,0x8000)
        for n in range(1000):
            self.d.dsp_step()
            self.assertEqual(self.d.dsp_ram_read(0xb010),(n+2)&65535)
            for v,s in enumerate(values):self.assertAlmostEqual(self.d.dsp_output(v),s,delta=1)
    def test_interpolation_wrap_and_all_volumes(self):
        wave=[((i*101+12345)&65535)-32768 for i in range(4096)]
        for n,s in enumerate(wave+[wave[0]]):self.d.dsp_ram_write(0x3000+n*2,s&65535)
        self.d.dsp_coef(1,0x8000)
        amplitudes=[32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0]
        phase=743
        for n in range(70000):
            vol=(n//4300)%16;self.d.dsp_coef(5,(-amplitudes[vol]*128*8)&65535)
            phase=(phase+743)&0xffffff
            self.d.dsp_step();index=phase>>12;fraction=phase&4095
            expected=(wave[index]*(4096-fraction)+wave[(index+1)&4095]*fraction)*amplitudes[vol]//(4096*32)
            self.assertAlmostEqual(self.d.dsp_output(0),expected,delta=1)
        self.assertEqual(self.d.dsp_ram_read(0xb010),(70001&65535))
    def test_eight_fm_buses_simultaneously_with_pcm(self):
        self.d.dsp_coef(10,21627)
        for n in range(100):
            for ch in range(8):self.d.dsp_input(8+ch,(ch*1000-4000)*16)
            self.d.dsp_step()
            for ch in range(8):self.assertAlmostEqual(self.d.dsp_output(4+ch),(ch*1000-4000)*21627//32768,delta=1)
if __name__=='__main__':
    LIB=Path(sys.argv.pop(1)).resolve();unittest.main()
