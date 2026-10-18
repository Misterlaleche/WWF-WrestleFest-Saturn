#!/usr/bin/env python3
"""Unmodified ROM $204B2/$204FA and $8B3A child-payment register regressions."""
import sys,hashlib,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN
from unicorn.m68k_const import *
rom=Path(sys.argv[1]).read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
for mode in range(4):
 u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,rom)
 def w(a,v,n=2):u.mem_write(a,v.to_bytes(n,'big'))
 for i in range(8):u.reg_write(UC_M68K_REG_D0+i,0xabc00000+i*0x1111)
 if mode<2:
  for i in range(2):
   a=0x1c05b0+i*0x10c;w(a,128,1);w(a+0x33,1|(64 if mode and i==0 else 0),1);w(a+6,0x200+0x40*i);w(a+10,0x180-0x20*i)
  u.reg_write(UC_M68K_REG_A0,0x1c11f4);pc=0x204b2
 else:
  w(0x1c007c,1);w(0x1c0161,1,1);w(0x1c0066,8);w(0x1c004f,1 if mode==2 else 0,1);w(0x140020,65535);w(0x140022,65535)
  a=0x1c05b0+3*0x10c;w(a+0xa2,1,1);w(a+0xa1,1,1);w(a+0x72,100);pc=0x8b3a
 u.reg_write(UC_M68K_REG_A7,0x1d0000);w(0x1d0000,0x70000,4);u.emu_start(pc,0x70000,count=100000);assert u.reg_read(UC_M68K_REG_PC)==0x70000
 out=subprocess.check_output([sys.argv[2],str(mode)],text=True).splitlines();actual=[int(v,16) for v in out[0].split()];expected=[u.reg_read(UC_M68K_REG_D0+i) for i in range(8)];assert actual==expected,(mode,[hex(x) for x in actual],[hex(x) for x in expected])
 if mode<2:assert bytes.fromhex(out[1])==bytes(u.mem_read(0x1c11f4,0x10c))
 else:assert [int(v) for v in out[1].split()]==[int.from_bytes(u.mem_read(0x1c004f,1),'big'),int.from_bytes(u.mem_read(a+0x66,2),'big')]
 print('PASS',('midpoint bit6clear','midpoint bit6set','refill paid success','refill last-slot failure')[mode],': all D0-D7 and affected state match direct ROM')
