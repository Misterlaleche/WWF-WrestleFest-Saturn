#!/usr/bin/env python3
"""Independent direct-ROM D1FC/DCA0 versus admitted native record D6/D7 contract.
Usage: PYTHONPATH=<unicorn> python3 verify_render_shadow_rom.py ROM DRIVER
No instructions or game child calls are intercepted. Only a code observer asserts DCA0 entry.
"""
import sys,hashlib,subprocess,random
from pathlib import Path
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_CODE
from unicorn.m68k_const import *
rom=Path(sys.argv[1]).read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
def be(a,n):return int.from_bytes(rom[a:a+n],'big')
rng=random.Random(0xD1FC);passed=blocked=0;outputs=set();kinds=set()
for character in [0,12,14]:
 for pose in [0,2,10]:
  table=0x38f14+int.from_bytes(rom[0x38f14+2*character:0x38f16+2*character],'big',signed=True)
  offset=be(table+pose*2,2);assert offset!=0xfffe and table!=0x38f14,'DCA0 must accept actual record'
  record=be(0x38fb8+4*character,4)+offset;header=be(record,2)
  for mirror in [0,1]:
   positions=[(0,0),(150,100),(1000,100),(100,1000),(0xffff,0xffff)]+[(rng.randrange(65536),rng.randrange(65536)) for _ in range(4)]
   for x,y in positions:
    seed=rng.getrandbits(32)
    native=subprocess.check_output([sys.argv[2],sys.argv[1],str(character),str(pose),str(x),str(y),str(mirror),str(seed)],text=True).split()
    status=int(native[0]);got=[int(v,16) for v in native[1:3]];assert int(native[3],16)==record
    if header&0x8300 or not header&255:
     assert status==1 and got==[(seed+6*0x11111111)&0xffffffff,(seed+7*0x11111111)&0xffffffff];blocked+=1;continue
    assert status==0
    u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,rom)
    def w(a,v,n=2):u.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'big'))
    a=0x1c05b0;w(a+2,character);w(a+4,pose|(0x8000 if mirror else 0));w(a+0x14,x);w(a+0x16,y)
    for i in range(8):u.reg_write(UC_M68K_REG_D0+i,(seed+i*0x11111111)&0xffffffff)
    u.reg_write(UC_M68K_REG_A0,a);u.reg_write(UC_M68K_REG_A2,0xc2000);u.reg_write(UC_M68K_REG_A7,0x1d0000);w(0x1d0000,0x70000,4)
    observed=set();u.hook_add(UC_HOOK_CODE,lambda cpu,pc,size,user:observed.add(pc))
    u.emu_start(0xd1fc,0x70000,count=500000);assert u.reg_read(UC_M68K_REG_PC)==0x70000,'ROM instruction limit';assert 0xdca0 in observed and 0xd212 in observed
    want=[u.reg_read(UC_M68K_REG_D6),u.reg_read(UC_M68K_REG_D7)]
    assert got==want,(character,pose,mirror,x,y,hex(header),[hex(v) for v in got],[hex(v) for v in want])
    outputs.add(want[0]);kinds.add(bool(header&0x4000));passed+=1
assert outputs=={0,4,5,0x1c1610} and kinds=={False,True},outputs
print('PASS direct-ROM D1FC/DCA0:',passed,'admitted records/positions; D6 branches',','.join(hex(x) for x in sorted(outputs)),';',blocked,'unsupported-kind rejections; both mirrors, randomized screen positions and incoming registers; A2=$C2000')
