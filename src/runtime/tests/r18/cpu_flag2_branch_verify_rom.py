#!/usr/bin/env python3
"""Compare the formerly blocked $1CCF0 CPU branch against the intact ROM."""
import ctypes as C
import json
import re
import subprocess
from pathlib import Path
import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT.parents[1]
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
report = json.loads((PROJECT / 'validation/local-20261025/diag2-snake-tiles-1200/report.json').read_text())
handoff = (PROJECT / 'validation/local-20261025/diag2-snake-tiles-1200/handoff-after-f8c-ram.bin').read_bytes()
actor0 = bytes.fromhex(report['actors_at_block'][0])
target = bytes.fromhex(report['actors_at_block'][5])
assert len(actor0) == len(target) == 0x10c and len(handoff) == 0x4000
core = re.search(r'^CORE=(.*)$', (ROOT / 'Makefile').read_text(), re.M)[1].split()
core.remove('src/wf_cpu_runtime.c')
library = ROOT / 'build/cpu_flag2_branch_oracle.so'
library.parent.mkdir(exist_ok=True)
subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-UNDEBUG',
                '-Wall', '-Wextra', '-Werror', '-Iinclude', *core,
                'tests/r18/cpu_flag2_branch_oracle_bridge.c', '-o', str(library)],
               cwd=ROOT, check=True)
native = C.CDLL(str(library)).cpu_flag2_branch_native
native.argtypes = [C.c_void_p, C.c_void_p, C.POINTER(C.c_uint32)]
native.restype = C.c_int
machine = mus.Machine(mus.CPUType.M68000, 2048)
cpu, mem = machine.cpu, machine.mem
cases = (0, 1, 2, 3, 34, 39, 127, 255)
for index in cases:
    m = bytearray(0x200000)
    m[:len(rom)] = rom
    m[0x1c0000:0x1c4000] = handoff
    a = bytearray(actor0)
    t = bytearray(target)
    t[0x44:0x46] = index.to_bytes(2, 'big')
    m[0x1c05b0:0x1c06bc] = a
    m[0x1c0aec:0x1c0bf8] = t
    mem.w_block(0, bytes(m))
    cpu.w_sp(0x1d0000)
    mem.w32(0x1d0000, 0x70000)
    cpu.w_pc(0x1ccf0)
    cpu.w_reg(8, 0x1c05b0)
    cpu.w_reg(9, 0x1c0aec)
    original_d = report['shadow_d_at_block']
    for i, value in enumerate(original_d):
        cpu.w_reg(i, value)
    for _ in range(40):
        if cpu.r_pc() == 0x70000:
            break
        cpu.execute(1)
    assert cpu.r_pc() == 0x70000, (index, hex(cpu.r_pc()))
    want_actor = bytes(mem.r_block(0x1c05b0, 0x10c))
    want_d = [cpu.r_reg(i) for i in range(8)]
    got_actor = C.create_string_buffer(bytes(a))
    got_target = C.create_string_buffer(bytes(t))
    got_d = (C.c_uint32 * 8)(*original_d)
    assert native(got_actor, got_target, got_d) == 0, index
    assert got_actor.raw[:0x10c] == want_actor, (index, 'actor')
    assert list(got_d) == want_d, (index, 'registers')
machine.cleanup()
print(f'PASS $1CCF0 original ROM differential: {len(cases)} indexes including frame327 value34')
