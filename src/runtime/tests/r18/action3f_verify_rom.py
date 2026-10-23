#!/usr/bin/env python3
"""Raw $3F versus the intact arcade $1C03E/$1836E owner.

Missing dispatch, timer signedness, effect row/position/queue, global side
selection, or D-register clobbers must fail full RAM and D0-D7 comparison.
No owner or branch is patched. The ROM sound-device exception is unused.
"""
import argparse, ctypes, hashlib, itertools, json, random, re, struct, subprocess, tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,default=ROOT/'audit/runtime18-cd/ROM68K.BIN');p.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('action3f_snapshot_f88acad.bin'));p.add_argument('--report',type=Path);args=p.parse_args()
rom=args.rom.read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert rom[0x12710:0x12714]==bytes.fromhex('0001835e')
rng=random.Random(0x1836e);actors=[0x1c05b0+i*0x10c for i in range(9)];coverage=set();continuations=0

def put(m,at,value,n=2):m[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')

def natural():
 data=args.snapshot.read_bytes();manifest=json.loads(Path(__file__).with_name('action3f_manifest.json').read_text())
 assert hashlib.sha256(data).hexdigest()==manifest['sha256'];assert len(data)==16472
 frame,slot,kind,pc,seq,status=struct.unpack_from('=6I',data);assert (frame,slot,kind,pc,seq,status)==(703,1,1,0x1c03e,141,1)
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=data[56:8248]
 assert m[actors[slot]+0x61]==0x3f
 return ('natural-frame703-slot1',m,slot,list(struct.unpack_from('=8I',data,24)))

def seed(slot):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 for i,a in enumerate(actors):
  put(m,a,0x8000);put(m,a+2,i);put(m,a+0x26,0,4)
 a=actors[slot];put(m,a+0x20,5);put(m,a+0x1c,0x8005);put(m,a+0x60,0x3f);put(m,a+0x24,0);put(m,a+0x22,0xff00);put(m,a+0x2a,18);put(m,a+0x2e,0)
 put(m,a+0x19,24,1);put(m,a+6,0x180);put(m,a+10,0x90);put(m,a+14,0)
 put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(rom[0x280a+k*8:0x280e+k*8],'big'),0)
 for i in range(12):m[0x1c1258+i*0x2a]&=0x7f
 return m,a

def fixtures():
 yield natural()
 # Init uses unsigned X compare and leaves D1-D7 alone.
 for slot,x,face,sel in itertools.product((0,1,5,8),(0,0x27f,0x280,0xffff),(0,0x8000),(0x3f,0x403f,0x803f,0xc03f)):
  m,a=seed(slot);put(m,a+0x1c,5);put(m,a+6,x);put(m,a+0x2e,face);put(m,a+0x60,sel)
  yield (f'init-{slot}-{x:x}-{face:x}-{sel:x}',m,slot)
 # Every phase/timer transition and signed magnitude boundary, with the
 # compact pool full, so this isolates the callback from effect allocation.
 for slot,frame,timer,magnitude in itertools.product((0,1,5,8),(0,1,2,0xfe),(0,1,12,0xff00),(0,1,18,0x7fff,0x8000,0xffff)):
  m,a=seed(slot);put(m,a+0x24,frame);put(m,a+0x22,timer);put(m,a+0x2a,magnitude)
  for i in range(11):m[0x1c1258+i*0x2a]|=0x80
  yield (f'phase-{slot}-{frame}-{timer}-{magnitude}',m,slot)
 # The first available compact slot, full pool guard, sign extension of the
 # actor X offset, cross-record facing read, clipping and existing queue.
 for free,face,offset,nextface,x,y,scroll in itertools.product((0,5,10,11),(0,0x8000),(0x80,0xff,0,0x7f),(0,0x80),(0x180,0x7fff),(0x90,0x7fff),(0,0x100)):
  m,a=seed(1);put(m,a+0x19,offset,1);put(m,a+0x2e,face);put(m,a+6,x);put(m,a+10,y);put(m,0x1c17e6,scroll);put(m,0x1c17ee,scroll)
  for i in range(11):m[0x1c1258+i*0x2a]|=0x80
  if free<11:m[0x1c1258+free*0x2a]&=0x7f;put(m,0x1c1258+free*0x2a+0x2e,nextface,1)
  put(m,0x1c19b8,2)
  yield (f'effect-{free}-{face:x}-{offset:x}-{nextface:x}-{x:x}-{y:x}-{scroll:x}',m,1)
 # State FF is stopped by the dispatcher before dereferencing this action.
 m,a=seed(8);put(m,a+0x20,0xff);yield ('state-ff',m,8)

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-action3f-') as temp:
 lib=Path(temp)/'action3f.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action3f_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action3f_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 count=0
 def compare(case):
  global count
  name,m,slot=case[:3];regs=case[3] if len(case)>3 else [rng.getrandbits(32) for _ in range(8)];count+=1
  nb=ctypes.create_string_buffer(bytes(m));nr=(ctypes.c_uint32*8)(*regs)
  status=native.wf_action3f_oracle(nb,nr,slot)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,bytes(m))
  for i in range(8,15):cpu.w_reg(i,0)
  for i,value in enumerate(regs):cpu.w_reg(i,value)
  cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for _ in range(10000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
  assert status==0,(name,'native action3f blocked',hex(-status))
  for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
   want=bytes(mem.r_block(at,n));got=nb.raw[at:at+n]
   assert got==want,(name,'RAM',hex(at),[(hex(at+i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:16])
  wantregs=[cpu.r_reg(i) for i in range(8)]
  assert list(nr)==wantregs,(name,'D0-D7',list(nr),wantregs)
  result=bytearray(nb.raw[:len(m)]);machine.cleanup();return result,list(nr)
 for case in fixtures():compare(case)
 # Unedited natural root evolves through every subsequent action3F tick.
 name,m,slot,regs=natural()
 for tick in range(80):
  m,regs=compare((f'natural-continuation-{tick}',m,slot,regs));continuations+=1
  if m[actors[slot]+0x21]!=5:break
 else:raise AssertionError('natural action3f did not terminate in 80 ticks')
 required=(0x18376,0x1839c,0x183a6,0x183b0,0x183c0,0x183c6,0x183d2,0x183da,0x10d48,0x10d5a,0x10d5e,0x10d9e,0x10da6,0x10dc6,0x10dcc,0x27da,0x27e0,0x27fc,0x2800,0x1c0dc,0x1c0f4,0x1c0fa,0x1c10a,0x1c0ba)
 assert all(pc in coverage for pc in required),('missing ROM branch coverage',[hex(pc) for pc in required if pc not in coverage])
 if args.report:
  args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps({'count':count,'passed':count,'natural_continuations':continuations,'rom_sha1':hashlib.sha1(rom).hexdigest(),'snapshot_sha256':hashlib.sha256(args.snapshot.read_bytes()).hexdigest(),'compared_memory':['$1C0000..$1C1FFF','$0C0000..$0C3FFF'],'compared_registers':'D0-D7','required_branch_sites':[hex(pc) for pc in required],'covered_branch_sites':[hex(pc) for pc in required if pc in coverage],'intercepted_rom_owners':[],'scope':'selector$3F callback$1836E; intact callback/descriptor/effect/queue and natural continuations'},indent=2)+'\n')
 print(f'PASS {count} action3f intact-ROM fixtures; {continuations} natural continuation ticks; RAM/FG/D0-D7; {len(required)} branch sites')
