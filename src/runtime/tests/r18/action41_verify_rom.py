#!/usr/bin/env python3
"""Beetle frame320 raw41 and branch matrix against intact ROM1C03E."""
import argparse,ctypes,hashlib,itertools,json,os,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--snapshot-only',action='store_true');args=p.parse_args()
meta=json.loads((HERE/'action41_manifest.json').read_text());rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert rom[0x12614+0x41*4:0x12618+0x41*4]==bytes.fromhex('0001848c');assert rom[0x1848c:0x18498]==bytes.fromhex('0001849800010001000e01a4')
assert rom[0x21]==0xff,'Tagged null state BYTE reads ROM reset-vector area'
capture=(HERE/'action41_snapshot_beetle_frame320.bin').read_bytes();assert len(capture)==8248 and hashlib.sha256(capture).hexdigest()==meta['snapshot_sha256'];assert list(struct.unpack_from('=6I',capture))==meta['header']
actors=[0x1c05b0+0x10c*i for i in range(9)];rng=random.Random(0x18498);coverage=set()
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(slot=4):
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000);a=actors[slot]
 for off in(0x1c,0x20):put(m,a+off,0x8005)
 put(m,a+0x60,0x41);put(m,a+0x92,0,4);put(m,a+0x36,0);put(m,a+0x24,0);put(m,a+0x22,1);put(m,0x1c0161,0,1)
 return m,a
def fixtures():
 m=bytearray(0x200000);m[:len(rom)]=rom;m[0x1c0000:0x1c2000]=capture[56:];yield 'observed-Beetle-frame320',m,4,struct.unpack_from('=8I',capture,24)
 if args.snapshot_only:return
 for slot,latch,selector,index,timer,face in itertools.product((0,4,8),(5,0x8005),(0x41,0x4041,0x8041,0xff41),(0,1,0xfe,0x100,0x1fe,0xffff),(0,1,14,0xff00,0xffff),(0,0x8000,0xffff)):
  m,a=seed(slot);put(m,a+0x1c,latch);put(m,a+0x60,selector);put(m,a+0x24,index);put(m,a+0x22,timer);put(m,a+0x2e,face)
  yield f'phase-{slot}-{latch:x}-{selector:x}-{index:x}-{timer:x}-{face:x}',m,slot
 for slot,relation,state,sub,index,timer,selector in itertools.product((0,4,8),('none','self','peer','tagged'),(4,0x8004,5),(0x1d,0x801d,0x1c),(0,0xfe,0x1fe),(0,12,0xff00),(0x41,0x8041)):
  m,a=seed(slot);target=a if relation=='self'else actors[(slot+4)%9];ptr=0 if relation=='none'else target|(0x80000000 if relation=='tagged'else 0)
  put(m,target+0x20,state);put(m,target+0x64,sub);put(m,a+0x92,ptr,4);put(m,a+0x24,index);put(m,a+0x22,timer);put(m,a+0x60,selector)
  yield f'link-{slot}-{relation}-{state:x}-{sub:x}-{index:x}-{timer:x}-{selector:x}',m,slot
 for mode,kind,bits in itertools.product(range(4),(0,1,3,5,0x81,0xff),range(256)):
  m,a=seed();put(m,0x1c0161,mode,1);put(m,a+0x36,kind,1);put(m,a+0x37,bits,1);yield f'contact-{mode}-{kind:x}-{bits:x}',m,4
 for mode,kind,bits,left,right in itertools.product(range(4),(1,5),(1,2,3),(0,0x100,1,0x101,0xff00,0xffff),(0,0x100,1,0x101,0xff00,0xffff)):
  m,a=seed();put(m,0x1c0161,mode,1);put(m,a+0x36,kind,1);put(m,a+0x37,bits,1);put(m,0x1c1150,left);put(m,0x1c11b0,right);yield f'rope-{mode}-{kind}-{bits}-{left:x}-{right:x}',m,4
 for ptr in(0x80000000,0xff000000,0x011c05b0,0xff1c05b0):
  m,a=seed();put(m,a+0x92,ptr,4);put(m,actors[0]+0x20,0x8004);put(m,actors[0]+0x64,0xff1d);yield f'bus-alias-{ptr:x}',m,4
 for ptr,latch in itertools.product((0x1c0001,0x1c05b1,0x1c0f1c,0xdeadbeef),(5,0x8005)):
  m,a=seed();put(m,a+0x92,ptr,4);put(m,a+0x1c,latch);put(m,a+0x60,0x8041);yield f'unread-relation-{ptr:x}-{latch:x}',m,4
 for mode,kind,bits,index,selector in itertools.product(range(4),(1,3,5),(1,2,3,0x10),(0,0xfe),(0x41,0x8041)):
  m,a=seed();put(m,a+0x36,kind,1);put(m,a+0x37,bits,1);put(m,a+0x92,actors[0],4);put(m,a+0x24,index);put(m,a+0x60,selector);put(m,0x1c0161,mode,1);put(m,actors[0]+0x20,0x8004);put(m,actors[0]+0x64,0xff1d)
  yield f'priority-{mode}-{kind}-{bits}-{index:x}-{selector:x}',m,4
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-action41-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 (frozen/'tests/r18').mkdir(parents=True);shutil.copy2(HERE/'action41_oracle_bridge.c',frozen/'tests/r18/action41_oracle_bridge.c')
 frozen_source=(frozen/'src/wf_r17_actions_r11.c').read_bytes();source_hash=hashlib.sha256(frozen_source).hexdigest()
 def owner_hash(data):
  start=data.index(b'/* $12614[$41]');end=data.index(b'/* $12614[$75]',start)
  return hashlib.sha256(data[start:end]+re.search(rb'^[ \t]*case 0x41u:[^\n]*',data,re.M)[0]).hexdigest()
 raw41_hash=owner_hash(frozen_source)
 lib=frozen/'action41.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action41_oracle_bridge.c','-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib));native.wf_action41_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,memory=machine.cpu,machine.mem;memory.w_block(0,rom);count=0
 try:
  for case in fixtures():
   name,mem,slot=case[:3];regs=case[3]if len(case)>3 else[rng.getrandbits(32)for _ in range(8)];count+=1
   mem[0xc0000:0xc4000]=rng.randbytes(0x4000);memory.w_block(0x1c0000,bytes(mem[0x1c0000:0x1c2000]));memory.w_block(0xc0000,bytes(mem[0xc0000:0xc4000]))
   for i in range(15):cpu.w_reg(i,regs[i]if i<8 else 0)
   cpu.w_reg(8,actors[slot]);cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e);descriptor=0
   for _ in range(2000):
    pc=cpu.r_pc();coverage.add(pc)
    if pc==0x1c0c8:descriptor=cpu.r_reg(9)
    if pc==0x70000:break
    if pc==0x2052:cpu.w_pc(memory.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
    cpu.execute(1)
   assert cpu.r_pc()==0x70000,(name,'ROM runaway',hex(cpu.r_pc()))
   nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs);nd=ctypes.c_uint32();status=native.wf_action41_oracle(nb,nr,slot,ctypes.byref(nd))
   assert status==0,(name,'native blocked',hex(-status));assert nd.value==descriptor,(name,'descriptor',hex(nd.value),hex(descriptor))
   for at,n in((0x1c0000,0x2000),(0xc0000,0x4000)):
    got=nb.raw[at:at+n];want=bytes(memory.r_block(at,n));assert got==want,(name,'RAM/FG',[(hex(at+i),g,e)for i,(g,e)in enumerate(zip(got,want))if g!=e][:16])
   assert list(nr)==[cpu.r_reg(i)for i in range(8)],(name,'D0-D7',list(nr),[cpu.r_reg(i)for i in range(8)])
   assert nb.raw[0x1c005c:0x1c0060]==mem[0x1c005c:0x1c0060],(name,'RNG changed')
  guards=0
  if not args.snapshot_only:
   for ptr in(0x1c0001,0x1c05b1,0x1c0f1c,0xdeadbeef):
    mem,a=seed();put(mem,a+0x92,ptr,4);nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*range(8));nd=ctypes.c_uint32()
    assert native.wf_action41_oracle(nb,nr,4,ctypes.byref(nd))==-0x184ce and nd.value==0;guards+=1
  assert bytes(memory.r_block(0,len(rom)))==rom
 finally:machine.cleanup()
 required={0x18498,0x184a0,0x184a8,0x184b0,0x184c2,0x184ce,0x184e2,0x184f8,0x18502,0x1850a,0x10ff0,0x11012,0x11024}
 if not args.snapshot_only:assert required<=coverage,('missing ROM branches',[hex(x)for x in sorted(required-coverage)])
 report={'status':'PASS','cases':count,'rom_sha1':meta['rom_sha1'],'snapshot_sha256':meta['snapshot_sha256'],'frozen_actions_sha256':source_hash,'current_actions_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),'compared':['All RAM1C0000..1C1FFF','FG0C0000..0C3FFF','D0-D7','RNG','tick descriptor'],'covered_rom_sites':[hex(x)for x in sorted(required&coverage)],'intercepts':['2052 sound output only'],'limits':['Beetle typed-state projection plus constructed owner boundaries, not a HOST all-actors match','No full scheduler, physical audio or Saturn timing equivalence claim']}
 assert owner_hash((ROOT/'src/wf_r17_actions_r11.c').read_bytes())==raw41_hash,'Raw41 owner changed while oracle ran'
 report.update(raw41_owner_sha256=raw41_hash,guard_controls=guards)
 if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
