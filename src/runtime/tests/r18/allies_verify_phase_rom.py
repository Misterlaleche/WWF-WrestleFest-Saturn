#!/usr/bin/env python3
"""Observed phase inputs vs complete ROM $1C150, including relation $1C228.

Catches a missing per-actor relation refresh, missing scheduler D0 setup,
incorrect actor order, and choosing an attack with stale zero distances.
"""
import argparse,ctypes,hashlib,json,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('snapshots',type=Path);p.add_argument('--report',type=Path)
args=p.parse_args();root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();results=[]
with tempfile.TemporaryDirectory() as temp:
 lib=Path(temp)/'phase.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/allies_phase_oracle_bridge.c','-o',str(lib)],cwd=root,check=True)
 native=ctypes.CDLL(str(lib));native.allies_phase_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32)]
 native.allies_actor_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for sequence in (0,4,25,26,105,138):
  path=args.snapshots/f'snapshot-{sequence:05}.bin';data=path.read_bytes();frame,slot,kind,pc,_,status=struct.unpack_from('=6I',data)
  assert slot==1 and kind==0
  before=data[56:8248];regs=struct.unpack_from('=8I',data,24)
  mem=bytearray(0x200000);mem[:len(rom)]=rom;mem[0x1c0000:0x1c2000]=before
  nb=ctypes.create_string_buffer(bytes(mem));nr=(ctypes.c_uint32*8)(*regs)
  status=native.allies_phase_oracle(nb,nr)
  machine=mus.Machine(mus.CPUType.M68000,2048);u=machine.cpu;m=machine.mem;m.w_block(0,bytes(mem))
  for page in range(0,0x80000,0x10000):
   for width in range(3):
    m.set_special_range_write_func(page,width,lambda address,value:None)
    m.set_special_range_read_func(page,width,lambda address,n=1<<width:int.from_bytes(rom[address:address+n],'big'))
  for i in range(8,15):u.w_reg(i,0)
  for i,value in enumerate(regs):u.w_reg(i,value)
  u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x1c150)
  visited=[];actor_order=[]
  for steps in range(100000):
   at=u.r_pc();visited.append(hex(at))
   if at==0x70000:break
   if at==0x1c228:actor_order.append((u.r_reg(8)-0x1c05b0)//0x10c)
   if at==0x2052:u.w_pc(m.r32(u.r_sp()));u.w_sp(u.r_sp()+4);continue
   u.execute(1)
  assert u.r_pc()==0x70000,('ROM runaway',path.name,hex(u.r_pc()))
  differences=[]
  for at,n in [(0x1c05b0,9*0x10c),(0x1c005c,4),(0x1c15e4,8),(0x1c1697,1),(0x1c1670,4),(0x1c167a,1),(0x1c1800,4),(0x1c1684,8)]:
   want=bytes(m.r_block(at,n));got=nb.raw[at:at+n]
   differences.extend({'address':hex(at+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(got,want)) if g!=e)
  registers=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(u.r_reg(i))} for i in range(8) if nr[i]!=u.r_reg(i)]
  a=0x1c06bc
  read=lambda m,at,n=2:int.from_bytes(m[at:at+n],'big')
  summary=lambda block:{'distance':read(block,a+0xb0),'distance_y':read(block,a+0xb2),'distance_class':read(block,a+0xb4,1),'state':hex(read(block,a+0x20)),'selector':hex(read(block,a+0x60))}
  rm=bytes(m.r_block(0,0x200000))
  result={'snapshot':path.name,'frame':frame,'native_status':status,'actor_order':actor_order,'before':summary(mem),'native':summary(nb.raw),'rom':summary(rm),'memory_differences':differences,'register_differences':registers,'visited':visited}
  if frame in (654,900):
   # Continue the ally's next decision from each side's phase output. This
   # isolates the effect of refreshed distance without simulating a match.
   next_status=native.allies_actor_oracle(nb,nr,1)
   u.w_reg(8,a);u.w_reg(9,0x1c8f4);u.w_sp(0x1d0000);m.w32(0x1d0000,0x70000);u.w_pc(0x1c8f4)
   for _ in range(100000):
    if u.r_pc()==0x70000:break
    u.execute(1)
   assert u.r_pc()==0x70000
   result['next_action']={'native_status':next_status,'native':summary(nb.raw),'rom':summary(bytes(m.r_block(0,0x200000)))}
   result['next_action']['actors_equal']=nb.raw[0x1c05b0:0x1c05b0+9*0x10c]==bytes(m.r_block(0x1c05b0,9*0x10c))
   result['next_action']['registers_equal']=list(nr)==[u.r_reg(i) for i in range(8)]
  results.append(result)
  machine.cleanup()
report={'count':len(results),'passed':sum(not r['native_status'] and not r['memory_differences'] and not r['register_differences'] and ('next_action' not in r or not r['next_action']['native_status'] and r['next_action']['actors_equal'] and r['next_action']['registers_equal']) for r in results),'results':results}
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n')
print('Complete CPU-phase real-snapshot ROM checks:',report['passed'],'/',report['count'])
for r in results:
 print(r['snapshot'],r['frame'],'status',r['native_status'],'before',r['before'],'native',r['native'],'rom',r['rom'],'diffs',r['memory_differences'][:5],'regs',r['register_differences'][:3])
 if 'next_action' in r:print(' next ally decision',r['next_action'])
raise SystemExit(0 if report['passed']==report['count'] else 1)
