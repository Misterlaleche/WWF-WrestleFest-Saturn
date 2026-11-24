import unittest
from rom_subset import CPU, STOP

class Opcodes(unittest.TestCase):
    def run_code(self, code, d=None):
        rom = bytearray(0x80000); blob = bytes.fromhex(code+' 4e75'); rom[0x100:0x100+len(blob)] = blob
        cpu = CPU(rom, bytes(0x4000), d or [0]*8)
        cpu.execute_to(0x100); return cpu

    def test_movem_reversed_mask(self):
        c=self.run_code('48e7 c000 7000 7200 4cdf 0003', [0x12345678,0xabcdef00]+[0]*6)
        self.assertEqual(c.d[:2],[0x12345678,0xabcdef00]);self.assertEqual(c.a[7],0x1d0800)
    def test_immediate_word_preserves_upper(self):
        c=self.run_code('0241 000f 0641 0002 0441 0004', [0,0xfeed0035]+[0]*6)
        self.assertEqual(c.d[1],0xfeed0003)
    def test_mulu_ignores_upper(self):
        c=self.run_code('c2fc 0003',[0,0xfeed0009]+[0]*6);self.assertEqual(c.d[1],27)
    def test_divs_negative_truncation(self):
        c=self.run_code('87fc 0003',[0,0,0,0xfffffff6]+[0]*4);self.assertEqual(c.d[3],0xfffffffd)
    def test_divs_negative_denominator(self):
        c=self.run_code('87fc fffd',[0,0,0,10]+[0]*4);self.assertEqual(c.d[3],0x0001fffd)
    def test_divs_overflow_rejected(self):
        with self.assertRaisesRegex(ValueError,'overflow'):self.run_code('87fc 0001',[0,0,0,0x10000]+[0]*4)
    def test_shift_zero_encoding_means_eight(self):
        c=self.run_code('e18b',[0,0,0,0x12345678]+[0]*4);self.assertEqual(c.d[3],0x34567800)
    def test_rotate_register_count(self):
        c=self.run_code('e2b8',[0x80000001,1]+[0]*6);self.assertEqual(c.d[0],0xc0000000)
    def test_rotate_zero_clears_carry(self):
        c=self.run_code('003c 0001 e2b8',[0x80000001,0]+[0]*6);self.assertFalse(c.c)
    def test_word_neg_carry(self):
        c=self.run_code('4443',[0,0,0,0xabcd0002]+[0]*4);self.assertEqual(c.d[3],0xabcdfffe);self.assertTrue(c.c)
    def test_long_wrap(self):
        c=self.run_code('d081',[0xffffffff,2]+[0]*6);self.assertEqual(c.d[0],1);self.assertTrue(c.c)
    def test_ccr_clear_only_selected(self):
        c=self.run_code('003c 000f 023c 00ee');self.assertFalse(c.c);self.assertTrue(c.z);self.assertTrue(c.n);self.assertTrue(c.v)
    def test_carry_survives_movem(self):
        c=self.run_code('48e7 8000 003c 0001 4cdf 0001');self.assertTrue(c.c)
    def test_unknown_opcode_fails(self):
        with self.assertRaises(ValueError):self.run_code('4afc')

if __name__=='__main__':unittest.main()
