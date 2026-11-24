"""Instruction-level checks for the limited test oracle, not a CPU qualification suite."""
import unittest
from rom_subset import CPU, REF, RAM, STOP, signed


class SubsetTests(unittest.TestCase):
    def cpu(self, code):
        rom = bytearray(0x80000)
        rom[0x100:0x100+len(bytes.fromhex(code))] = bytes.fromhex(code)
        c = CPU(rom, bytes(0x4000), [0xaabbccdd] * 8)
        c.pc = 0x100
        return c

    def test_move_word_keeps_high_bits_and_sets_flags(self):
        c = self.cpu('303c 8001 4e75'); c.run(0x100)
        self.assertEqual(c.d[0], 0xaabb8001); self.assertTrue(c.n); self.assertFalse(c.z)

    def test_movea_word_sign_extension_and_flags_unchanged(self):
        c = self.cpu('327c 8001 4e75'); c.z = True; c.run(0x100)
        self.assertEqual(c.a[1], 0xffff8001); self.assertTrue(c.z)

    def test_moveq_sign_extends(self):
        c = self.cpu('7aff 4e75'); c.run(0x100)
        self.assertEqual(c.d[5], 0xffffffff); self.assertTrue(c.n)

    def test_clr_clears_whole_byte(self):
        c = self.cpu('4228 002e 4e75'); c.write(REF+0x2e, 1, 0x7f); c.run(0x100)
        self.assertEqual(c.read(REF+0x2e, 1), 0); self.assertTrue(c.z)

    def test_bset_bchg_bclr_and_btst_change_only_zero_flag(self):
        for op, expected in [('08e8', 0x81), ('0868', 0x81), ('08a8', 0x01), ('0828', 0x01)]:
            c = self.cpu(op+' 0007 002e 4e75'); c.write(REF+0x2e, 1, 1)
            c.c = c.n = c.v = True; c.run(0x100)
            self.assertEqual(c.read(REF+0x2e, 1), expected); self.assertTrue(c.z)
            self.assertTrue(c.c and c.n and c.v)

    def test_branch_word_relative_base(self):
        c = self.cpu('6000 0006 7001 7002 7003 4e75'); c.run(0x100)
        self.assertEqual(c.d[0], 3); self.assertNotIn(0x104, c.trace)

    def test_bsr_returns_to_after_extension(self):
        c = self.cpu('6100 0006 7001 4e75 7202 4e75'); c.run(0x100)
        self.assertEqual(c.d[:2], [1, 2]); self.assertEqual(c.a[7], 0x1d0800)

    def test_dbra_preserves_upper_word_and_flags(self):
        c = self.cpu('51cc fffe 4e75'); c.d[4] = 0x12340002; c.c = c.z = True; c.run(0x100)
        self.assertEqual(c.d[4], 0x1234ffff); self.assertEqual(c.trace.count(0x100), 3)
        self.assertTrue(c.c and c.z)

    def test_tagged_memory_and_rom_writes(self):
        c = self.cpu('4e75'); c.write(0x801c0034, 1, 0x12)
        self.assertEqual(c.read(0xff1c0034, 1), 0x12)
        c.write(0x80000034, 1, 0x55)
        self.assertEqual(c.read(0x34, 1), 0); self.assertEqual(c.rom_writes, [[0x34, 1, 0x55]])

    def test_cmpa_retains_tag_bits(self):
        c = self.cpu('b3fc 001c05b0 4e75'); c.a[1] = 0x801c05b0; c.run(0x100)
        self.assertFalse(c.z)
        c = self.cpu('b3fc 801c05b0 4e75'); c.a[1] = 0x801c05b0; c.run(0x100)
        self.assertTrue(c.z)

    def test_unsigned_cmp_word_carry(self):
        for left, right in [(0, 65535), (65535, 0), (0x8000, 0x7fff), (37, 37)]:
            c = self.cpu(f'b07c {right:04x} 4e75'); c.d[0] = 0x87650000 | left; c.run(0x100)
            self.assertEqual(c.c, left < right); self.assertEqual(c.z, left == right)
            self.assertEqual(c.d[0], 0x87650000 | left)

    def test_subq_byte_wrap(self):
        c = self.cpu('5328 0023 4e75'); c.run(0x100)
        self.assertEqual(c.read(REF+0x23, 1), 255); self.assertTrue(c.c)

    def test_adda_keeps_flags_and_32_bit_value(self):
        c = self.cpu('d9fc 0000010c 4e75'); c.a[4] = 0x801c05b0; c.z = True; c.run(0x100)
        self.assertEqual(c.a[4], 0x801c06bc); self.assertTrue(c.z)

    def test_index_word_is_signed_and_long_is_full_width(self):
        c = self.cpu('2673 00fc 4e75'); c.a[3] = REF+8; c.d[0] = 0xabcdfffe
        c.write(REF+2, 4, 0x801c05b0); c.run(0x100)
        self.assertEqual(c.a[3], 0x801c05b0)
        c = self.cpu('2673 0800 4e75'); c.a[3] = REF; c.d[0] = 4
        c.write(REF+4, 4, 0xff1c05b0); c.run(0x100)
        self.assertEqual(c.a[3], 0xff1c05b0)

    def test_jsr_indirect_and_absolute_return(self):
        c = self.cpu('4e93 7003 4e75 7205 4e75'); c.a[3] = 0x106; c.run(0x100)
        self.assertEqual(c.d[:2], [3, 5]); self.assertEqual(c.a[7], 0x1d0800)
        c = self.cpu('4eb9 0000010a 7003 4e75 7205 4e75'); c.run(0x100)
        self.assertEqual(c.d[:2], [3, 5]); self.assertEqual(c.a[7], 0x1d0800)

    def test_lsl_word_two_preserves_high_and_carry(self):
        for low, result, carry in ((9, 36, False), (0x4000, 0, True), (0x8000, 0, False), (0xffff, 0xfffc, True)):
            c = self.cpu('e540 4e75'); c.d[0] = 0xabcd0000|low; c.run(0x100)
            self.assertEqual(c.d[0], 0xabcd0000|result)
            self.assertEqual(c.c, carry); self.assertFalse(c.v)
            self.assertEqual(c.z, result == 0)

    def test_fail_closed(self):
        with self.assertRaises(ValueError): self.cpu('ffff').run(0x100)
        with self.assertRaises(ValueError): self.cpu('4e75').read(1, 2)
        with self.assertRaises(ValueError): self.cpu('4e75').read(0x90000, 1)
        with self.assertRaises(ValueError): self.cpu('60fe').run(0x100, limit=5)


if __name__ == '__main__':
    unittest.main()
