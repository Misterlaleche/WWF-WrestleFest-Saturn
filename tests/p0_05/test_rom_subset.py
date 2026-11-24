"""Literal NOP controls: only PC/trace may change, unknown instructions fail."""
import unittest
from rom_subset import CPU
class NopTests(unittest.TestCase):
    def machine(self):
        rom=bytearray(0x80000);rom[0x100:0x106]=bytes.fromhex('4e714e75ffff')
        cpu=CPU(bytes(rom),bytes(0x4000),[0x12340000+i for i in range(8)])
        cpu.pc=0x100;return cpu
    def test_nop_preserves_registers_flags_and_all_memory(self):
        c=self.machine();c.n=True;c.z=False;c.v=True;c.c=True
        before=(list(c.d),list(c.a),c.n,c.z,c.v,c.c,bytes(c.ram),bytes(c.stack),c.rom,list(c.rom_writes))
        c.step()
        self.assertEqual(c.pc,0x102);self.assertEqual(c.trace,[0x100])
        self.assertEqual(before,(c.d,c.a,c.n,c.z,c.v,c.c,bytes(c.ram),bytes(c.stack),c.rom,c.rom_writes))
    def test_nop_then_rts_returns_to_sentinel(self):
        c=self.machine();ram,d=c.execute_to(0x100,limit=3)
        self.assertEqual(c.pc,0x70000);self.assertEqual(c.trace,[0x100,0x102])
        self.assertEqual(ram,bytes(0x4000));self.assertEqual(d,[0x12340000+i for i in range(8)])
    def test_unknown_instruction_is_not_silently_treated_as_nop(self):
        c=self.machine();c.pc=0x104
        with self.assertRaises(ValueError):c.step()
if __name__=='__main__':unittest.main()
