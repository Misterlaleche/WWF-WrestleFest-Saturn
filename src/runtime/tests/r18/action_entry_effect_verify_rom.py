"""Compare linked TAG entrance and landing effects with the arcade ROM."""
import argparse
import ctypes
import hashlib
import itertools
import json
import random
import re
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--selector', type=lambda value: int(value, 0),
                    choices=(0x4d, 0x71, 0x76), default=0x4d)
args = parser.parse_args()
ROM = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(ROM).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
RAM = 0x1c0000
ACTOR = RAM + 0x5b0
rng = random.Random(0x4d1914)


def put(mem, address, value, width=2):
    mem[address:address+width] = (value & ((1 << (8*width))-1)).to_bytes(width, 'big')


core = re.search(r'^CORE=(.*)$', (ROOT / 'Makefile').read_text(), re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action4d-effect-') as directory:
    library = Path(directory) / 'action4d.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O0', '-UNDEBUG',
                    '-Wall', '-Wextra', '-Werror', '-Iinclude', *core,
                    'tests/r18/action39_oracle_bridge.c', '-o', str(library)],
                   cwd=ROOT, check=True)
    native = ctypes.CDLL(str(library))
    native.wf_action39_oracle.argtypes = [ctypes.c_void_p,
                                          ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint]
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, memory = machine.cpu, machine.mem
    for page in range(0, len(ROM), 0x10000):
        for width in range(3):
            memory.set_special_range_write_func(page, width, lambda address, value: None)
            memory.set_special_range_read_func(page, width,
                lambda address, n=1 << width: int.from_bytes(ROM[address:address+n], 'big'))
    coverage = set()
    cases = 0
    frames = (0,) if args.selector == 0x76 else (0, 1)
    for free, frame, facing, character in itertools.product(
            range(11), frames, (0, 0x8000), range(12)):
        mem = bytearray(0x200000)
        mem[:len(ROM)] = ROM
        mem[RAM:RAM+0x4000] = rng.randbytes(0x4000)
        mem[0xc0000:0xc4000] = rng.randbytes(0x4000)
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0 if i == free else 0x80, 1)
        for i in range(6):
            counter = int.from_bytes(ROM[0x280a+8*i:0x280e+8*i], 'big')
            put(mem, counter, 0)
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, 0, 1)
        put(mem, RAM+0x17e6, 0)
        put(mem, RAM+0x17ee, 0)
        put(mem, ACTOR, 0x8000)
        put(mem, ACTOR+2, character)
        put(mem, ACTOR+6, 0x260)
        put(mem, ACTOR+10, 0x160)
        put(mem, ACTOR+14, 0x140)
        put(mem, ACTOR+0x1c, 0x8005)
        put(mem, ACTOR+0x20, 5)
        put(mem, ACTOR+0x22, 0 if args.selector == 0x4d else 1)
        put(mem, ACTOR+0x24, frame)
        put(mem, ACTOR+0x2e, facing)
        put(mem, ACTOR+0x34, 0, 1)
        put(mem, ACTOR+0x60, args.selector)
        put(mem, ACTOR+0x19, (0x7f, 0x80)[free & 1], 1)
        regs = [rng.getrandbits(32) for _ in range(8)]
        memory.w_block(0, bytes(mem))
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        cpu.w_reg(8, ACTOR)
        for i, value in enumerate(regs):
            cpu.w_reg(i, value)
        for _ in range(20000):
            pc = cpu.r_pc()
            coverage.add(pc)
            if pc == 0x70000:
                break
            cpu.execute(1)
        assert cpu.r_pc() == 0x70000, (free, frame, facing, character, 'ROM runaway')
        buffer = ctypes.create_string_buffer(bytes(mem))
        native_regs = (ctypes.c_uint32*8)(*regs)
        status = native.wf_action39_oracle(buffer, native_regs, 0)
        tag = (free, frame, facing, character)
        assert status == 0, (tag, 'native blocked', hex(-status))
        for address, length in ((RAM, 0x4000), (0xc0000, 0x4000)):
            got = buffer.raw[address:address+length]
            want = bytes(memory.r_block(address, length))
            assert got == want, (tag, hex(address), [(hex(address+i), x, y)
                for i, (x, y) in enumerate(zip(got, want)) if x != y][:10])
        assert list(native_regs) == [cpu.r_reg(i) for i in range(8)], (tag, 'D0-D7')
        cases += 1
    callback = {0x4d: 0x18948, 0x71: 0x19f1e, 0x76: 0x1a10e}[args.selector]
    assert 0x10d3a in coverage and callback in coverage, ('effect path', coverage)
    assert bytes(memory.r_block(0, len(ROM))) == ROM
    machine.cleanup()
    print(json.dumps({'status': 'PASS', 'cases': cases,
                      'effects': {0x4d: [19, 20], 0x71: [13, 14],
                                  0x76: [8]}[args.selector],
                      'free_slots': 11,
                      'characters': 12}))
