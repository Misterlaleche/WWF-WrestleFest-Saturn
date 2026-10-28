#!/usr/bin/env python3
"""Full TAG count-frame $1F914, its real display/bonus owners and next $21B4.
No ROM patches/hooks and no decision mocks. The native sound device is silent.
"""
import argparse,ctypes,hashlib,json,random,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,default=root/'audit/runtime18-cd/ROM68K.BIN');p.add_argument('--royal',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--snapshot',type=Path);p.add_argument('--released-link',action='store_true');p.add_argument('--report',type=Path);a=p.parse_args()
if a.released_link and not a.royal:p.error('--released-link requires --royal')
rom=a.rom.read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
rng=random.Random(0x20022)
def w(b,a,v):b[a:a+2]=(v&65535).to_bytes(2,'big')
def l(b,a,v):b[a:a+4]=(v&0xffffffff).to_bytes(4,'big')
core=next(line[5:].split() for line in (root/'Makefile').read_text().splitlines() if line.startswith('CORE='))
extra=['src/wf_r18_referee.c','src/wf_r18_match_result.c','src/wf_r18_video.c','tests/r18/referee_count_chain_probe.c']
with tempfile.TemporaryDirectory(prefix='wf-ref-count-') as tmp:
 lib=Path(tmp)/'referee.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O0' if a.quick else '-O1','-Wall','-Wextra','-Werror','-Iinclude',*dict.fromkeys(core+extra),'-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.referee_count_chain.argtypes=[ctypes.c_void_p]*5
 romc=ctypes.create_string_buffer(rom)
 cases=[(phase,timer,entry,config,candidate) for phase in range(7) for timer in (1,2,255) for entry in (False,True) for config in (range(1) if a.royal else range(4)) for candidate in ((False,True) if a.royal else (False,))]
 # Overflow case protects original ADDQ.B at $20082 (the low byte must survive).
 cases += [(1,1,False,0,False)]
 if a.quick or a.snapshot:cases=[(5,1,False,0,True)]
 released=[(ptr,timer) for ptr in (0,0x80000000,0x1c09e0) for timer in (0,1,2,255)]+[(0xdeadbeef,2),(0xdeadbeef,255)]
 if a.released_link:cases=[(6,timer,False,0,False) for ptr,timer in released]
 coverage=set()
 ignored_rom_writes=set()
 for case,(phase,timer,entry,config,candidate) in enumerate(cases):
  ram=bytearray(0x2000);fg=rng.randbytes(0x4000);regs=[rng.getrandbits(32) for _ in range(8)]
  ram[0x5b0:0xf1c]=rng.randbytes(9*0x10c);ram[0x11f4:0x1300]=rng.randbytes(0x10c)
  l(ram,0x5c,rng.getrandbits(32));w(ram,0x66,config<<13);w(ram,0x7c,1);l(ram,0x80,rng.getrandbits(32));w(ram,0x169a,0xffab if case==len(cases)-1 else 0xab00+phase//2);w(ram,0x169e,0x8000)
  ram[0x161]=int(a.royal)
  w(ram,0x17e6,0x140);w(ram,0x17ee,0x200)
  for slot in range(9):
   at=0x5b0+slot*0x10c;w(ram,at,0x8000);l(ram,at+0x26,0x1c05b0+4*0x10c);l(ram,at+0x86,0x1c05b0+(slot^1)*0x10c if slot<8 else 0x1c05b0);ram[at+0x35]|=1
   if a.royal:l(ram,at+0x7e,0);w(ram,at+0x60,0x48)
  ref=0x11f4;w(ram,ref,0x8000);w(ram,ref+2,12);w(ram,ref+4,5 if phase%2 else 6);w(ram,ref+6,0x280);w(ram,ref+10,0x180);w(ram,ref+14,0x140);w(ram,ref+0x12,0)
  w(ram,ref+0x1c,6 if entry else 0x8006);w(ram,ref+0x20,6);w(ram,ref+0x24,phase);ram[ref+0x23]=timer;w(ram,ref+0x54,0x3162+phase//2);l(ram,ref+0x56,0x1c05b0)
  if a.royal:
   # $200E2 retains A0=referee; offsets beyond +$64 alias the effect pool.
   l(ram,ref+0x7e,0x1c05b0+7*0x10c if candidate else 0);ram[ref+0x61]=0
  if a.snapshot:
   snapshot=a.snapshot.read_bytes();ram[:]=snapshot[56:8248];regs=list(struct.unpack_from('=8I',snapshot,24));assert len(ram)==0x2000 and ram[0x161]==1
  if a.released_link:
   ptr,timer=released[case];w(ram,ref+0x24,6);ram[ref+0x23]=timer
   pinning=int.from_bytes(ram[ref+0x56:ref+0x5a],'big')&0xffffff
   l(ram,pinning-0x1c0000+0x26,ptr)
  m=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=m.cpu,m.mem;mem.w_block(0,rom);mem.w_block(0x1c0000,bytes(ram));mem.w_block(0xc0000,fg)
  # Arcade program ROM is read-only even when an expired grapple points at zero.
  for page in range(0,len(rom),0x10000):
   for width in range(3):
    mem.set_special_range_write_func(page,width,lambda address,value:ignored_rom_writes.add(address))
    mem.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  def call(pc):
   cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
   for _ in range(20000):
    if cpu.r_pc()==0x70000:return
    coverage.add(cpu.r_pc())
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
 if a.released_link:
  assert {0x2006c,0x2014a,0x20154}<=coverage,('missing released-link paths',coverage)
  assert ignored_rom_writes=={0x109},('unexpected ROM bus writes',ignored_rom_writes)
 elif a.royal and not a.snapshot:assert {0x21358,0x21368,0x200ec}<=coverage,('missing Royal cleanup coverage',coverage)
 report={'status':'PASS','cases':len(cases),'mode':'Royal' if a.royal else 'TAG','rom_pcs':[hex(pc) for pc in sorted(coverage)],'rom_sha1':hashlib.sha1(rom).hexdigest(),'ignored_rom_writes':[hex(at) for at in sorted(ignored_rom_writes)],'snapshot':str(a.snapshot) if a.snapshot else None,'released_link':a.released_link,'compared':['work RAM','render queue','FG0','D0-D7','next RNG']}
 if a.report:a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
 print(f"PASS {len(cases)} complete ROM $1F914 {report['mode']} count frames + next $21B4: full work RAM/render queue, FG0, D0-D7 and RNG")
