"""Literal BCHG tests: catches bit width, previous Z, untouched flags/registers."""
import unittest
from rom_subset import CPU

class DynamicBCHG(unittest.TestCase):
    def cpu(self, code, d1, byte=0, d0=0):
        rom=bytearray(0x80000);blob=bytes.fromhex(code+' 4e75');rom[0x100:0x100+len(blob)]=blob
        ram=bytearray(0x4000);ram[0x612]=byte;ram[0x613]=0xa5
        c=CPU(rom,ram,[d0,d1,0x1234,0x5678,0,0,0,0]);c.a[0]=0x1c05b0
        c.n=c.v=c.c=True;c.z=False
        c.execute_to(0x100);return c
    def test_byte_old_clear(self):
        c=self.cpu('0368 0062',1,0x80)
        self.assertEqual(c.ram[0x612:0x614],bytes([0x82,0xa5]));self.assertTrue(c.z)
        self.assertTrue(c.n and c.v and c.c);self.assertEqual(c.d[:4],[0,1,0x1234,0x5678])
    def test_byte_old_set(self):
        c=self.cpu('0368 0062',2,0xff)
        self.assertEqual(c.ram[0x612],0xfb);self.assertFalse(c.z)
    def test_byte_modulo_eight(self):
        c=self.cpu('0368 0062',0x12340009,0x10)
        self.assertEqual(c.ram[0x612],0x12);self.assertTrue(c.z)
    def test_long_modulo_thirty_two(self):
        c=self.cpu('0340',0xffffffff,d0=1)
        self.assertEqual(c.d[0],0x80000001);self.assertTrue(c.z)
        self.assertTrue(c.n and c.v and c.c)
    def test_toggle_twice(self):
        c=self.cpu('0368 0062 0368 0062',0,0xfe)
        self.assertEqual(c.ram[0x612],0xfe);self.assertFalse(c.z)
if __name__=='__main__':unittest.main()
