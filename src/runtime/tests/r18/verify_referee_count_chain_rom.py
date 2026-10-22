#!/usr/bin/env python3
"""Full TAG count-frame $1F914, its real display/bonus owners and next $21B4.
No ROM patches/hooks and no decision mocks. The native sound device is silent.
"""
import argparse,ctypes,hashlib,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,default=root/'audit/runtime18-cd/ROM68K.BIN');a=p.parse_args()
rom=a.rom.read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x20022)
def w(b,a,v):b[a:a+2]=(v&65535).to_bytes(2,'big')
def l(b,a,v):b[a:a+4]=(v&0xffffffff).to_bytes(4,'big')
core=next(line[5:].split() for line in (root/'Makefile').read_text().splitlines() if line.startswith('CORE='))
extra=['src/wf_r18_referee.c','src/wf_r18_match_result.c','src/wf_r18_video.c','tests/r18/referee_count_chain_probe.c']
with tempfile.TemporaryDirectory(prefix='wf-ref-count-') as tmp:
 lib=Path(tmp)/'referee.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O1','-Wall','-Wextra','-Werror','-Iinclude',*dict.fromkeys(core+extra),'-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.referee_count_chain.argtypes=[ctypes.c_void_p]*5
 romc=ctypes.create_string_buffer(rom)
 cases=[(phase,timer,entry,config) for phase in range(7) for timer in (1,2,255) for entry in (False,True) for config in range(4)]
 # Overflow case protects original ADDQ.B at $20082 (the low byte must survive).
 cases += [(1,1,False,0)]
 for case,(phase,timer,entry,config) in enumerate(cases):
  ram=bytearray(0x2000);fg=rng.randbytes(0x4000);regs=[rng.getrandbits(32) for _ in range(8)]
  ram[0x5b0:0xf1c]=rng.randbytes(9*0x10c);ram[0x11f4:0x1300]=rng.randbytes(0x10c)
  l(ram,0x5c,rng.getrandbits(32));w(ram,0x66,config<<13);w(ram,0x7c,1);l(ram,0x80,rng.getrandbits(32));w(ram,0x169a,0xffab if case==len(cases)-1 else 0xab00+phase//2);w(ram,0x169e,0x8000)
  w(ram,0x17e6,0x140);w(ram,0x17ee,0x200)
  for slot in range(9):
   at=0x5b0+slot*0x10c;w(ram,at,0x8000);l(ram,at+0x26,0x1c05b0+4*0x10c);l(ram,at+0x86,0x1c05b0+(slot^1)*0x10c if slot<8 else 0x1c05b0);ram[at+0x35]|=1
  ref=0x11f4;w(ram,ref,0x8000);w(ram,ref+2,12);w(ram,ref+4,5 if phase%2 else 6);w(ram,ref+6,0x280);w(ram,ref+10,0x180);w(ram,ref+14,0x140);w(ram,ref+0x12,0)
  w(ram,ref+0x1c,6 if entry else 0x8006);w(ram,ref+0x20,6);w(ram,ref+0x24,phase);ram[ref+0x23]=timer;w(ram,ref+0x54,0x3162+phase//2);l(ram,ref+0x56,0x1c05b0)
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem;mem.w_block(0,rom);mem.w_block(0x1c0000,bytes(ram));mem.w_block(0xc0000,fg)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  def call(pc):
   cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
   for _ in range(20000):
    if cpu.r_pc()==0x70000:return
    cpu.execute(1)
   raise AssertionError(('ROM runaway',case,hex(cpu.r_pc())))
  call(0x1f914);want_regs=[cpu.r_reg(i) for i in range(8)];want_fg=bytes(mem.r_block(0xc0000,0x4000));call(0x21b4);want_after=[cpu.r_reg(i) for i in range(8)];want_ram=bytes(mem.r_block(0x1c0000,0x2000));m.cleanup()
  nr=ctypes.create_string_buffer(bytes(ram));nf=ctypes.create_string_buffer(fg);nd=(ctypes.c_uint32*8)(*regs);after=(ctypes.c_uint32*8)()
  status=native.referee_count_chain(nr,nf,nd,romc,after);assert status==0,(case,phase,timer,entry,'native blocked',status)
  # Complete work RAM, including normalized native render-queue pointers.
  for start,end in ((0,0x2000),):
   diff=[(hex(i),nr.raw[i],want_ram[i]) for i in range(start,end) if nr.raw[i]!=want_ram[i]]
   assert not diff,(case,phase,timer,entry,'RAM',diff[:12])
  assert nf.raw[:0x4000]==want_fg,(case,'FG0')
  assert list(nd)==want_regs,(case,phase,timer,entry,'D0-D7',list(map(hex,nd)),list(map(hex,want_regs)))
  assert list(after)==want_after,(case,'next RNG registers')
 print(f'PASS {len(cases)} complete ROM $1F914 TAG count frames + next $21B4: full work RAM/render queue, FG0, D0-D7 and RNG')
