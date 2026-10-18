#!/usr/bin/env python3
"""Execute unchanged ROM $F9A->$10E8, compare all 29 native owner tuples.
Test oracle ONLY: pip install unicorn==2.1.4. Native engine contains no emulator.
Usage: python3 verify_runtime17_frame_rom.py MAIN68K.BIN NATIVE_FRAME_DRIVER
The native test writes its initial RAM to a temporary file. Hardware input is
released, graphics hardware is mapped RAM, and no gameplay call is intercepted.
Only instruction hooks at real owner return addresses observe register values.
Coverage: one active TAG frame with visible raw7C movement, actual referee and
fixed-object sprites, native participant owner, then VBlank ACK. This is not
proof of every action, branch, rendering record or subsequent frame.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile
from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE
from unicorn.m68k_const import UC_CPU_M68K_M68000, UC_M68K_REG_A7, UC_M68K_REG_D0, UC_M68K_REG_PC
rom_path=pathlib.Path(sys.argv[1]).resolve()
rom=rom_path.read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
with tempfile.TemporaryDirectory(prefix='r17-rom-') as tmp:
    ram_path=pathlib.Path(tmp)/'initial.bin'
    output=subprocess.check_output([str(pathlib.Path(sys.argv[2]).resolve()),str(ram_path),str(rom_path)],text=True)
    ram=ram_path.read_bytes()
lines=[line.split() for line in output.splitlines() if len(line.split())==11]
assert len(lines)==29, output
ack=[line.split()[1:] for line in output.splitlines() if line.startswith('ACK ')][0]
returns=[0xfb4,0xfb8,0xfbe,0xfc4,0xfca,0xfd0,0xfd6,0xfdc,0xfe2,0xfe8,0xfee,0xff4,0xffa,0x1000,0x1006,0x100c,0x1010,0x1014,0x1018,0x101e,0x1024,0x1028,0x102c,0x1032,0x1036,0x103a,0x103e,0x1042,0x1048]
u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN)
u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
u.mem_map(0,0x400000)
u.mem_write(0,rom)
u.mem_write(0x1c0000,ram)
u.mem_write(0x140020,b'\xff'*16)
u.reg_write(UC_M68K_REG_A7,0x1d0000)
seen=set();mismatches=[];active_path=set();ack_running=False
def values():
    return [u.reg_read(UC_M68K_REG_D0+j) for j in range(8)]+[int.from_bytes(u.mem_read(0x1c005c,4),'big')]
def observe(cpu,pc,size,user):
    if ack_running and pc==0xf9a:
        cpu.emu_stop();return
    active_path.add(pc)
    if pc in returns:
        i=returns.index(pc);seen.add(i);p=lines[i]
        expected=[int(v,16) for v in p[2:]]
        actual=values()
        if actual!=expected:
            mismatches.append((p[0],['%08X'%v for v in expected],['%08X'%v for v in actual]))
u.hook_add(UC_HOOK_CODE,observe)
u.emu_start(0xf9a,0x10e8,count=1000000)
assert u.reg_read(UC_M68K_REG_PC)==0x10e8, 'ROM instruction budget exhausted'
assert len(seen)==29
assert {0x1a5e8,0x10bd0,0xd1fc,0x6e34}<=active_path
assert not mismatches, mismatches
assert int.from_bytes(u.mem_read(0x1c05b6,2),'big')==0x1e0
u.mem_write(0x1c006f,bytes([u.mem_read(0x1c006f,1)[0]|2]))
ack_running=True
u.emu_start(0x10e8,0x70000,count=1000000)
assert u.reg_read(UC_M68K_REG_PC)==0xf9a,hex(u.reg_read(UC_M68K_REG_PC))
assert values()==[int(v,16) for v in ack]
assert int.from_bytes(u.mem_read(0x1c0080,4),'big')==1
assert u.mem_read(0x1c0083,1)[0]==1
print('PASS actual ROM F9A->10E8: all29 D0-D7/RNG tuples, visible raw7C movement, native render-record shadow, native TAG, ACK->F9A and next branch-B selector')
