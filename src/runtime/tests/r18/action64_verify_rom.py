"""Compare Royal linked movement $12614[$64] with the intact 68000 ROM."""
import ctypes
import hashlib
import json
import os
import random
import re
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

root = Path(__file__).resolve().parents[2]
workspace = root.parents[1]
rom = (root / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x127a4:0x127a8] == bytes.fromhex('00019608')
assert rom[0x19608:0x19618] == bytes.fromhex('00019618000100020008ff0000160013')
report = json.loads((workspace / 'validation/local-20261025/royal-long-stage9z-7000/report.json').read_text())
actor = [0x1c05b0 + i * 0x10c for i in range(9)]


def put(data, address, value, width=2):
    data[address:address + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'big')


def fixtures():
    base = bytearray(0x200000)
    base[:len(rom)] = rom
    for index, raw in enumerate(report['actors_at_block']):
        base[actor[index]:actor[index] + 0x10c] = bytes.fromhex(raw)
    put(base, 0x1c007c, 1)
    put(base, 0x1c0161, 1, 1)
    a = actor[5]
    for velocity, facing, contact in ((0, 0x80, 0), (1, 0, 0), (42, 0x80, 0),
                                      (43, 0, 0), (0, 0x80, 0x10)):
        data = bytearray(base)
        put(data, a + 0x66, velocity)
        put(data, a + 0x2e, facing, 1)
        put(data, a + 0x37, contact, 1)
        yield f'first-{velocity}-{facing}-{contact}', data, 5
    for selector, contact, score, mash, aa in ((0x4064, 0, 0, 0, 0),
                                              (0x8064, 0, 1, 1, 3),
                                              (0x8064, 0, 1, 0, 3),
                                              (0xc064, 0x10, 3, 1, 1)):
        data = bytearray(base)
        put(data, a + 0x1c, 0x8005)
        put(data, a + 0x60, selector)
        put(data, a + 0x37, contact, 1)
        put(data, a + 0x66, score)
        put(data, a + 0xa3, mash, 1)
        put(data, a + 0xaa, aa)
        yield f'latched-{selector}-{contact}-{score}-{mash}-{aa}', data, 5


core = re.search(r'^CORE=(.*)$', (root / 'Makefile').read_text(), re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action64-', ignore_cleanup_errors=os.name == 'nt') as folder:
    library = Path(folder) / 'action64.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Iinclude', *core, 'src/wf_r18_runtime_handoff.c',
                    'tests/r18/action0b_oracle_bridge.c', '-o', str(library)],
                   cwd=root, check=True)
    native = ctypes.CDLL(str(library))
    native.action0b_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32),
                                      ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32)]
    rng = random.Random(0x19618)
    coverage = set()
    count = 0
    for name, data, slot in fixtures():
        regs = [rng.getrandbits(32) for _ in range(8)]
        data[0xc0000:0xc4000] = rng.randbytes(0x4000)
        machine = mus.Machine(mus.CPUType.M68000, 2048)
        cpu, memory = machine.cpu, machine.mem
        memory.w_block(0, bytes(data))
        for index, value in enumerate(regs):
            cpu.w_reg(index, value)
        cpu.w_reg(8, actor[slot])
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        descriptor = 0
        for _ in range(30000):
            pc = cpu.r_pc()
            coverage.add(pc)
            if pc == 0x70000:
                break
            if pc == 0x1c0c8:
                descriptor = cpu.r_reg(9)
            cpu.execute(1)
        assert cpu.r_pc() == 0x70000, (name, 'ROM runaway', hex(cpu.r_pc()))
        ram = bytes(memory.r_block(0x1c0000, 0x4000))
        fg = bytes(memory.r_block(0xc0000, 0x4000))
        want_regs = [cpu.r_reg(index) for index in range(8)]
        machine.cleanup()
        buffer = ctypes.create_string_buffer(bytes(data))
        native_regs = (ctypes.c_uint32 * 8)(*regs)
        native_descriptor = ctypes.c_uint32()
        status = native.action0b_oracle(buffer, native_regs, slot,
                                        ctypes.byref(native_descriptor))
        assert status == 0, (name, 'native blocked', hex(-status))
        assert native_descriptor.value == descriptor, (name, 'descriptor',
                                                       hex(native_descriptor.value), hex(descriptor))
        for address, expected in ((0x1c0000, ram), (0xc0000, fg)):
            got = buffer.raw[address:address + len(expected)]
            assert got == expected, (name, 'memory', [(hex(address + index), x, y)
                                                      for index, (x, y) in enumerate(zip(got, expected))
                                                      if x != y][:12])
        assert list(native_regs) == want_regs, (name, 'D0-D7', list(native_regs), want_regs)
        count += 1
    required = {0x19618, 0x19624, 0x19630, 0x19636, 0x1964e, 0x19662}
    assert required.issubset(coverage), [hex(pc) for pc in sorted(required - coverage)]
    output = workspace / 'validation/local-20261025/action64-rom-report.json'
    output.write_text(json.dumps({'status': 'PASS', 'cases': count,
                                  'compared': ['RAM16KiB', 'FG16KiB', 'D0-D7', 'descriptor']}, indent=2) + '\n')
    print(f'PASS {count} action64 cases; raw RAM, FG, D0-D7, descriptor')
