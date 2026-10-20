#!/usr/bin/env python3
"""Compare native CPU decisions with unmodified ROM execution in Musashi."""
import os,ctypes,hashlib,random,re,subprocess,sys,tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1]
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
core.remove('src/wf_cpu_runtime.c')
rng=random.Random(0x1f7f0)
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'cpu.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/cpu_decision_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_cpu_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for case in range(888):
  mem=bytearray(0x200000);mem[:len(rom)]=rom
  def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
  a=0x1c05b0;t=a+0x10c
  w(a+0x20,0x800c);w(a+0xbd,48,1);w(a+0x57,case%12,1)
  w(a+0x2e,128 if case%2 else 0,1);w(a+0x31,case%12,1)
  w(a+0x7a,t,4);w(a+0x26,t,4);w(t+0x7a,a,4);w(t+0x26,a,4)
  w(a+0x66,150);w(t+0x66,100);w(t+0x70,case%3)
  w(0x1c007c,1);w(0x1c16a0,0x10 if case%2 else 0)
  w(0x1c005c,rng.getrandbits(32),4);w(0x1c0080,255+case,4)
  if 72<=case<96:
   w(a+0x20,0x8000);w(a+0x57,2,1);w(a+0x33,0x81,1);w(a+0x34,0x80,1)
   w(a+0xb5,8,1);w(a+0xb0,150);w(a+6,619);w(t+6,769)
   w(t+0x33,0x13,1);w(t+0x2e,0,1);w(t+0x20,0xc002)
  if 96<=case<288:
   w(0x1c007c,1 if case%9==0 else 0);w(0x1c0161,1 if case%13==0 else 0,1)
   w(0x1c0166,1 if case<240 else 0);w(a+0x57,case%12 if case<240 else case%2,1)
   w(a+0x103,(case//12)%6,1);w(a+0x101,0x95,1);w(a+0x34,0x42,1)
   if case%7==0:w(t+0x66,150)
  if 288<=case<336:
   w(a+0x20,5);w(a+0x60,0x16 if case%2 else 0x4015)
   w(a+0xbc,1);w(a+0x86,t,4);w(a+6,550+case%30);w(a+10,320+case%20)
   w(a+0xb5,0,1);w(a+0x33,0x81 if case%3 else 1,1)
   w(0x1c0166,0);w(0x1c007c,1)
  if case>=336:
   w(a+2,case%12);w(a+0x20,0x8002 if case<360 else 0x8005);w(a+0x1c,0x8002 if case<360 else (0x8005 if case%2 else 5))
   w(a+0x60,0x4016);w(a+0x36,(5 if case%2 else 4)*256+1);w(a+0x44,case%3)
   w(a+0x2e,0x8000);w(a+6,620);w(a+10,340);w(a+0x2c,0x40)
   w(a+0x22,case%3);w(a+0x24,case%4);w(a+0x4c,0x9999)
  if case>=384:
   w(a+0x20,6);w(a+0x1c,0x8006 if case%2 else 6);w(a+0x24,0xfe if case%3==0 else case%2)
  if case>=408:
   w(a+0x20,5);w(a+0x1c,0x8005 if case%2 else 5);w(a+0x60,0x8006 if case%3 else 6);w(a+0x34,8 if case%4==0 else 0,1);w(a+0x92,case%3,4)
  if case>=432:
   w(a,0x8000);w(a+0x20,5);w(a+0x1c,0x8005 if case%2 else 5);w(a+0x60,0x8066 if case%3 else 0x66);w(a+0x86,t,4);w(a+0x56,case%12);w(t+0x33,0,1);w(a+0x33,0x83 if case%4 else 3,1);w(t+0x72,150);w(t+0x66,case%3*50+10);w(a+0x8a,0x140020,4);w(t+0x8a,0x140022,4)
  if case>=456:
   w(a+0x20,0x8000);w(a+0xb5,0,1);w(a+0xb7,12,1);w(a+0x33,0x81,1);w(a+0x34,0x80,1);w(a+0x86,t,4);w(t+0x33,3,1);w(t+0x20,0x8006);w(a+0x7a,t,4);w(a+0x66,100+case%3*30);w(t+0x66,130);w(a+0x56,0x8002)
  if case>=480:
   w(a+0x20,0x8001);w(a+0xb7,0,1);w(a+0x33,0x81,1);w(a+0xb5,0,1);w(t+0x33,0,1);w(t+0x20,0x8005);w(t+0x60,0x15)
  if case>=504:
   w(a+0x20,5);w(a+0x1c,0x8005 if case%2 else 5);w(a+0x60,0x4d);w(a+0x34,8 if case%3 else 0,1);w(a+0x24,case%5 if case%4 else 0xfe);w(a+0x22,0 if case%3 else 4);w(a+0x33,0x81 if case%3 else 1,1)
  if case>=504:w(0x1c17e6,480);w(0x1c17ee,560)
  if case>=528:
   w(a+0x20,8);w(a+0x1c,0x8008 if (case//4)%2 else 8);w(a+0x44,4+case%4);w(a+0x24,0xfe if case%3 else case%5)
  if case>=552:
   w(a+0x20,0x8001);w(a+0x1c,0x8001);w(a+0xb7,0,1);w(a+0x33,0x81,1);w(a+0xb5,0,1);w(t+0x33,0x13,1);w(t+0x20,0xc002);w(t+0x2e,0,1);w(a+6,619);w(t+6,769)
  if case>=576:
   w(a+0x20,9);w(a+0x1c,0x8009 if case%2 else 9);w(a+0x44,case%10);w(a+0x46,case%4);w(a+0xfe,0x8000 if case%3 else 0)
  if case>=600:
   w(a+0x20,4);w(a+0x1c,0x8004 if (case//4)%2 else 4);w(a+0x64,14);w(a+0x92,t,4);w(a+0x24,case%4);w(a+0x22,0 if case%3 else 3);w(a+0x36,[0,1,3,5,6][case%5]*256+(0x10 if case%3 else 2));w(t+6,600+case%100);w(t+10,270+case%150);w(t+0x2e,0x8000 if case%2 else 0);w(a+0x33,4 if case%7 else 0,1)
  if case>=696:
   w(a+0x64,5);w(a+0x1c,0x8004 if case%2 else 4);w(a+0x72,150);w(a+0x66,case%5*40);w(a+0x70,case%3);w(a+0xaa,[0,1,5,0x4000][case%4]);w(a+0xa3,case%3,1);w(a+0x56,0x8002 if case%3 else 2);w(a+0x36,0x510 if case%4 else 0x501)
  if case>=792:
   j=case-792;route=j%3
   w(a+0x20,5);w(a+0x1c,0x8005 if (j//3)%2 else 5);w(a+0x60,13);w(a+0x24,[0,3,4,0xfe][(j//6)%4]);w(a+0x22,0);w(a+0x44,route);w(a+0x34,8 if route==0 else 0,1);w(t+0x20,5 if route==1 else 0);w(t+0x60,0x53);w(t+0x32,0,1);w(t+6,620);w(t+10,340);w(a+0x2e,0x8000 if (j//48)%2 else 0);w(a+0x36,0x510 if (j//24)%2 else 0x500);w(0x1c1670,0x12345678,4)
  regs=[rng.getrandbits(32) for _ in range(8)]
  nr=(ctypes.c_uint32*9)(*regs,0);nb=ctypes.create_string_buffer(bytes(mem))
  if case<int(os.environ.get("WF_ORACLE_START","0")):continue
  print("case",case,flush=True)
  status=native.wf_cpu_oracle(nb,nr,257 if case>=576 else 0 if case>=552 else 257 if case>=504 else 0 if case>=456 else 257 if case>=336 else 256 if 96<=case<288 else 0)
  assert status==0,('native CPU blocked',case,hex(-status))
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;memory=machine.mem
  memory.w_block(0,bytes(mem))
  for i,v in enumerate(regs):u.w_reg(i,v)
  u.w_reg(8,a);u.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000)
  u.w_pc(0x1c03e if case>=576 else 0x1c8f4 if case>=552 else 0x1c03e if case>=504 else 0x1c8f4 if case>=480 else 0x1c2cc if case>=456 else 0x1c03e if case>=336 else 0x1dfe2 if case>=288 else 0x1f7f0 if case>=96 else 0x1f06a if case<72 else 0x1c2cc)
  for steps in range(100000):
   if u.r_pc()==0x70000:break
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',case,hex(u.r_pc()))
  want=bytes(memory.r_block(a,9*0x10c));got=nb.raw[a:a+9*0x10c]
  assert got==want,('actors',case,[(hex(i),g,e) for i,(g,e) in enumerate(zip(got,want)) if g!=e][:12])
  assert nb.raw[0x1c005c:0x1c0060]==bytes(memory.r_block(0x1c005c,4)),('RNG',case)
  if case>=432:
   for at,n in ((0x180000,0x1000),(0x1c1684,8)):
    assert nb.raw[at:at+n]==bytes(memory.r_block(at,n)),('tag writes',case,hex(at))
  if case>=504:
   for at,n in ((0x1c1258,12*0x2a),(0x1c1697,1),(0x1c15d2,2),(0x1c1802,2),(0x1c1670,4),(0x1c167a,1),(0x1c02b8,0x2f8)):
    assert nb.raw[at:at+n]==bytes(memory.r_block(at,n)),('effect/queue',case,hex(at),[(hex(i),g,e) for i,(g,e) in enumerate(zip(nb.raw[at:at+n],bytes(memory.r_block(at,n)))) if g!=e][:10])
  assert list(nr)[:8]==[u.r_reg(i) for i in range(8)],('registers',case,list(nr),[u.r_reg(i) for i in range(8)])
  if case>=336:
   for at in (0x1c1150,0x1c1180,0x1c11b0,0x1c11e0):assert nb.raw[at:at+2]==bytes(memory.r_block(at,2)),('rope global',case,hex(at))
  if 96<=case<288:assert nr[8]==(u.r_sr()&1),('carry',case,nr[8],u.r_sr())
  machine.cleanup()
 print('PASS',888-int(os.environ.get('WF_ORACLE_START','0')),'ROM CPU/action cases: actors, globals, effects/queues, RNG, D0-D7 and arbitration carry')
