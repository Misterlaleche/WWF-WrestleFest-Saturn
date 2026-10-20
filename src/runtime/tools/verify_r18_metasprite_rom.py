#!/usr/bin/env python3
"""Differential D1FC execution: native record parser versus original ROM.
No ROM code hooks or intercepted calls. Compares D6/D7, A2 record count,
and every WP26-consumed sprite field; unused word high bytes are normalized.
"""
import argparse,ctypes,random,subprocess,tempfile,sys
from pathlib import Path
from wf_r18_rom_graphics import Rom,be16
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_MEM_READ_UNMAPPED
from unicorn.m68k_const import *
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--cases',type=int,default=300);a=p.parse_args();rom=Rom(a.rom);root=Path(__file__).resolve().parents[1]
CB=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t)
class Record(ctypes.Structure):_fields_=[('bytes',ctypes.c_uint8*16)]
class Ctx(ctypes.Structure):_fields_=[('read',CB),('user',ctypes.c_void_p),('palette',ctypes.POINTER(ctypes.c_uint8)),('records',ctypes.POINTER(Record)),('capacity',ctypes.c_size_t),('count',ctypes.c_size_t),('d6',ctypes.c_uint32),('d7',ctypes.c_uint32)]
source=ctypes.create_string_buffer(rom.p)
@CB
def read(user,at,dst,n):
 if at+n>len(rom.p):return 1
 ctypes.memmove(dst,ctypes.addressof(source)+at,n);return 0
rng=random.Random(0xd1fc18);cases=[]
for s,_,_,n in rom.active_sets():
 for pose in range(n):
  ent=rom.pose_entry(s,pose)
  if ent not in (None,65534,65535):cases.append((s,pose,be16(rom.p,ent)))
# Ensure each record class and flags is present, then sample across all sets.
selected=[]
for kind in range(4):
 for flag in (0,256,512,768):
  candidates=[c for c in cases if c[2]>>14==kind and c[2]&768==flag]
  if candidates:selected.extend(rng.sample(candidates,min(5,len(candidates))))
selected+=rng.sample(cases,min(a.cases,len(cases)))
counts={};blocked=[]
with tempfile.TemporaryDirectory() as tmp:
 lib=Path(tmp)/'meta.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'src/wf_r18_metasprite.c'),'-o',str(lib)],check=True);native=ctypes.CDLL(str(lib));native.wf_r18_metasprite_emit.argtypes=[ctypes.POINTER(Ctx)]+[ctypes.c_uint16]*5
 for i,(s,pose,h) in enumerate(selected):
  partner=rng.randrange(12);mirror=i%2;x,y=rng.choice([(150,100),(0,0),(1000,100),(100,1000),(65535,65535),(rng.randrange(65536),rng.randrange(65536))]);seed=rng.getrandbits(32)
  palette=bytes(rng.randrange(16) for _ in range(256));pal=(ctypes.c_uint8*256).from_buffer_copy(palette);records=(Record*1024)();ctx=Ctx(read,None,pal,records,1024,0,seed,(seed+1)&0xffffffff)
  result=native.wf_r18_metasprite_emit(ctypes.byref(ctx),s,pose|(mirror<<15),x,y,partner)
  if result:
   blocked.append((s,pose,hex(h)));assert ctx.d6==seed and ctx.count==0;continue
  u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,rom.p)
  def mirror_read(cpu,access,address,size,value,user):
   physical=address&0xffffff;page=address&~0xfff;lowpage=physical&~0xfff
   if lowpage>=len(rom.p):return False
   cpu.mem_map(page,0x1000);cpu.mem_write(page,rom.p[lowpage:lowpage+0x1000]);return True
  u.hook_add(UC_HOOK_MEM_READ_UNMAPPED,mirror_read)
  def w(at,v,n=2):u.mem_write(at,(v&((1<<(8*n))-1)).to_bytes(n,'big'))
  actor=0x1c05b0;other=0x1c06b0;w(actor+2,s);w(actor+4,pose|(mirror<<15));w(actor+0x14,x);w(actor+0x16,y);w(actor+0x26,other,4);w(other+2,partner);u.mem_write(0x1c1610,palette)
  for j in range(8):u.reg_write(UC_M68K_REG_D0+j,(seed+j*0x11111111)&0xffffffff)
  u.reg_write(UC_M68K_REG_D6,seed);u.reg_write(UC_M68K_REG_D7,(seed+1)&0xffffffff);u.reg_write(UC_M68K_REG_A0,actor);u.reg_write(UC_M68K_REG_A2,0xc2000);u.reg_write(UC_M68K_REG_A7,0x1d0000);w(0x1d0000,0x70000,4)
  try:u.emu_start(0xd1fc,0x70000,count=500000)
  except Exception as e:raise RuntimeError((i,s,pose,hex(h),partner,mirror,x,y,hex(u.reg_read(UC_M68K_REG_PC)))) from e
  assert u.reg_read(UC_M68K_REG_PC)==0x70000,('ROM instructionlimit',s,pose,hex(h))
  wanted=(u.reg_read(UC_M68K_REG_D6),u.reg_read(UC_M68K_REG_D7),(u.reg_read(UC_M68K_REG_A2)-0xc2000)//16)
  got=(ctx.d6,ctx.d7,ctx.count)
  assert got==wanted,(i,s,pose,hex(h),partner,mirror,x,y,'register/count',got,wanted)
  raw=bytes(u.mem_read(0xc2000,ctx.count*16))
  for j in range(ctx.count):
   native_bytes=bytes(records[j].bytes);expected=raw[j*16:j*16+16]
   for off in (0,1,3,5,7,9,10,11):assert native_bytes[off]==expected[off],(i,s,pose,hex(h),partner,mirror,x,y,'record',j,off,native_bytes.hex(),expected.hex())
  key=(h>>14,h&768);counts[key]=counts.get(key,0)+1
print('PASS',sum(counts.values()),'direct-ROM cases; classes',counts,'blocked',len(blocked),blocked[:20])
