#!/usr/bin/env python3
"""Focused original-ROM blocks; explicit fixtures are not a power-on bootstrap."""
from pathlib import Path
import argparse,ctypes,hashlib,json,subprocess,zipfile
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.m68k_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args();ROOT=args.out.resolve();ROOT.mkdir(parents=True,exist_ok=False)
ROMZIP=args.rom
production=Path(__file__).resolve().parents[1]
library=ROOT/'identity.so'
subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(production/'src'),str(production/'src/wf_roster_identity.c'),'-o',str(library)],check=True)
native=ctypes.CDLL(str(library));native.wf_roster_raw_identity.argtypes=[ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint8)];native.wf_roster_raw_identity.restype=ctypes.c_bool
with zipfile.ZipFile(ROMZIP) as z:
 even=z.read('31e14-0.ic18');odd=z.read('31e13-0.ic19')
rom=bytearray(len(even)+len(odd));rom[::2]=even;rom[1::2]=odd;ROM=bytes(rom)
assert hashlib.sha1(ROM).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
BASE=0x1c0000
ACT=[BASE+0x5b0+i*0x10c for i in range(9)]
D=[UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7]
A=[UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6]
class Probe:
 def __init__(self,royal=0):
  self.u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);self.u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
  self.u.mem_map(0,0x200000);self.u.mem_write(0,ROM);self.u.reg_write(UC_M68K_REG_SR,0x2700)
  self.u.reg_write(UC_M68K_REG_A7,BASE+0x3bd0);self.w(BASE+0x3bd0,0x1ff000,4)
  self.w(BASE+0x161,royal,1)
  for at in range(0x140020,0x140028,2):self.w(at,0xffff)
  self.pcs=[];self.palette=[]
  self.u.hook_add(UC_HOOK_MEM_WRITE,lambda u,ac,at,n,v,ud:self.palette.append({'pc':hex(u.reg_read(UC_M68K_REG_PC)),'address':hex(at),'value':hex(v)}),begin=0x188000,end=0x1887ff)
 def w(self,at,v,n=2):self.u.mem_write(at,int(v).to_bytes(n,'big'))
 def r(self,at,n=2):return int.from_bytes(self.u.mem_read(at,n),'big')
 def run(self,start,stops):
  self.u.reg_write(UC_M68K_REG_A7,BASE+0x3bd0);self.w(BASE+0x3bd0,0x1ff000,4)
  def code(u,at,n,ud):
   self.pcs.append(at)
   if at in stops:u.emu_stop()
  h=self.u.hook_add(UC_HOOK_CODE,code)
  self.u.emu_start(start,0x1ff000,count=100000)
  self.u.hook_del(h)
  pc=self.u.reg_read(UC_M68K_REG_PC)
  assert pc in stops or pc==0x1ff000,hex(pc)
  return hex(pc)
 def actor(self,i):
  a=ACT[i]
  return dict(active=hex(self.r(a)),slot=self.r(a+0x23,1),previous_slot=self.r(a+0x25,1),confirmed=self.r(a+0x33,1)&1,raw_type=self.r(a+2),raw_identity=hex(self.r(a+0x56)),credit=self.r(a+0xa1,1),port=hex(self.r(a+0x8a,4)),partner=hex(self.r(a+0x86,4)),restore_palette=hex(self.r(a+0xae)))
 def selection(self,slot,index=0):
  a=ACT[index];p=ACT[index^1]
  self.w(a,0x8000);self.w(a+0x22,slot);self.w(a+0x25,slot,1);self.w(a+0xa1,1,1)
  self.w(a+0x8a,0x140020,4);self.w(a+0x86,p,4);self.w(p+0x86,a,4);self.w(a+0xae,0xf0)
  self.w(BASE+0x11f4+slot*2,1)
  self.u.reg_write(A[0],a);self.u.reg_write(D[6],3-index);self.w(0x140020,0xffef)

result={'native_identity_verified':True,'rom_sha1':hashlib.sha1(ROM).hexdigest(),'mapping':[],'confirm':[],'palette':[]}
for slot in range(10):
 p=Probe(1);p.selection(slot);end=p.run(0x5da0,{0x5e82})
 raw=p.r(ACT[0]+0x56)
 assert raw==int.from_bytes(ROM[0x64cc+slot*2:0x64ce+slot*2],'big')
 native_raw=ctypes.c_uint8(255);assert native.wf_roster_raw_identity(slot,ctypes.byref(native_raw)) and native_raw.value==raw
 result['mapping'].append(dict(slot=slot,raw=raw,native_raw=native_raw.value,history=p.r(BASE+0x598),end=end))
for royal in [0,1]:
 for slot in [0,7,8,9]:
  for actor in ([0,1] if slot==8 and royal==0 else [0]):
   p=Probe(royal);p.selection(slot,actor);end=p.run(0x59d2,{0x5cd0,0x5d04,0x5d06,0x5d0a,0x5d10})
   result['confirm'].append(dict(royal=royal,slot=slot,actor=actor,end=end,actors=[p.actor(i) for i in range(2)],visited=[hex(pc) for pc in p.pcs]))
for royal in [0,1]:
 for slot in range(10):
  p=Probe(royal);p.selection(slot)
  for count in range(1,76):
   p.w(ACT[0]+0x58,count)
   p.run(0x5eae,set())
  assert len(p.palette)==75
  result['palette'].append(dict(royal=royal,slot=slot,first_five=p.palette[:5],last=p.palette[-1]))
result['setup_fixtures']=[]
for royal,slots in [(0,[0,1]),(0,[8,9]),(1,[0]),(1,[9])]:
 p=Probe(royal)
 # Fixture is explicitly declared; these values are NOT a production reset/seed.
 p.w(BASE+0x7c,0x8000);p.w(BASE+0x5c,0x12345678,4);p.w(BASE+0x80,0x1020304,4)
 for i in range(10):p.w(BASE+0x598+i*2,0xffff)
 for i,slot in enumerate(slots):
  p.w(ACT[i],0x8000);p.w(ACT[i]+0x22,slot);p.w(ACT[i]+0xa1,int(i==0),1)
  p.w(ACT[i]+0x8a,0x140020,4);p.w(ACT[i]+0x86,ACT[i^1],4)
 p.run(0x5da0,{0x5e82});p.run(0xd1c,set())
 before=dict(seed=p.r(BASE+0x5c,4),global80=p.r(BASE+0x80,4),d=[p.u.reg_read(r) for r in D],a=[p.u.reg_read(r) for r in A])
 p.run(0x1086c,set())
 result['setup_fixtures'].append(dict(royal=royal,slots=slots,before=before,actors=[p.actor(i) for i in range(9)],history=[p.r(BASE+0x598+i*2) for i in range(10)],seed_after=p.r(BASE+0x5c,4)))
(ROOT/'oracle_results.json').write_text(json.dumps(result,indent=2)+'\n')
print('Exact ROM blocks verified: 10 slot mappings,',len(result['confirm']),'confirmation fixtures,',len(result['palette'])*75,'palette writes')
for row in result['confirm']: print({k:v for k,v in row.items() if k!='visited'})
