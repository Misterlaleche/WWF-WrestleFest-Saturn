#!/usr/bin/env python3
"""RAW $6C: full native bounded dispatcher versus untouched ROM $1C03E.
No routines are intercepted. Real snapshot1344 and descriptor/landing edges.
"""
import argparse,ctypes,hashlib,itertools,json,random,re,struct,subprocess,tempfile
from pathlib import Path
import machine68k as mus
p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--report',type=Path);a=p.parse_args()
here=Path(__file__).resolve().parent;root=here.parents[1];manifest=json.loads((here/'action6c_manifest.json').read_text());rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes();snap=(here/'action6c_snapshot_f88acad.bin').read_bytes()
assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1'];assert hashlib.sha256(snap).hexdigest()==manifest['fixture_sha256'];assert rom[0x127c4:0x127c8]==bytes.fromhex('00019c28')
rng=random.Random(0x19c48);original=bytearray(0x200000);original[:len(rom)]=rom;original[0x1c0000:0x1c2000]=snap[56:8248];cases=[('natural-noinput1344-slot5',original,5,struct.unpack_from('=8I',snap,24))]
def w(m,p,v,n=2):m[p:p+n]=(v&((1<<(8*n))-1)).to_bytes(n,'big')
for slot,sel,facing in itertools.product(range(9),(0x6c,0x406c,0x806c,0xc06c),(0,0x8000)):
 m=bytearray(original);p=0x1c05b0+slot*0x10c;w(m,p+0x20,0x8005);w(m,p+0x1c,5);w(m,p+0x60,sel);w(m,p+0x2e,facing)
 cases.append((f'derived-init-{slot}-{sel}-{facing}',m,slot,tuple(rng.getrandbits(32) for _ in range(8))))
for frame,timer,landing,facing in itertools.product((0,1,2,3,4,5,0xfe),(0,1,2,0xff00),(0,0x10),(0,0x8000)):
 m=bytearray(original);p=0x1c05b0+5*0x10c;w(m,p+0x20,5);w(m,p+0x1c,0x8005);w(m,p+0x60,0x6c);w(m,p+0x24,frame);w(m,p+0x22,timer);w(m,p+0x37,landing,1);w(m,p+0x2e,facing)
 cases.append((f'derived-tick-{frame}-{timer}-{landing}-{facing}',m,5,tuple(rng.getrandbits(32) for _ in range(8))))
if a.quick:cases=cases[:1]
core=re.search(r'^CORE=(.*)$',(root/'Makefile').read_text(),re.M)[1].split();results=[];coverage=set()
with tempfile.TemporaryDirectory() as tmp:
 lib=Path(tmp)/'raw6c.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'tests/r18/action6c_oracle_bridge.c','-o',str(lib)],cwd=root,check=True);native=ctypes.CDLL(str(lib));native.wf_action6c_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 for label,m,slot,regs in cases:
  nb=ctypes.create_string_buffer(bytes(m));nr=(ctypes.c_uint32*8)(*regs);status=native.wf_action6c_oracle(nb,nr,slot)
  machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,bytes(m));cpu.w_sr(0x2700)
  for i,v in enumerate(regs):cpu.w_reg(i,v)
  cpu.w_reg(8,0x1c05b0+slot*0x10c);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
  for steps in range(20000):
   pc=cpu.r_pc();coverage.add(pc)
   if pc==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(label,'ROM runaway',hex(cpu.r_pc()))
  diff=[]
  for at,n in [(0x1c0000,0x2000),(0xc0000,0x4000)]:diff.extend({'address':hex(at+i),'native':g,'rom':e} for i,(g,e) in enumerate(zip(nb.raw[at:at+n],mem.r_block(at,n))) if g!=e)
  regdiff=[{'register':'D'+str(i),'native':hex(nr[i]),'rom':hex(cpu.r_reg(i))} for i in range(8) if nr[i]!=cpu.r_reg(i)]
  results.append({'label':label,'status':status,'memory_differences':diff,'register_differences':regdiff});machine.cleanup()
failed=[r for r in results if r['status'] or r['memory_differences'] or r['register_differences']];report={**manifest,'count':len(results),'passed':len(results)-len(failed),'natural_count':1,'rom_pc_coverage':[hex(pc) for pc in sorted(coverage)],'results':results}
if a.report:a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
print('RAW6C ROM',report['passed'],'/',report['count'])
for r in failed[:4]:print(r['label'],'status',r['status'],'memory',r['memory_differences'][:8],'regs',r['register_differences'])
raise SystemExit(bool(failed))
