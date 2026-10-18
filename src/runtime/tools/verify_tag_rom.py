#!/usr/bin/env python3
"""Execute unmodified canonical $6E34 including every child; compare native state/video/D0-D7."""
import sys,hashlib,subprocess,tempfile,random
from pathlib import Path
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_MEM_WRITE
from unicorn.m68k_const import *
rom=Path(sys.argv[1]).read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
def run(name,patch,seed=None):
 mem=bytearray(0x200000);mem[:len(rom)]=rom
 def w(a,v,n=2):mem[a:a+n]=v.to_bytes(n,'big')
 w(0x1c007c,1);w(0x1c0066,4);w(0x1c004f,1,1)
 for i in range(4):w(0x140020+2*i,0xffff)
 w(0x140020,0xff7f)
 for i in range(8):w(0x1f0000+4*i,0x12340000+i*0x1111,4)
 if seed is not None:
  rng=random.Random(seed)
  for i in range(8):w(0x1f0000+4*i,rng.getrandbits(32),4)
  for actor in range(9):
   for offset in list(range(4,0x1c))+list(range(0x38,0x56))+list(range(0x58,0x66))+list(range(0xb0,0xfe)):
    w(0x1c05b0+actor*0x10c+offset,rng.getrandbits(8),1)
 for a,v,n in patch:w(a,v,n)
 u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,bytes(mem));u.reg_write(UC_M68K_REG_A7,0x1d0000);u.mem_write(0x1d0000,(0x70000).to_bytes(4,'big'))
 for i in range(8):u.reg_write(UC_M68K_REG_D0+i,int.from_bytes(mem[0x1f0000+4*i:0x1f0004+4*i],'big'))
 writes=[]
 def trace(cpu,access,address,size,value,user):
  if 0xc0000<=address<0xc4000 or 0x180000<=address<0x181000:writes.append((address,value&((1<<(8*size))-1),size))
 u.hook_add(UC_HOOK_MEM_WRITE,trace)
 u.emu_start(0x6e34,0x70000,count=200000);assert u.reg_read(UC_M68K_REG_PC)==0x70000,name
 with tempfile.TemporaryDirectory() as t:
  p=Path(t);(p/'in').write_bytes(mem);subprocess.run([sys.argv[2],str(p/'in'),str(p/'out'),str(p/'trace')],check=True);actual=(p/'out').read_bytes();native_writes=[(int(a,16),int(v,16),int(n)) for a,v,n in (x.split() for x in (p/'trace').read_text().splitlines())]
  for low,high in [(0xc0000,0xc4000),(0x180000,0x181000)]:
   expected_trace=[x for x in writes if low<=x[0]<high];actual_trace=[x for x in native_writes if low<=x[0]<high];assert actual_trace==expected_trace,(name,'write trace',next(((i,x,y) for i,(x,y) in enumerate(zip(actual_trace,expected_trace)) if x!=y),('length',len(actual_trace),len(expected_trace))))
 for a,n in [(0x1c0000,0x2000),(0xc0000,0x2000),(0x180000,0x1000)]:
  expected=bytes(u.mem_read(a,n));diff=[(hex(a+i),x,y) for i,(x,y) in enumerate(zip(actual[a:a+n],expected)) if x!=y];assert not diff,(name,diff[:30])
 regs=[u.reg_read(UC_M68K_REG_D0+i) for i in range(8)];got=[int.from_bytes(actual[0x1f0000+4*i:0x1f0004+4*i],'big') for i in range(8)];assert regs==got,(name,[hex(v) for v in got],[hex(v) for v in regs]);print('PASS',name)
a=0x1c05b0;b=a+0x10c;c=a+4*0x10c;d=c+0x10c
run('immediate refill',[(a,128,1)])
run('no credits',[(a,128,1),(0x1c004f,0,1)])
run('partner refill',[(a,128,1),(a+0x33,1,1),(a+0x86,b,4),(b+0x56,64,1)])
run('relocation with reciprocal repair',[(c,128,1),(d,128,1),(c+0x33,1,1),(c+0x57,2,1),(d+0x57,3,1),(c+0x26,a+6*0x10c,4),(a+6*0x10c+0x9c,c,4),(0x1c1684,d,4)])
run('delayed paid choice start',[(0x1c0066,0x804,2),(a+2*0x10c,128,1)])
run('delayed choice input',[(0x1c0066,0x804,2),(0x1c1690,0x2040,2),(0x1c0156,0x140020,4),(0x140020,0xfffe,2)])
run('delayed confirmation migration',[(0x1c0066,0x804,2),(0x1c1690,0x2040,2),(0x1c0156,0x140020,4),(0x140020,0xffef,2),(c,128,1),(d,128,1),(c+0x33,1,1)])
run('delayed confirmation refill',[(0x1c0066,0x804,2),(0x1c1690,0x2080,2),(0x1c0156,0x140020,4),(0x140020,0xffef,2),(a,128,1)])
run('immediate lockout rejection',[(a,128,1),(a+0xfe,1,2)])
run('reserve exclusion character4',[(c+0x57,4,1)])
run('delayed timeout busy retry',[(0x1c0066,0x804,2),(0x1c1690,0x2040,2),(0x1c1698,511,2),(0x1c0156,0x140020,4),(c+0xfe,1,2),(0x1c15e3,8,1)])
run('progression4 immediate paid partner',[(0x1c0066,0x804,2),(0x1c0162,4,2),(a,128,1),(b,128,1),(0x140022,0xff7f,2)])
for slot in range(4):
 run('migration input slot%d'%slot,[(0x140020,0xffff,2),(0x140020+2*slot,0xff7f,2),(c,128,1),(d,128,1),(d+0x33,1,1),(c+0x57,7,1),(d+0x57,8,1)])

for seed in [0,1,0x12345678,0xffffffff]:
 run("seed%d refill"%seed,[(a,128,1)],seed)
 run("seed%d raw migration"%seed,[(c,128,1),(d,128,1),(c+0x33,1,1),(c+0x57,9,1),(d+0x57,11,1)],seed)
 run("seed%d delayed direction"%seed,[(0x1c0066,0x804,2),(0x1c1690,0x2080,2),(0x1c0156,0x140020,4),(0x140020,0xfffb,2)],seed)
