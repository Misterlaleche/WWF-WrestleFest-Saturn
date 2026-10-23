#!/usr/bin/env python3
"""Canonical scroll ownership and limits against untouched $26936/$2983C."""
import ctypes, hashlib, random, re, subprocess, sys, tempfile
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[1];rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M).group(1).split()
rng=random.Random(0x269362983c);total=0
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'camera.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/camera_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.wf_camera_oracle.argtypes=[ctypes.c_uint,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint16)]
 for op in range(3):
  for case in range(768):
   mem=bytearray(0x200000);mem[:len(rom)]=rom
   def w(at,v,n=2):mem[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
   scene=case%7;w(0x1c007e,scene);w(0x1c0161,(case//7)%4,1)
   edges=[0,319,320,321,511,512,513,639,640,641,703,704,705,0x7fff,0x8000,0xffff]
   for j,off in enumerate((0x17e6,0x17ea,0x17ee,0x17f2)):
    w(0x1c0000+off,edges[(case+j*3)%len(edges)] if case<256 else rng.randrange(65536))
   for off in (0x1804,0x1806):w(0x1c0000+off,rng.randrange(65536))
   for i in range(9):
    a=0x1c05b0+i*0x10c;w(a,0x8000 if case<16 or rng.randrange(3) else 0)
    w(a+0x4b,(case+i)%6 if case<256 else rng.randrange(6),1);w(a+0x33,rng.choice([0,1,4,5,255]),1)
    for off in (6,10,14):w(a+off,rng.randrange(65536))
   regs=[rng.getrandbits(32) for _ in range(8)];nr=(ctypes.c_uint32*8)(*regs);nb=ctypes.create_string_buffer(bytes(mem));cache=(ctypes.c_uint16*4)()
   assert native.wf_camera_oracle(op,nb,nr,cache)==0
   machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
   for i in range(8,15):u.w_reg(i,0)
   for i,v in enumerate(regs):u.w_reg(i,v)
   for pc in ([0x26936] if op==0 else [0x2983c] if op==1 else [0x26936,0x2983c]):
    u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(pc)
    for steps in range(10000):
     if u.r_pc()==0x70000:break
     u.execute(1)
    assert u.r_pc()==0x70000,('ROM runaway',op,case,hex(u.r_pc()))
   offsets=(0x17e6,0x17ea,0x17ee,0x17f2)
   want=[m.r16(0x1c0000+x) for x in offsets];got=[int.from_bytes(nb.raw[0x1c0000+x:0x1c0002+x],'big') for x in offsets]
   assert got==want,('canonical scrolls',op,case,got,want)
   assert list(cache)==want,('adapter coherence',op,case,list(cache),want)
   assert list(nr)==[u.r_reg(i) for i in range(8)],('D0-D7',op,case,list(nr),[u.r_reg(i) for i in range(8)])
   assert nb.raw[0x1c05b0:0x1c0f1c]==bytes(m.r_block(0x1c05b0,9*0x10c)),('actors mutated',op,case)
   machine.cleanup();total+=1
 print('PASS',total,'camera ROM cases: track/clamp/composed, stale adapter, 7 scenes, all classes, unsigned paired limits, actors and D0-D7')
