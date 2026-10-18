#!/usr/bin/env python3
"""Test-only unmodified MC68000 ROM oracle. pip install unicorn==2.1.4.
Usage: PYTHONPATH=<unicorn location> python3 tools/verify_active_rom.py ROM NATIVE_TEST
Native production neither links nor uses Unicorn. This verifies one active owner,
not a complete frame; ROM calls and callbacks are never intercepted or replaced.
"""
import hashlib
import subprocess
import sys
from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE
from unicorn.m68k_const import UC_CPU_M68K_M68000, UC_M68K_REG_A7, UC_M68K_REG_D0, UC_M68K_REG_PC
rom = open(sys.argv[1], 'rb').read()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
u = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
u.mem_map(0, 0x200000)
u.mem_write(0, rom)
def write(address, value, size=2):
    u.mem_write(address, value.to_bytes(size, 'big'))
for j in range(8):
    u.reg_write(UC_M68K_REG_D0+j, 0x11110000+j*0x1111)
for address, value, size in [(0x1c05b0,0x80,1),(0x1c05d0,0x8005,2),
        (0x1c05cc,0x8005,2),(0x1c0610,0x7c,2),(0x1c05d4,0xfe,2),
        (0x1c05d6,0x1c06bc,4),(0x1c05b6,0x280,2),(0x1c05ba,0x150,2),
        (0x1c05be,0x100,2),(0x1c17e6,0x140,2),(0x1c17ee,0x200,2),
        (0x1c005c,0x13579bdf,4)]:
    write(address,value,size)
seen = set()
u.hook_add(UC_HOOK_CODE, lambda cpu, pc, size, data: seen.add(pc))
def execute(pc):
    u.reg_write(UC_M68K_REG_A7,0x1d0000)
    write(0x1d0000,0x70000,4)
    u.emu_start(pc,0x70000,count=100000)
    assert u.reg_read(UC_M68K_REG_PC)==0x70000, 'instruction budget exhausted'
execute(0xf4c2)
assert {0x1a5e8,0x1a5ea,0x10bd0,0x1c0c8,0x247c,0x27b8,0x2208,0x24090} <= seen
native = subprocess.check_output([sys.argv[2],'--dump'],text=True).splitlines()
regs = [u.reg_read(UC_M68K_REG_D0+j) for j in range(8)]
assert [int(x,16) for x in native[0].split()] == regs + [0x13579bdf]
for k in range(2):
    expected=bytes(u.mem_read(0x1c05b0+k*0x10c,0x10c))
    actual=bytes.fromhex(native[k+1])
    assert actual==expected, [(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
execute(0x21b4)
assert u.reg_read(UC_M68K_REG_D0)==0x8ad04688
assert bytes(u.mem_read(0x1c005c,4)).hex()=='8ad04688'
print('PASS direct-ROM active F4C2: D0-D7, both complete actor records, downstream RNG 8AD04688; no game hooks')
# Independent $F560 fixture from test_runtime17.c: RNG inside $F5E4 must not
# escape its D0-D4 MOVEM frame, while the RNG state in RAM does change.
u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
u.mem_map(0,0x200000);u.mem_write(0,rom)
write(0x1c005c,0x10203040,4);write(0x1c007c,1)
for i in range(2):
    a=0x1c05b0+i*0x10c
    for offset,value,size in [(0,0x80,1),(0x20,0x8000,2),(0x56,i*0x80,1),
            (0x2e,i*0x80,1),(0x7a,0x1c05b0+(1-i)*0x10c,4),
            (6,0x200+i*0x20,2),(10,0x100+i*8,2)]:
        write(a+offset,value,size)
execute(0xf560)
assert [u.reg_read(UC_M68K_REG_D0+j) for j in range(8)]==[9,0,0x113,8,0xffff,0,0,0]
assert bytes(u.mem_read(0x1c005c,4)).hex()=='c1020305'
print('PASS direct-ROM F560 reference for test_runtime17: MOVEM boundary tuple and RNG C1020305')
