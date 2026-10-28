"""Raw original285DA and every constituent owner, no helper interception."""
import argparse,ctypes as ct,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];BASE=0x1c0000;SIZE=0x4000
class Shadow(ct.Structure):_fields_=[('d',ct.c_uint32*8),('valid',ct.c_uint8)]
Read=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_void_p,ct.c_size_t)
class Context(ct.Structure):_fields_=[('ram',ct.c_void_p),('size',ct.c_size_t),('regs',ct.POINTER(Shadow)),('a',ct.POINTER(ct.c_uint32)),('read',Read),('user',ct.c_void_p),('blocked',ct.c_uint32),('address',ct.c_uint32)]
def put(mem,at,v,n=2):mem[at:at+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def main():
 p=argparse.ArgumentParser();p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');args=p.parse_args()
 rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
 rng=random.Random(0x285da28b46);machine=mus.Machine(mus.CPUType.M68000,2048);cpu,ram=machine.cpu,machine.mem
 @Read
 def read(user,at,dst,n):
  if at>len(rom)or n>len(rom)-at:return 1
  ct.memmove(dst,rom[at:at+n],n);return 0
 counts={};calls=0;guards=0;coverage=set()
 with tempfile.TemporaryDirectory(prefix='wf-front-world-',ignore_cleanup_errors=os.name=='nt')as td:
  lib=Path(td)/'world.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_world.c','-o',str(lib)],cwd=ROOT,check=True)
  native=ct.CDLL(str(lib));native.wf_r18_frontend_world_call.argtypes=[ct.POINTER(Context),ct.c_uint32]
  def invoke(owner,before,d,a,label):
   nonlocal calls
   ram.w_block(0,rom);ram.w_block(BASE,bytes(before));cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
   for i,v in enumerate(d+a):cpu.w_reg(i,v)
   for _ in range(200000):
    pc=cpu.r_pc()
    if pc==0x70000:break
    coverage.add(pc);cpu.execute(1)
   else:raise AssertionError(('ROM runaway',label,hex(owner),hex(pc)))
   raw=ct.create_string_buffer(bytes(before),SIZE);nd=Shadow((ct.c_uint32*8)(*d),1);na=(ct.c_uint32*7)(*a);c=Context(ct.addressof(raw),SIZE,ct.pointer(nd),na,read,None,0,0)
   assert native.wf_r18_frontend_world_call(ct.byref(c),owner)==0,('native blocked',label,hex(owner),hex(c.blocked),hex(c.address))
   want=bytes(ram.r_block(BASE,SIZE));assert raw.raw==want,('RAM16KB',label,hex(owner),[(hex(BASE+i),v,w)for i,(v,w)in enumerate(zip(raw.raw,want))if v!=w][:8])
   wd=[cpu.r_reg(i)for i in range(8)];wa=[cpu.r_reg(i)for i in range(8,15)]
   assert list(nd.d)==wd,('D0-D7',label,hex(owner),list(nd.d),wd)
   assert list(na)==wa,('A0-A6',label,hex(owner),list(na),wa)
   assert bytes(ram.r_block(0,len(rom)))==rom
   counts[hex(owner)]=counts.get(hex(owner),0)+1;calls+=1
   return bytearray(want),wd,wa
  def seed(world):
   raw=bytearray(rng.randbytes(SIZE));put(raw,0x7e,world);raw[0x182a]=raw[0x185a]=255;raw[0x188a:0x194a]=bytes(192);put(raw,0x19aa,0);put(raw,0x19ac,0)
   return raw,[rng.getrandbits(32)for _ in range(8)],[rng.getrandbits(32)for _ in range(7)]
  for world in range(7):
   raw,d,a=seed(world)
   for tick in range(3 if args.smoke else 128):
    # Queue consumer is external: supply its next-frame cleared counts.
    put(raw,0x19aa,0);put(raw,0x19ac,0)
    put(raw,0x17da,(4+(tick//8)%30)*2);put(raw,0x17dc,(4+(tick//16)%9)*2)
    raw,d,a=invoke(0x285da,raw,d,a,('trajectory',world,tick))
   for i in range(1 if args.smoke else 64):
    raw,d,a=seed(world|(0x4000 if i&1 else 0));put(raw,0x17da,rng.randrange(65536));put(raw,0x17dc,rng.randrange(65536))
    invoke(0x28608,raw,d,a,('viewport/wrap',world,i))
   for owner in(0x28808,0x2884e):
    for i in range(1 if args.smoke else 32):
     raw,d,a=seed(world);ids=list(range((12,12,44,0,12,32,44)[world]));rng.shuffle(ids);previous=ids[:len(ids)//2];current=ids[len(ids)//4:3*len(ids)//4]
     raw[0x182a:0x182a+len(current)+1]=bytes(current+[255]);raw[0x185a:0x185a+len(previous)+1]=bytes(previous+[255])
     for j,v in enumerate(previous):raw[0x188a+j*4:0x188e+j*4]=bytes([0xc0|v,rng.randrange(8),rng.randrange(256),rng.randrange(256)])
     invoke(owner,raw,d,a,('listdiff',world,i))
   nids=(36,32,44,12,12,36,44)[world]
   for i in range(2 if args.smoke else nids*8):
    raw,d,a=seed(world);slot=i%48;at=0x188a+slot*4;flags=(0x80,0xc0)[i&1]|(i//8)%nids
    raw[at:at+4]=bytes([flags,(i//2)%8,(0,1,255)[i%3],0xa5])
    # Peer scan can synchronize against inactive records, and includes
    # raw-A0 alias self comparison exactly as CMPA.L, not physical alias.
    for j in range(48):
     if j!=slot:raw[0x188a+j*4+3]=(i+j)%4
    a[0]=(BASE+at)|(0x80000000 if i&4 else 0)
    owner=(0x28b46,0x28b3e,0x288f0,0x28896)[i%4]
    invoke(owner,raw,d,a,('dynamic',world,i))
  # Invalid native domain controls do not replace ROM comparisons.
  raw,d,a=seed(4);buf=ct.create_string_buffer(bytes(raw),SIZE);nd=Shadow((ct.c_uint32*8)(*d),1);na=(ct.c_uint32*7)(*a);c=Context(ct.addressof(buf),SIZE,ct.pointer(nd),na,read,None,0,0)
  assert native.wf_r18_frontend_world_call(None,0x285da)==2;guards+=1
  c.size=SIZE-1;assert native.wf_r18_frontend_world_call(ct.byref(c),0x285da)==2;guards+=1;c.size=SIZE
  assert native.wf_r18_frontend_world_call(ct.byref(c),0x123456)==1 and c.blocked==0x123456;guards+=1
  c.read=Read();assert native.wf_r18_frontend_world_call(ct.byref(c),0x28608)==1 and c.blocked==0x2864a;guards+=1;c.read=read
  na[0]=0x1bffff;assert native.wf_r18_frontend_world_call(ct.byref(c),0x28b46)==1;guards+=1
  # A valid new frame cannot write beyond its48-byte producer queue.
  na[0]=BASE+0x188a;ct.memmove(ct.addressof(buf)+0x188a,b'\x80\0\0\0',4);ct.memmove(ct.addressof(buf)+0x19aa,b'\0\x30\0\x30',4)
  assert native.wf_r18_frontend_world_call(ct.byref(c),0x28b46)==1 and c.blocked==0x28c52;guards+=1
  machine.cleanup();report={'status':'PASS','calls':calls,'owners':counts,'guards':guards,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_world.c').read_bytes()).hexdigest(),'compared':['all RAM16KB including list tail/objects/animationqueues','D0-D7 including D6.B','A0-A6 including raw aliases'],'rom_modifications':[],'helper_intercepts':[],'covered_rom_pcs':[hex(v)for v in sorted(coverage)],'limits':'Literal original world0..6 tables and exact28B3E handler. Invalid allocation/handler dependencies fail closed. No VRAM/IRQ/timing policy.'}
  if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
  print(json.dumps(report))
if __name__=='__main__':main()
