#!/usr/bin/env python3
"""Differential-check state-10 callback $122E6 against the intact 68000 ROM."""
import ctypes as C
import re
import subprocess
from pathlib import Path
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
core.remove('src/wf_r17_dispatch_1c03e.c')
lib=ROOT/'build/main_ten_oracle.so'
lib.parent.mkdir(exist_ok=True)
subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                '-Iinclude',*core,'tests/r18/main_ten_oracle_bridge.c','-o',str(lib)],
               cwd=ROOT,check=True)
native=C.CDLL(str(lib)).main_ten_native
native.argtypes=[C.c_void_p,C.c_uint8,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32),C.POINTER(C.c_uint32)]
native.restype=C.c_int
machine=mus.Machine(mus.CPUType.M68000,2048)
cpu,mem=machine.cpu,machine.mem
cases=[('tag-init',0,route,0,0,0) for route in range(8)]
cases += [('tag-drop',0,0,0x80,2,0),('tag-still',0,2,0x80,4,4),
          ('tag-finish',0,1,0x80,0xfe,0),('tag-idle',0,3,0x80,1,1)]
cases += [('royal-init',2,route,0,0,0) for route in (8,9)]
cases += [('royal-drop',2,8,0x80,4,0),('royal-still',2,9,0x80,4,1),
          ('royal-finish',2,9,0x80,0xfe,0),('royal-idle',2,8,0x80,1,0)]
for name,mode,route,latch,phase,timer in cases:
    actor=bytearray(0x10c)
    actor[0x1c]=latch
    actor[0x20:0x22]=(10).to_bytes(2,'big')
    actor[0x22:0x24]=timer.to_bytes(2,'big')
    actor[0x25]=phase
    actor[0x2e:0x30]=(0x8000).to_bytes(2,'big')
    actor[0x33]=0x7f
    actor[0x32]=0x7f
    actor[0x44:0x46]=route.to_bytes(2,'big')
    actor[0x12:0x14]=(4).to_bytes(2,'big')
    actor[0x4a:0x4c]=(0x1234).to_bytes(2,'big')
    dregs=[0xaced0000+i for i in range(8)]
    m=bytearray(0x200000)
    m[:len(rom)]=rom
    m[0x1c0161]=mode
    m[0x1c1670:0x1c1674]=(0x12345678).to_bytes(4,'big')
    m[0x1c05b0:0x1c06bc]=actor
    mem.w_block(0,bytes(m))
    cpu.w_sp(0x1d0000)
    mem.w32(0x1d0000,0x70000)
    cpu.w_pc(0x122e6)
    cpu.w_reg(8,0x1c05b0)
    cpu.w_reg(9,0x12282)
    for i,v in enumerate(dregs):cpu.w_reg(i,v)
    for _ in range(100):
        if cpu.r_pc()==0x70000:break
        cpu.execute(1)
    assert cpu.r_pc()==0x70000,(name,route,hex(cpu.r_pc()))
    want_actor=bytes(mem.r_block(0x1c05b0,0x10c))
    want_global=mem.r32(0x1c1670)
    want_d=[cpu.r_reg(i) for i in range(8)]
    want_desc=cpu.r_reg(9)
    got_actor=C.create_string_buffer(bytes(actor))
    got_global=C.c_uint32(0x12345678)
    got_d=(C.c_uint32*8)(*dregs)
    got_desc=C.c_uint32()
    assert native(got_actor,mode,C.byref(got_global),got_d,C.byref(got_desc))==0,(name,route)
    assert got_actor.raw[:0x10c]==want_actor,(name,route,'actor')
    assert got_global.value==want_global,(name,route,'global')
    assert list(got_d)==want_d,(name,route,'dregs')
    assert got_desc.value==want_desc,(name,route,'descriptor',hex(got_desc.value),hex(want_desc))
machine.cleanup()
print(f'PASS state10 $122E6 ROM differential: {len(cases)} routes and phases')
