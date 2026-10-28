"""AE20(0) presenter program and callbacks versus full intact original ROM.

Reuse the audited post-tag machine/checkpoint oracle on a private frozen
source/header/ROM copy, with a new fixture set and explicit original waits.
"""
from pathlib import Path
import hashlib,os,runpy,shutil,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
body=(HERE/'post_tag_verify_rom.py').read_text()
body=body.replace('0x2988}','0x2988,0xb29e,0xb2e4}')
body=body.replace('WAITS={','WAITS={0xaf40:0xaf4a,0xb068:0xb072,')
if "('callback_a0',ctypes.c_uint32)"not in body:body=body.replace("('carry',ctypes.c_uint8)]","('carry',ctypes.c_uint8),('callback_a0',ctypes.c_uint32)]")
start=body.index('def fixtures():');end=body.index("source=ROOT/'src/wf_r18_post_tag.c'",start)
body=body[:start]+'''def fixtures():
 mem=seed();put(mem,0x1c008c,1);put(mem,0x1c1278,0x5a5a);yield 'AE20-early-return-0-1',0xae20,mem,0,(0xb0c0,),False
 mem=seed(4,15);yield 'AE20-program0-original-full',0xae20,mem,0,(0xb0c0,),True
 if args.smoke:return
 for arg,gate in itertools.product((0,1,2,0xffff),(1,0x8000,0xffff)):
  if arg==0 and gate==1:continue
  mem=seed();put(mem,0x1c008c,gate);put(mem,0x1c1278,0x5a5a);yield f'AE20-early-return-{arg}-{gate}',0xae20,mem,arg,(0xb0c0,),False
 for high,gate in itertools.product((0,0x80000000,0xffff0000),(0,1,0x8000,0xffff)):
  mem=seed();put(mem,0x1c008c,gate);yield f'AE20-arg3-high-{high}-{gate}',0xae20,mem,high|3,(0xb0c0,),False
 for mode,progress,flags in((0,9,0),(0,0x104,0x8000),(1,4,0),(2,9,0x8000)):
  mem=seed(progress,15);put(mem,0x1c0161,mode,1);put(mem,0x1c0066,flags);yield f'AE20-program0-{mode}-{progress}-{flags}',0xae20,mem,0,(0xb0c0,),True
 for role,flag,timer,phase in itertools.product(range(3),(0,1,0x80,0x81),(0,1,0x7fff,0x8000,0xffff),range(62)):
  if phase>=(29,62,32)[role]:continue
  mem=seed();a=0x1c1258+role*0x2a;put(mem,a+0x1d,flag,1);put(mem,a+0x22,timer);put(mem,a+0x24,phase);put(mem,0x1c1276,0)
  yield f'B0C8-callback-{role}-{flag}-{timer}-{phase}',0xb0c8,mem,role,(0xb148,),False
 yield 'B152-original-teardown',0xb152,seed(),None,(0xb19a,),False
''' +body[end:]
body=body.replace("if owner==0xbf5e:regs[8]=0x1c1258+d0*0x2a","if owner in(0xbf5e,0xb0c8):regs[8]=0x1c1258+d0*0x2a")
body=body.replace("if owner in(0x139c,0x160e):", "if name.startswith('AE20-arg3-high-'):regs[0]=d0\n  if owner in(0x139c,0x160e):")
body=body.replace("'AE20 argument3 only; other programs remain explicit dependency'","'AE20 arguments0/3; programs1/2 remain explicit dependencies; no forced progression or visibility'")
body=body.replace("'This regression suite covers AE20 argument3; argument0 is verified separately; programs1/2 remain guarded'","'AE20 arguments0/3; programs1/2 remain explicit dependencies; no forced progression or visibility'")
extra='''
 @Rom
 def denied_reader(_,at,dest,n):
  if at==denied_at:return 1
  return read_rom(_,at,dest,n)
 prefix_checks=0
 for denied_at,pc,phase in((0xb19c,0xb0e6,0),(0xb1a8,0xb0f0,0),(0xb1a9,0xb12a,0),(0xb14a,0xb10e,1)):
  raw=seed();a=0x1c1258;put(raw,a+0x1d,1,1);put(raw,a+0x22,0);put(raw,a+0x24,phase)
  regs=[0x81a50000+i for i in range(15)];regs[0]=0x81a50000;regs[8]=a
  want=bytearray(raw);wr,_,_=execute(want,regs,0xb0c8,(pc,))
  buf=ctypes.create_string_buffer(bytes(raw));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);m=Machine(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),None,None,0);c=Context(ctypes.pointer(m),denied_reader,None);s=State();native.wf_r18_post_tag_init(ctypes.byref(s),0xb0c8)
  for step in range(30):
   status=native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))
   if status<0:break
  assert status==-1 and s.blocked_dependency==pc,(hex(pc),status,hex(s.pc))
  assert list(sr.d)+list(m.a)==wr,('guard prefix registers',hex(pc))
  assert buf.raw[0x1c0000:0x1c4000]==want[0x1c0000:0x1c4000],('guard prefix RAM',hex(pc));controls+=1;prefix_checks+=1
 for bad,flag,pc in((0,0,0xb0c8),(0xdeadbeef,1,0xb0c8),(0x1c1259,1,0xb0d2)):
  raw=seed();a=bad&0xffffff
  if 0x1c0000<=a<0x1c3f00:put(raw,a+0x1d,flag,1)
  buf=ctypes.create_string_buffer(bytes(raw));sr=Shadow((ctypes.c_uint32*8)(*range(8)),1);m=Machine(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(bad,0,0,0,0,0,0),None,None,0);c=Context(ctypes.pointer(m),read_rom,None);s=State();native.wf_r18_post_tag_init(ctypes.byref(s),0xb0c8)
  for step in range(10):
   status=native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))
   if status<0:break
  assert status==-1 and s.blocked_dependency==pc;assert buf.raw[:0x200000]==bytes(raw);assert list(sr.d)==list(range(8));controls+=1
 c.read_rom=read_rom
 for arg in(1,2,4,0xffff):
  raw=seed();put(raw,0x1c008c,0);put(raw,0x1c1278,0x5a5a);regs=[0x81a50000+i for i in range(15)];regs[0]=0x81a50000|arg
  want=bytearray(raw);wr,_,_=execute(want,regs,0xae20,(0xae30,))
  buf=ctypes.create_string_buffer(bytes(raw));sr=Shadow((ctypes.c_uint32*8)(*regs[:8]),1);m=Machine(ctypes.cast(ctypes.addressof(buf)+0x1c0000,ctypes.POINTER(ctypes.c_uint8)),ctypes.pointer(sr),(ctypes.c_uint32*7)(*regs[8:]),None,None,0);c=Context(ctypes.pointer(m),read_rom,None);s=State();native.wf_r18_post_tag_init(ctypes.byref(s),0xae20)
  for step in range(5):
   status=native.wf_r18_post_tag_step(ctypes.byref(s),ctypes.byref(c))
   if status<0:break
  assert status==-1 and s.pc==0xae30 and s.blocked_dependency==0xae20
  assert list(sr.d)+list(m.a)==wr and buf.raw[0x1c0000:0x1c4000]==want[0x1c0000:0x1c4000];controls+=1;prefix_checks+=1
'''
body=body.replace(' assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()',extra+' assert bytes(machine.mem.r_block(0,len(rom)))==rom;machine.cleanup()')
body=body.replace("'waits':waits,","'waits':waits,'guard_prefix_checks':prefix_checks,'coverage_sites':[hex(pc)for pc in sorted(coverage)],")
with tempfile.TemporaryDirectory(prefix='wf-ae20-program0-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('include',):shutil.copytree(ROOT/name,frozen/name)
 for name in('src/wf_r18_post_tag.c','src/wf_r17_m68k_shadow.c','tests/r18/post_tag_pending.c','audit/runtime18-cd/ROM68K.BIN'):
  dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
 script=frozen/'tests/r18/post_tag_verify_rom.py';script.write_text(body)
 source_hash=hashlib.sha256((frozen/'src/wf_r18_post_tag.c').read_bytes()).hexdigest()
 print('Frozen AE20 source',source_hash,flush=True)
 runpy.run_path(str(script),run_name='__main__')
