#!/usr/bin/env python3
"""Real /proc read tests against an owned synthetic RAM process, not an emulator."""

import fcntl
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

import observer


HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
TEST_ROOT = PROJECT / 'research/build/mednafen/observer-tests'
PROGRAM = bytes((i * 73 + (i >> 7) * 29 + 17) & 255 for i in range(8192))
SYMBOLS = dict(video_clock=0x06080000, wp1980_started=0x06080004,
               wp1980_draws=0x06080008, wp1980_vblanks=0x0608000c,
               rt=0x06090000, odd=0x06081001)
LAYOUT = PROJECT / 'verification/actors-layout.bin'
FIXTURE = r'''
import json,mmap,os,struct,sys
from pathlib import Path
program=bytes((i*73+(i>>7)*29+17)&255 for i in range(8192))
ram=mmap.mmap(-1,0x100000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS)
def write(address,data):
    offset=address-0x06000000
    for i,value in enumerate(data):ram[(offset+i)^1]=value
write(0x06004000,program)
clock=1234
write(0x06080000,struct.pack('>4I',clock,1,77,78))
write(0x06081000,bytes.fromhex('10 21 32 43 54 65 76 87 98'))
write(0x06090000+24,struct.pack('>I',77))
write(0x06090000+352,bytes.fromhex('12 34 56 78'))
for slot in range(9):write(0x06090000+384+slot*268,bytes((slot*17+i*3)&255 for i in range(268)))
second=None
if len(sys.argv)>1 and sys.argv[1]=='two':
    second=mmap.mmap(-1,0x100000,flags=mmap.MAP_PRIVATE|mmap.MAP_ANONYMOUS)
    for i,value in enumerate(program):second[0x4000+(i^1)]=value
print(json.dumps({'ready':True,'proc_pid':int(Path('/proc/self/stat').read_text().split(' ',1)[0])}),flush=True)
for line in sys.stdin:
    if line.startswith('advance '):
        clock+=int(line.split()[1]);write(0x06080000,struct.pack('>I',clock));print('OK',flush=True)
'''


class OwnedFixture:
    def __init__(self, two=False):
        self.process = subprocess.Popen([sys.executable, '-u', '-c', FIXTURE, *(['two'] if two else [])],
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True)
        ready = json.loads(self.process.stdout.readline())
        if not ready['ready']:
            raise RuntimeError(self.process.stderr.read())
        self.proc_pid = ready['proc_pid']

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=5)
        self.process.stdout.close()
        self.process.stderr.close()


class ObserverTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        TEST_ROOT.mkdir(parents=True, exist_ok=True)

    def test_live_read_only_process_endianness_actors_and_dump(self):
        fixture = OwnedFixture()
        try:
            with observer.ReadOnlyObserver(fixture.process.pid, PROGRAM, proc_pid=fixture.proc_pid) as obs:
                identity = obs.locate()
                self.assertEqual(identity['instruction_match_bytes'], 8192)
                self.assertEqual(fcntl.fcntl(obs.fd, fcntl.F_GETFL) & os.O_ACCMODE, os.O_RDONLY)
                watches = observer.build_watches(SYMBOLS, LAYOUT,
                    ['odd3:odd:3', 'odd5:odd:5', 'alias:0x26081001:2'])
                self.assertEqual(len([w for w in watches if w['name'].startswith('actor')]), 9)
                sample = obs.sample(watches)
                self.assertEqual(sample['video_clock'], 1234)
                self.assertEqual(sample['stable_video_clock_before'], 1234)
                self.assertEqual(sample['stable_video_clock_after'], 1234)
                self.assertTrue(sample['boundary_stable'])
                self.assertEqual(sample['game_frame'], 77)
                self.assertEqual(sample['wp1980_draws'], 77)
                self.assertEqual(sample['rng_state'], 0x12345678)
                self.assertEqual(sample['odd3'], 0x213243)
                self.assertEqual(sample['odd5'], 0x2132435465)
                self.assertEqual(sample['alias'], 0x2132)
                self.assertEqual(obs.read_guest(0x06081001, 8), bytes.fromhex('21 32 43 54 65 76 87 98'))
                for slot in range(9):
                    expected = bytes((slot * 17 + i * 3) & 255 for i in range(268)).hex()
                    self.assertEqual(sample['actor' + str(slot)], expected)
                self.assertNotIn('actor9', sample)
                fixture.process.stdin.write('advance 3\n');fixture.process.stdin.flush()
                self.assertEqual(fixture.process.stdout.readline().strip(), 'OK')
                self.assertEqual(obs.sample(watches)['video_clock'], 1237)
                self.assertEqual(obs.stats['skipped_clock_values'], 2)
                with tempfile.TemporaryDirectory(dir=TEST_ROOT) as directory:
                    report = obs.dump_hwram(directory)
                    self.assertFalse(report['atomic'])
                    self.assertEqual(Path(report['saturn_path']).stat().st_size, 1048576)
                    self.assertEqual(Path(report['saturn_path']).read_bytes()[0x4000:0x6000], PROGRAM)
                self.assertTrue(obs.verify_signature())
        finally:
            fixture.close()

    def test_signature_mismatch_and_ambiguity_are_rejected(self):
        fixture = OwnedFixture()
        try:
            wrong = bytearray(PROGRAM);wrong[17] ^= 0x80
            with observer.ReadOnlyObserver(fixture.process.pid, wrong, proc_pid=fixture.proc_pid) as obs:
                with self.assertRaises(observer.NotReady):obs.locate()
        finally:
            fixture.close()
        fixture = OwnedFixture(two=True)
        try:
            with observer.ReadOnlyObserver(fixture.process.pid, PROGRAM, proc_pid=fixture.proc_pid) as obs:
                with self.assertRaisesRegex(observer.ObserverError, 'Ambiguous'):obs.locate()
        finally:
            fixture.close()

    def test_exact_elf_watch_resolution_and_bounds(self):
        sdk = '/workspace/scratch/1e822aeaeac7/wp1979/tooling/sh2eb-elf'
        elf = PROJECT / 'research/perf/sprite-prewarm-opening/runtime-warm/audit/wp1980-build/wrestlefest-wp1980.elf'
        symbols = observer.load_symbols(elf, sdk)
        watches = observer.build_watches(symbols, LAYOUT, ['pad_level:pad+8:2'])
        by_name = {watch['name']: watch for watch in watches}
        self.assertEqual(by_name['game_frame']['address'], symbols['rt'] + 24)
        self.assertEqual(by_name['actor8']['address'], symbols['rt'] + 384 + 8 * 268)
        self.assertEqual(by_name['actor8']['size'], 268)
        self.assertNotIn('actor9', by_name)
        self.assertEqual(by_name['pad_level']['address'], symbols['pad'] + 8)
        for invalid in ('bad:unknown', 'bad:0x00200000:4', 'bad:0x060fffff:2', 'bad:rt:0', 'bad name:rt'):
            with self.assertRaises(ValueError):observer.resolve_watch(invalid, symbols)

    def test_pre_exec_identity_keeps_the_child_pid(self):
        with tempfile.TemporaryDirectory(dir=TEST_ROOT) as directory:
            identity_path = Path(directory) / 'identity.json'
            command = [sys.executable, str(HERE / 'launch_identity.py'), str(identity_path),
                       sys.executable, '-u', '-c', FIXTURE]
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True)
            try:
                ready = json.loads(process.stdout.readline())
                record = json.loads(identity_path.read_text())
                self.assertEqual(record['namespace_pid'], process.pid)
                self.assertEqual(record['proc_pid'], ready['proc_pid'])
                with observer.ReadOnlyObserver(process.pid, PROGRAM, proc_pid=record['proc_pid']) as obs:
                    obs.locate()
                    self.assertEqual(obs.start_ticks, record['process_start_ticks'])
                    self.assertEqual(obs.sample(observer.build_watches(SYMBOLS))['video_clock'], 1234)
            finally:
                process.stdin.close();process.wait(timeout=5)
                process.stdout.close();process.stderr.close()


if __name__ == '__main__':
    unittest.main(verbosity=2)
