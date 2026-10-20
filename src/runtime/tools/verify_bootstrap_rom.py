import sys,subprocess,struct,hashlib,argparse
import machine68k as mus
parser=argparse.ArgumentParser();parser.add_argument('--rom',default='work/recovery/main68k.bin');parser.add_argument('--driver',default='work/runtime18/bootstrap/driver');parser.add_argument('--credit',type=int,default=0);parser.add_argument('--remainder',type=int,default=0);args=parser.parse_args()
rom=open(args.rom,'rb').read()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
for seed in [0,1,0x13579bdf,0xffffffff]:
 for stage in range(10):
  m=mus.Machine(mus.CPUType.M68000,2048);u=m.cpu;mem=m.mem;mem.w_block(0,rom)
  def w(p,v,n=2):mem.w_block(p,v.to_bytes(n,'big'))
  w(0x1c005c,seed,4);w(0x1c0162,stage);w(0x1c0066,0x300);w(0x1c007c,1);w(0x1c004f,args.credit,1);w(0x1c004e,args.remainder,1)
  for i in range(8):u.w_reg(i,0x11110000+i*0x1111)
  for i in range(12):w(0x1c0598+i*2,65535)
  w(0x1c0598,0);w(0x1c059a,1)
  for i in range(2):
   a=0x1c05b0+i*0x10c;w(a,0x8000);w(a+0x56,i);w(a+0xa0,1)
  writes=[]
  mem.set_trace_mode(True)
  mem.set_trace_func(lambda op,sz,a,v: writes.append((a,v,1<<sz)) if op=='W' and (0xc0000<=a<0xc4000 or 0x180000<=a<0x181000) else None)
  u.w_sp(0x1d0000);w(0x1d0000,0x70000,4);u.w_pc(0x1086c)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   if u.r_pc()==0x2aea:
    sp=u.r_sp();u.w_pc(mem.r32(sp));u.w_sp(sp+4)
   else:u.execute(1)
  assert u.r_pc()==0x70000,hex(u.r_pc())
  expected=bytes(mem.r_block(0x1c05b0,9*0x10c))+bytes(mem.r_block(0x1c11f4,0x64))
  expected+=struct.pack('<8I',*[u.r_reg(i) for i in range(8)])+struct.pack('<I',int.from_bytes(mem.r_block(0x1c005c,4),'big'))
  expected+=b''.join(bytes(mem.r_block(0x1c0598+i*2,2))[::-1] for i in range(12))+b''.join(bytes(mem.r_block(0x1c0380+i*2,2))[::-1] for i in range(12))
  expected+=bytes(mem.r_block(0x1c0063,1))+struct.pack('<HH',mem.r16(0x1c0072),mem.r16(0x1c169e))
  expected+=struct.pack('<I',len(writes))+b''.join(struct.pack('<3I',*v) for v in writes)
  m.cleanup()
  got=subprocess.check_output([args.driver,str(seed),str(stage)])
  diff=[(i,a,b) for i,(a,b) in enumerate(zip(expected,got)) if a!=b]
  assert len(expected)==len(got),(len(expected),len(got))
  if diff:print('FAIL',hex(seed),stage,'len',len(expected),len(got),'diff',diff[:20]);sys.exit(1)
print('PASS 40 TAG setups: all actor/referee bytes, D0-D7, RNG, history and roster match canonical ROM; palette transport intercepted only.')
