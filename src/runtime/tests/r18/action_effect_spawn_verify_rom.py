"""Action 65/73/74 effect sprites versus intact ROM RAM, FG and registers."""
import argparse,ctypes,hashlib,itertools,json,random,re,subprocess,tempfile
from pathlib import Path
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
ROM=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(ROM).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
FAMILY={0x35:0x175a8,0x65:0x19664,0x73:0x1a014,0x74:0x1a050}
p=argparse.ArgumentParser();p.add_argument('--selector',type=lambda x:int(x,0));p.add_argument('--fast',action='store_true');args=p.parse_args()
selected=[args.selector] if args.selector is not None else list(FAMILY)
for selector in FAMILY:
    assert int.from_bytes(ROM[0x12614+selector*4:0x12618+selector*4],'big')==FAMILY[selector]
rng=random.Random(0x1073a74)
ACTORS=[0x1c05b0+i*0x10c for i in range(9)]
def put(m,at,v,n=2):m[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
def seed(selector,slot,latch,frame,timer,full,face,offset):
 m=bytearray(0x200000);m[:len(ROM)]=ROM;m[0x1c0000:0x1c2000]=rng.randbytes(0x2000)
 m[0xc0000:0xc4000]=rng.randbytes(0x4000)
 for i,a in enumerate(ACTORS):put(m,a,0x8000);put(m,a+2,i);put(m,a+0x26,ACTORS[(i+1)%9],4)
 a=ACTORS[slot];put(m,a+0x20,5);put(m,a+0x1c,latch);put(m,a+0x60,selector)
 if selector==0x35:
  put(m,a+0x44,3);put(m,a+0x37,0,1)
 put(m,a+0x24,frame);put(m,a+0x22,timer);put(m,a+0x2e,face)
 put(m,a+0x19,offset,1);put(m,a+6,0x180);put(m,a+10,0x100);put(m,a+14,0x140)
 put(m,0x1c17e6,0);put(m,0x1c17ee,0)
 for k in range(6):put(m,int.from_bytes(ROM[0x280a+k*8:0x280e+k*8],'big'),0)
 for i in range(11):m[0x1c1258+i*0x2a]|=0x80
 if not full:m[0x1c1258]&=0x7f
 return m
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-effects-')as td:
 lib=Path(td)/'effects.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O0' if args.fast else '-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,
                 'tests/r18/action_effect_spawn_oracle_bridge.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));native.wf_effect_spawn_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=machine.cpu,machine.mem;count=0
 matrix=((selector,slot,latch,frame,timer,full,face,offset)
         for selector in selected
         for slot,latch,frame,timer,full,face,offset in itertools.product(
          (0,1,8),(0x8005,) if selector==0x35 else (5,0x8005),
          (4,0xfe) if selector==0x35 else (0,0xfe),
          (0,1),(0,1),(0,0x8000),(0,0x80)))
 for selector,slot,latch,frame,timer,full,face,offset in matrix:
  m=seed(selector,slot,latch,frame,timer,full,face,offset)
  regs=[rng.getrandbits(32)for _ in range(8)];ram.w_block(0,bytes(m))
  for i in range(8,15):cpu.w_reg(i,0)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_reg(8,ACTORS[slot]);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  descriptor=0
  for _ in range(20000):
   if cpu.r_pc()==0x70000:break
   if cpu.r_pc()==0x1c0c8:descriptor=cpu.r_reg(9)
   cpu.execute(1)
  tag=(hex(selector),slot,latch,frame,timer,full,face,offset)
  assert cpu.r_pc()==0x70000,(tag,'ROM runaway',hex(cpu.r_pc()))
  nb=ctypes.create_string_buffer(bytes(m));nr=(ctypes.c_uint32*8)(*regs);desc=ctypes.c_uint32()
  status=native.wf_effect_spawn_oracle(nb,nr,slot,ctypes.byref(desc))
  assert status==0,(tag,'native blocked',hex(-status))
  assert desc.value==descriptor,(tag,'descriptor',hex(desc.value),hex(descriptor))
  for at,n in ((0x1c0000,0x2000),(0xc0000,0x4000)):
   got=nb.raw[at:at+n];want=bytes(ram.r_block(at,n))
   assert got==want,(tag,'RAM/FG',[(hex(at+i),x,y)for i,(x,y)in enumerate(zip(got,want))if x!=y][:12])
  assert list(nr)==[cpu.r_reg(i)for i in range(8)],(tag,'registers')
  count+=1
 machine.cleanup()
print(json.dumps({'status':'PASS','cases':count,'selectors':[hex(x)for x in selected]}))
