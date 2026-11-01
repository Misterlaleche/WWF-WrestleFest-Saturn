"""CPU integration checks: real linked code, mock SCSP registers, no audio model."""
import argparse
import json
from pathlib import Path
import struct
import sys
import unittest

BUILD = None


def segments(path):
    data = path.read_bytes()
    if data[:7] != b'\x7fELF\x01\x02\x01':
        raise ValueError('ELF32 BE required')
    h = struct.unpack_from('>HHIIIIIHHHHHH', data, 16)
    if h[0] != 2 or h[1] != 4 or h[6] != 0x1000000:
        raise ValueError('68000 executable required')
    result = []
    for n in range(h[9]):
        p = struct.unpack_from('>8I', data, h[4] + n * h[8])
        if p[0] == 1:
            result.append((p[2], data[p[1]:p[1]+p[4]], p[5]))
    return result


class Firmware(unittest.TestCase):
    def setUp(self):
        from machine68k import Machine, CPUType
        self.m = Machine(CPUType.M68000, 1088)
        self.mem, self.cpu = self.m.mem, self.m.cpu
        self.mem.w_block(0, b'\xa5' * 0x80000)
        for address, data, size in segments(BUILD / 'wf_scsp_driver.elf'):
            # Intentionally leave all NOBITS dirty: startup must clear them.
            self.mem.w_block(address, data)
        self.symbols = json.loads((BUILD / 'symbols.json').read_text())
        self.mem.w16(0xc010, 6631)
        self.mem.w16(0xc012, (-2685) & 0xffff)
        self.cpu.pulse_reset()
        for _ in range(600):
            self.cpu.execute(100000)
            if self.mem.r16(0xc046) in (2, 3):
                break
        self.assertEqual(self.mem.r16(0xc046), 2, 'startup did not reach READY')

    def tearDown(self):
        self.m.cleanup()

    def command(self, cmd, seq=None, argument=0):
        seq = seq if seq is not None else ((self.mem.r16(0xc048)+1) & 0xffff)
        self.mem.w16(0xc04c, cmd)
        self.mem.w16(0xc04e, argument)
        self.mem.w16(0xc048, seq)
        for _ in range(40):
            self.cpu.execute(2000)
            if self.mem.r16(0xc04a) == seq:
                return self.mem.r16(0xc050), self.mem.r16(0xc052)
        self.fail('command was not acknowledged')

    def test_vectors_dirty_bss_and_ram_ownership(self):
        from machine68k import Register
        self.assertEqual(self.mem.r32(0), 0x14000)
        self.assertEqual(self.mem.r32(4), self.symbols['wf_m68k_reset'])
        self.assertTrue(0x12000 <= self.cpu.r_reg(Register.A7) <= 0x14000)
        self.assertEqual(self.mem.r16(0xc008+4), 0x7777)
        self.assertEqual(self.mem.r32(0xc014), self.symbols['pcmCtrlData']+0x25a00000)
        self.assertEqual(self.mem.r_block(0x14000, 0x5c000), b'\xa5'*0x5c000)
        self.assertEqual(self.mem.r_block(0x70000, 6144),
                         (BUILD/'sine-three-cycles.pcm').read_bytes())
        self.assertEqual(self.mem.r32(self.symbols['dataTimers']), 0)

    def test_fm_start_stop_duplicate_and_bad_command(self):
        self.assertEqual(self.command(1)[0], 0)
        for slot in range(4):
            self.assertTrue(self.mem.r16(0x100000+32*slot) & 0x800)
        self.assertEqual(self.mem.r16(0xc054), 1)
        # Same sequence must not run again even if payload is changed.
        self.mem.w16(0xc04c, 2)
        self.cpu.execute(20000)
        self.assertEqual(self.mem.r16(0xc054), 1)
        self.assertEqual(self.command(2)[0], 0)
        self.assertEqual(self.mem.r16(0xc054), 0)
        self.assertEqual(self.command(0xdead)[0], 1)
        self.assertEqual(self.command(1, 0xffff)[0], 0)
        self.assertEqual(self.command(2, 0)[0], 0)

    def test_pcm_allocator_full_never_steals_reserved_slots(self):
        p = self.symbols['pcmCtrlData']
        for i in range(93):
            a = p + i*20
            self.mem.w8(a, 1)       # looping PCM
            self.mem.w8(a+1, 1)     # 8 bit
            self.mem.w16(a+2, 1)
            self.mem.w16(a+4, 0x4000)
            self.mem.w16(a+8, 1000)
            self.mem.w8(a+13, 128)
            self.mem.w8(a+18, 1)
        self.command(1)
        self.mem.w16(0xc00c, 1)
        self.cpu.execute(250000)
        targets = [self.mem.r8(p+i*20+19) for i in range(93)]
        assigned = [v for v in targets if v != 255]
        self.assertEqual(len(assigned), 25)
        self.assertEqual(len(set(assigned)), 25)
        self.assertTrue(all(v not in (0,1,2,3,16,17,31) for v in assigned))
        self.assertEqual(self.command(2)[0], 0)
        self.assertEqual(self.mem.r16(0xc046), 2)

    def test_slot31_and_fm_can_coexist(self):
        self.assertEqual(self.command(3)[0], 0)  # prepare exact R04 ring
        base = 0x100000+31*32
        self.assertEqual([self.mem.r16(base+x) for x in (2,4,6,16)], [0,0,32767,0])
        self.assertEqual(self.command(4)[0], 0)
        self.command(1)
        self.assertTrue(self.mem.r16(base) & 0x800)
        self.command(2)
        self.assertTrue(self.mem.r16(base) & 0x800)
        # Mock monitor response; CPU test does not emulate SCSP latch/audio.
        self.mem.w16(0x100408, 5 << 7)
        self.assertEqual(self.command(6), (0, 5))
        self.assertEqual(self.command(5)[0], 0)
        self.assertFalse(self.mem.r16(base) & 0x800)

    def test_arcade_words_are_acknowledged_without_fabricated_fm(self):
        self.assertEqual(self.mem.r16(0xc044), 3)
        self.assertEqual(self.command(7, argument=0x3102), (3, 0x3102))
        self.assertEqual(self.mem.r16(0xc05a), 0x3102)
        self.assertEqual(self.mem.r16(0xc05c), 1)
        self.assertEqual(self.mem.r16(0xc054), 0)
        self.assertEqual(self.command(7, argument=0x3132), (3, 0x3132))
        self.assertEqual(self.command(7, argument=0x311f), (3, 0x311f))
        self.command(1)
        self.assertEqual(self.mem.r16(0xc054), 1)
        self.assertEqual(self.command(7, argument=0x3100), (0, 0x3100))
        self.assertEqual(self.mem.r16(0xc054), 0)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--build', required=True, type=Path)
    a, rest = p.parse_known_args()
    BUILD = a.build
    unittest.main(argv=[sys.argv[0], *rest])
