#!/usr/bin/env python3
"""Board payloads against unmodified ROM, full RAM/D/A and ordered bus effects."""
import argparse,ctypes as ct,hashlib,json,os,random,subprocess,tempfile
from pathlib import Path
import machine68k as mus
ROOT=Path(__file__).resolve().parents[2];BASE=0x1c0000;SIZE=0x4000
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
class Shadow(ct.Structure):_fields_=[('d',ct.c_uint32*8),('valid',ct.c_uint8)]
ReadROM=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_void_p,ct.c_size_t)
Store=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.c_uint32,ct.c_uint)
ReadBus=ct.CFUNCTYPE(ct.c_int,ct.c_void_p,ct.c_uint32,ct.POINTER(ct.c_uint32),ct.c_uint)
class Context(ct.Structure):
 _fields_=[('ram',ct.c_void_p),('size',ct.c_size_t),('regs',ct.POINTER(Shadow)),('a',ct.POINTER(ct.c_uint32)),('rom',ReadROM),('store',Store),('read',ReadBus),('user',ct.c_void_p),('blocked',ct.c_uint32),('address',ct.c_uint32)]
def put(mem,at,value,n=2):mem[at:at+n]=(value&((1<<(8*n))-1)).to_bytes(n,'big')
def get(mem,at,n=2):return int.from_bytes(mem[at:at+n],'big')
def digest(raw):return hashlib.sha256(raw).hexdigest()
def fixtures(rng,smoke):
 for config in ((0,0xff,0x40,0x41)if smoke else range(256)):
  yield 0x2006,('config',config),{0x67:bytes([config])}
 for flags in ((0x80,0xc0,0xc4)if smoke else range(256)):
  for timer in ((95,)if smoke else(0,94,95,96,255)):
   for enabled in ((0x10,)if smoke else(0,0x10)):
    yield 0xb53a,('flags/timer/gate',flags,timer,enabled),{0xf1c:bytes([timer,flags]),0x6f:bytes([enabled])}
 for owner in(0x27012,0x27066):
  for counter in((0,1,14,0xffff)if smoke else(0,1,2,14,15,16,0x7fff,0x8000,0x8001,0xffff,*[rng.randrange(65536)for _ in range(128)])):
   yield owner,('counter',counter),{0x1800 if owner==0x27012 else 0x1802:counter.to_bytes(2,'big')}
 for flags in((0,0x20)if smoke else range(256)):
  for flash in((0,1)if smoke else(0,1,0x80,0xff)):
   yield 0x264aa,('flash',flags,flash),{0x169e:bytes([flags]),0x16a5:bytes([flash])}
 for world in(range(1)if smoke else range(7)):
  for first,second in(((1,1),(0,0))if smoke else((0,0),(1,0),(0,1),(1,1),(2,3),(47,48),(48,47))):
   for alias in((0,)if smoke else(0,0x2000,0x4000,0xe000)):
    yield 0x2946a,('queues',world|alias,first,second),{0x7e:(world|alias).to_bytes(2,'big'),0x19ac:first.to_bytes(2,'big'),0x19aa:second.to_bytes(2,'big')}
 for i in range(2 if smoke else 32):yield 0x2988,('palette reset',i),{}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path);p.add_argument('--smoke',action='store_true');args=p.parse_args()
 rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()==SHA1
 rng=random.Random(0xb53a2946a);counts={};total=0;guards=0;writes_total=0;reads_total=0
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu=machine.cpu;ram=machine.mem
 current={};errors=[]
 @ReadROM
 def read_rom(user,at,dst,n):
  if at>len(rom) or n>len(rom)-at:return 1
  ct.memmove(dst,rom[at:at+n],n);return 0
 @Store
 def store(user,at,value,width):
  try:
   current['trace'].append(('W',at,value,width,digest(current['raw'].raw)))
   put(current['mem'],at,value,width);return 0
  except BaseException as e:errors.append(repr(e));return 1
 @ReadBus
 def read_bus(user,at,value,width):
  try:
   value[0]=get(current['mem'],at,width)
   current['trace'].append(('R',at,value[0],width,digest(current['raw'].raw)));return 0
  except BaseException as e:errors.append(repr(e));return 1
 with tempfile.TemporaryDirectory(prefix='wf-frontend-board-',ignore_cleanup_errors=os.name=='nt')as td:
  lib=Path(td)/'board.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_frontend_board.c','-o',str(lib)],cwd=ROOT,check=True)
  native=ct.CDLL(str(lib));native.wf_r18_frontend_board_call.argtypes=[ct.POINTER(Context),ct.c_uint32]
  for owner,label,changes in fixtures(rng,args.smoke):
   mem=bytearray(0x200000);mem[:len(rom)]=rom
   for at,n in((BASE,SIZE),(0x80000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40)):mem[at:at+n]=rng.randbytes(n)
   for off,b in changes.items():mem[BASE+off:BASE+off+len(b)]=b
   if owner==0x2988:mem[0x140026]|=4
   d=[rng.getrandbits(32)for _ in range(8)];a=[rng.getrandbits(32)for _ in range(7)]
   ram.w_block(0,bytes(mem));cpu.w_sr(0x2700);cpu.w_sp(0x1d0000);ram.w32(0x1d0000,0x70000);cpu.w_pc(owner)
   for i,v in enumerate(d+a):cpu.w_reg(i,v)
   trace=[];polls=[0,0]
   for step in range(150000):
    pc=cpu.r_pc()
    if pc==0x70000:break
    if pc==0x298c:polls[0]+=1
    if pc==0x29f6:
     polls[1]+=1;ram.w8(0x140026,ram.r8(0x140026)&~4)
    access=None
    if pc in(0x2006,0x200e,0x2016,0x201e,0x2026,0x202e):access=((0x100008,0x10000c,0x140006,0x140010,0x140012,0x140014)[(0x2006,0x200e,0x2016,0x201e,0x2026,0x202e).index(pc)],2)
    elif pc in(0x2036,0x2048):
     at=0x10000b;trace.append(('R',at,ram.r8(at),1,digest(ram.r_block(BASE,SIZE))));access=(at,1)
    elif pc==0xb5aa:access=((cpu.r_reg(9)+(cpu.r_reg(1)&0xffff))&0xffffff,2)
    elif 0x294b4<=pc<=0x2950e or 0x29564<=pc<=0x2958e:
     offset=ram.r16(pc+4);access=((cpu.r_reg(11)+offset)&0xffffff,4)
    elif pc in(0x264be,0x264d0):access=(0x18071c,2)
    elif pc in(0x264c6,0x264d8):access=(0x18071e,2)
    elif pc in(0x29c6,0x29c8,0x29ca,0x29cc):access=(cpu.r_reg(8+(pc-0x29c6)//2)&0xffffff,4)
    elif pc==0x29ee:access=(0x184000,2)
    before=digest(ram.r_block(BASE,SIZE))if access else None
    cpu.execute(1)
    if access:
     at,width=access;value={1:ram.r8,2:ram.r16,4:ram.r32}[width](at);trace.append(('W',at,value,width,before))
   else:raise AssertionError(('ROM runaway',hex(owner),hex(pc),label))
   raw=ct.create_string_buffer(bytes(mem[BASE:BASE+SIZE]),SIZE);nd=Shadow((ct.c_uint32*8)(*d),1);na=(ct.c_uint32*7)(*a)
   ctx=Context(ct.addressof(raw),SIZE,ct.pointer(nd),na,read_rom,store,read_bus,None,0,0)
   current.update(mem=mem,raw=raw,trace=[]);errors.clear()
   status=native.wf_r18_frontend_board_call(ct.byref(ctx),owner)
   assert status==0,('native blocked',hex(owner),label,status,hex(ctx.blocked),hex(ctx.address),errors)
   assert not errors,errors
   assert current['trace']==trace,('ordered bus accesses or pre-write RAM',hex(owner),label,len(current['trace']),len(trace),next(((i,g,w)for i,(g,w)in enumerate(zip(current['trace'],trace))if g!=w),None))
   assert raw.raw==bytes(ram.r_block(BASE,SIZE)),('RAM16KB',hex(owner),label,[(hex(BASE+i),v,ram.r8(BASE+i))for i,v in enumerate(raw.raw)if v!=ram.r8(BASE+i)][:8])
   assert list(nd.d)==[cpu.r_reg(i)for i in range(8)],('D0-D7',hex(owner),label,list(nd.d),[cpu.r_reg(i)for i in range(8)])
   assert list(na)==[cpu.r_reg(i)for i in range(8,15)],('A0-A6',hex(owner),label,list(na),[cpu.r_reg(i)for i in range(8,15)])
   for at,n in((0x80000,0x4000),(0x180000,0x9000),(0x100000,0x20),(0x140000,0x40)):
    # The driver supplies LOW after payload; polling is deliberately external.
    if owner==0x2988 and at==0x140000:mem[0x140026]&=~4
    assert mem[at:at+n]==bytes(ram.r_block(at,n)),('video/MMIO',hex(owner),label,hex(at))
   if owner==0x2988:assert polls==[1,1] and len(trace)==513
   assert bytes(ram.r_block(0,len(rom)))==rom
   total+=1;counts[hex(owner)]=counts.get(hex(owner),0)+1
   writes_total+=sum(e[0]=='W'for e in trace);reads_total+=sum(e[0]=='R'for e in trace)
  # Native guards are separate from valid ROM domain, never oracle bypasses.
  before=raw.raw;original=list(nd.d),list(na)
  for bad in(None,):assert native.wf_r18_frontend_board_call(bad,0x2006)==2;guards+=1
  ctx.size=SIZE-1;assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2006)==2;guards+=1;ctx.size=SIZE
  nd.valid=0;assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2006)==2;guards+=1;nd.valid=1
  assert raw.raw==before and(list(nd.d),list(na))==original
  assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x123456)!=0 and ctx.blocked==0x123456;guards+=1
  # Each producer queue occupies48 bytes. Oversized counts must not emit.
  for off in(0x19ac,0x19aa):
   ct.memset(ct.addressof(raw)+0x19aa,0,4);ct.memmove(ct.addressof(raw)+off,b'\0\x31',2);current['trace'].clear();before=raw.raw
   assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2946a)!=0 and not current['trace'] and raw.raw==before;guards+=1
  # Missing dependencies block exactly when needed. No-op gates do not
  # require fake ROM/bus providers. Real dependencies are never ignored.
  ctx.store=Store();current['trace'].clear();before=raw.raw
  assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2006)==1
  assert(ctx.blocked,ctx.address)==(0x2006,0x100008)and not current['trace']and raw.raw==before;guards+=1
  ctx.store=store;ctx.read=ReadBus();current['trace'].clear()
  assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2006)==1
  assert(ctx.blocked,ctx.address)==(0x2036,0x10000b)and len(current['trace'])==6 and raw.raw==before;guards+=1
  ctx.read=read_bus;ctx.rom=ReadROM();ct.memmove(ct.addressof(raw)+0x1800,b'\0\1',2);before=raw.raw
  assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x27012)==1
  assert(ctx.blocked,ctx.address)==(0x2702c,0x2704a)and raw.raw==before;guards+=1
  ctx.rom=read_rom
  # Read-modify-write failure must not apply a made-up byte or continue.
  @ReadBus
  def reject_read(*unused):return 1
  ctx.read=reject_read;current['trace'].clear();before=raw.raw
  assert native.wf_r18_frontend_board_call(ct.byref(ctx),0x2006)==1
  assert(ctx.blocked,ctx.address)==(0x2036,0x10000b)and len(current['trace'])==6 and raw.raw==before;guards+=1
  ctx.read=read_bus
  # Store failure at either the first access or later payload accesses
  # aborts immediately; earlier successful bus effects remain committed.
  for owner,fail_at in((0x2006,0),(0x2006,5),(0x2988,0),(0x2988,3),(0x2988,511),(0x2988,512)):
   attempts=[]
   @Store
   def fail_store(user,at,value,width):
    attempts.append((at,value,width))
    if len(attempts)-1==fail_at:return 1
    return store(user,at,value,width)
   ctx.store=fail_store;current['trace'].clear()
   assert native.wf_r18_frontend_board_call(ct.byref(ctx),owner)==1
   assert len(attempts)==fail_at+1 and len(current['trace'])==fail_at
   assert ctx.blocked and ctx.address==attempts[-1][0];guards+=1
  ctx.store=store
  # Gated no-op owners work without irrelevant external callbacks.
  ctx.rom=ReadROM();ctx.store=Store();ctx.read=ReadBus()
  for off in(0x1800,0x1802,0x19aa,0x19ac):ct.memset(ct.addressof(raw)+off,0,2)
  ct.memset(ct.addressof(raw)+0x6f,0,1);ct.memset(ct.addressof(raw)+0x169e,0,1)
  before=raw.raw;original=list(nd.d),list(na)
  for owner in(0xb53a,0x27012,0x27066,0x2946a,0x264aa):
   assert native.wf_r18_frontend_board_call(ct.byref(ctx),owner)==0
   assert raw.raw==before and(list(nd.d),list(na))==original;guards+=1
  machine.cleanup()
  report={'status':'PASS','calls':total,'owners':counts,'native_guards':guards,'ordered_bus_writes':writes_total,'ordered_bus_reads':reads_total,'compared':['all RAM16KB','BG16KB','paletteRAM36KB including96byte padding','MMIO bytes','D0-D7/A0-A6','ordered bus access width/address/value and pre-write RAM digest'],'rom_sha1':SHA1,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_frontend_board.c').read_bytes()).hexdigest(),'rom_modifications':[],'helper_intercepts':[],'limits':'2988 HIGH and LOW supplied explicitly by test driver; native2988 payload does not poll. CCR/stack not modeled by payload API. Producer queues max48 bytes each.'}
  if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
  print(json.dumps(report))
if __name__=='__main__':main()
