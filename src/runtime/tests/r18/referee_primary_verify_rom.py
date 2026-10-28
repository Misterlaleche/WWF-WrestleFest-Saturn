"""Compare idle/return referee acquisition with the original 68000 owners.

The ROM runs $1F97E/$1F99E through RTS without patched code or intercepts.
The bridge selects the same primary stage before the independent action stage.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess
import tempfile

import machine68k as mus

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RAM = 0x1c0000
REF = RAM + 0x11f4
ACTORS = [RAM + 0x5b0 + 0x10c*i for i in range(9)]
parser = argparse.ArgumentParser()
parser.add_argument('--quick', action='store_true')
parser.add_argument('--report', type=Path)
args = parser.parse_args()
rom = (ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'

def put(mem, at, value, size=2):
    mem[at:at+size] = value.to_bytes(size, 'big')

def fixture(state, mode, credit, old, trigger, slot, timer=0):
    mem = bytearray(0x200000)
    mem[:len(rom)] = rom
    put(mem, RAM+0x7c, credit)
    put(mem, RAM+0x161, mode, 1)
    put(mem, RAM+0x1682, timer)
    put(mem, RAM+0x1684, ACTORS[slot], 4)
    put(mem, REF+0x20, state)
    put(mem, REF+0x35, 0xff, 1)
    # Two in-ring participants for the original TAG midpoint helper.
    other = 4 if slot == 0 else 0
    for i in (slot, other, 5):
        put(mem, ACTORS[i], 0x8000)
        put(mem, ACTORS[i]+6, 480 + i*24)
        put(mem, ACTORS[i]+10, 320 + i*4)
    for i in (slot, other):
        put(mem, ACTORS[i]+0x33, 1, 1)
    if old != 'none':
        put(mem, REF+0x56, ACTORS[5], 4)
        put(mem, ACTORS[5]+0xfe, {'free':0, 'locked':1, 'result':0x8000}[old])
    if trigger in ('pin', 'inactive'):
        put(mem, ACTORS[slot]+0x35, 1, 1)
    elif trigger in ('submission', 'submission-outside'):
        put(mem, ACTORS[slot]+0x35, 2, 1)
    if trigger == 'inactive':
        put(mem, ACTORS[slot], 0)
    if trigger == 'submission-outside':
        put(mem, ACTORS[slot]+0x33, 0, 1)
    # Invalid TAG midpoint fixtures are omitted by the matrix generator.
    return mem

configs = [(5, 1, 1, 'free', 'pin', 4, 0)]
if not args.quick:
    configs = []
    for state, mode, credit, old, trigger, slot in itertools.product(
            (0, 5), (0, 1), (0, 1), ('none', 'free', 'locked', 'result'),
            ('none', 'pin', 'submission', 'inactive', 'submission-outside'), (0, 4, 8)):
        if mode == 0 and trigger in ('inactive', 'submission-outside'):
            continue
        configs.append((state, mode, credit, old, trigger, slot, 0))
    for mode, timer in itertools.product((0, 1), (1, 2)):
        configs.append((0, mode, 1, 'free', 'pin', 4, timer))

sources = ['src/wf_actor.c', 'src/wf_physics.c', 'generated/wf_physics_tables.c',
           'src/wf_r17_m68k_shadow.c', 'src/wf_r17_render_contract.c',
           'tests/r18/referee_primary_oracle_bridge.c']
coverage = set()
failures = []
randomizer = random.Random(0x1f9d0)
with tempfile.TemporaryDirectory() as tmp:
    library = Path(tmp)/'ref-primary.so'
    # Keep only the primary stage: unrelated action owners need not be linked.
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O1', '-Wall',
                    '-ffunction-sections', '-fdata-sections', '-fvisibility=hidden',
                    '-Wl,--gc-sections', '-Wl,-z,defs',
                    '-Wextra', '-Werror', '-Iinclude', *sources, '-o', str(library)],
                   cwd=ROOT, check=True)
    native = ctypes.CDLL(str(library))
    native.referee_primary_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    for config in configs:
        state, mode, credit, old, trigger, slot, timer = config
        mem = fixture(*config)
        regs = tuple(randomizer.getrandbits(32) for _ in range(8))
        buf = ctypes.create_string_buffer(bytes(mem))
        nregs = (ctypes.c_uint32*8)(*regs)
        status = native.referee_primary_oracle(buf, nregs)
        machine = mus.Machine(mus.CPUType.M68000, 2048)
        cpu, memory = machine.cpu, machine.mem
        memory.w_block(0, bytes(mem))
        cpu.w_sr(0x2700)
        for i, value in enumerate(regs): cpu.w_reg(i, value)
        cpu.w_reg(8, REF)
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1f97e if state == 0 else 0x1f99e)
        for step in range(10000):
            pc = cpu.r_pc()
            if pc == 0x70000: break
            coverage.add(pc)
            cpu.execute(1)
        assert cpu.r_pc() == 0x70000, (config, 'ROM runaway', hex(cpu.r_pc()))
        expected = bytes(memory.r_block(RAM, 0x2000))
        differences = [(hex(RAM+i), actual, wanted)
                       for i, (actual, wanted) in enumerate(zip(buf.raw[RAM:RAM+0x2000], expected))
                       if actual != wanted]
        register_differences = [(i, hex(nregs[i]), hex(cpu.r_reg(i)))
                                for i in range(8) if nregs[i] != cpu.r_reg(i)]
        if status or differences or register_differences:
            failures.append({'config':config, 'status':status,
                             'ram':differences, 'registers':register_differences})
        machine.cleanup()
required = {0x1f9cc, 0x1f9d0, 0x1f9d2, 0x1f9d6, 0x20576, 0x2058e, 0x205a0}
if not args.quick:
    assert required <= coverage, ('missing branches', sorted(required-coverage))
report = {'status':'FAIL' if failures else 'PASS', 'cases':len(configs),
          'rom_sha1':hashlib.sha1(rom).hexdigest(),
          'native_sha256':hashlib.sha256((ROOT/'src/wf_r17_referee.c').read_bytes()).hexdigest(),
          'rom_pcs':[hex(pc) for pc in sorted(coverage)], 'failures':failures}
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({'status':report['status'], 'cases':len(configs),
                  'failed':len(failures), 'first':failures[:1]}))
raise SystemExit(bool(failures))
