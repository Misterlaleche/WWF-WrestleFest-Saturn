#!/usr/bin/env python3
"""Original IRQ $087A-$0886 without hooks, differential RAM/register writes."""
import ctypes,random,subprocess,tempfile,hashlib
from pathlib import Path
import machine68k as mus
root=Path(__file__).resolve().parents[2];rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
rng=random.Random(0x2701227066)
with tempfile.TemporaryDirectory() as tmp:
 lib=Path(tmp)/'display.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-Iinclude','src/wf_r18_display.c','src/wf_r18_hud.c','src/wf_r18_video.c','src/wf_saturn_memory.c','tests/r18/display_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));source=ctypes.create_string_buffer(rom);native.display_case.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint16)]
 for case in range(800):
  enable=[0x80,0,0xff][case%3];flags=[1,0,0x81,0x11,3][case//3%5]
  cy=[0,1,14,0x8000,0xffff,rng.randrange(15)][case//15%6];cx=rng.randrange(15)
  vals=[enable,flags,cy,cx]+[rng.randrange(65536) for _ in range(4)]
  q=mus.Machine(mus.CPUType.M68000,2048);u=q.cpu;m=q.mem;m.w_block(0,rom)
  for at,v in zip([0x1c1800,0x1c1802,0x1c17e6,0x1c17ee,0x1c17ea,0x1c17f2],vals[2:]):m.w16(at,v)
  m.w8(0x1c006f,flags)
  for i in range(4):m.w16(0x100000+i*2,0x1234)
  if enable&128 and flags&1 and not flags&128:
   for pc in [0x27012,0x27066,0x26e1a]:
    u.w_sp(0x1f0000);m.w32(0x1f0000,0x70000);u.w_pc(pc)
    for step in range(1000):
     if u.r_pc()==0x70000:break
     u.execute(1)
    else:raise AssertionError(hex(u.r_pc()))
  expected=[m.r16(at) for at in [0x1c1800,0x1c1802,0x1c17e6,0x1c17ee,0x1c17ea,0x1c17f2,0x100000,0x100002,0x100004,0x100006]]
  out=(ctypes.c_uint16*10)();status=native.display_case(source,(ctypes.c_uint16*8)(*vals),out)
  assert status==0 and list(out)==expected,(case,vals,status,list(out),expected)
  q.cleanup()
 print('PASS 800 original-ROM display IRQ cases: signed indices, wrap, countdowns, gates and register inhibit')

 # $264AA uses no calls or register changes: compare all palette bytes.
 for case in range(256):
  vals=[rng.choice([0,128,255]),rng.choice([0,1,3,17,129]),rng.randrange(65536),rng.randrange(65536)]
  q=mus.Machine(mus.CPUType.M68000,2048);u=q.cpu;m=q.mem;m.w_block(0,rom);m.w_block(0x180700,b'\xa5'*32)
  m.w16(0x1c169e,vals[2]);m.w16(0x1c16a4,vals[3])
  if vals[0]&128 and vals[1]&1 and not vals[1]&128:
   u.w_sp(0x1f0000);m.w32(0x1f0000,0x70000);u.w_pc(0x264aa)
   for step in range(1000):
    if u.r_pc()==0x70000:break
    u.execute(1)
   else:raise AssertionError(hex(u.r_pc()))
  out=(ctypes.c_uint8*32)();assert native.hud_irq_case((ctypes.c_uint16*4)(*vals),out)==0
  assert bytes(out)==bytes(m.r_block(0x180700,32)),(case,vals)
  q.cleanup()
 print('PASS 256 original-ROM HUD IRQ $264AA clock-palette cases')
