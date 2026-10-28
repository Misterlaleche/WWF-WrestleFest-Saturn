#!/usr/bin/env python3
"""CPU data read-before-write evidence; no native RAM reconstruction policy."""
import json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
target=Path(sys.argv[1])if len(sys.argv)>1 else None
saved=sys.argv;sys.argv=[str(HERE/'post_tag_verify_rom.py')]
ns={'__file__':str(HERE/'post_tag_verify_rom.py'),'__name__':'post_tag_readset_support'}
exec(compile((HERE/'post_tag_verify_rom.py').read_text().split('source=ROOT/')[0],str(HERE/'post_tag_verify_rom.py'),'exec'),ns)
sys.argv=saved
machine=ns['machine'];put=ns['put'];seed=ns['seed'];execute=ns['execute'];reads={};written=set();instructions={}
def trace(kind,width,at,value):
 if not(0x1c0000<=at<0x1c4000):return
 n=1<<width
 if kind=='W':written.update(range(at,at+n))
 elif kind=='R':
  pc=machine.cpu.r_pc()
  for byte in range(at,at+n):
   if byte not in written:reads.setdefault(byte-0x1c0000,set()).add(pc)
def spans(values):
 out=[]
 for at in sorted(values):
  if out and at==out[-1][1]:out[-1][1]+=1
  else:out.append([at,at+1])
 return [[f'{a:04X}',f'{b-1:04X}']for a,b in out]
report=[]
for name,owner,progress,mode,stops in [('victory-first',0x11b6,0,0,(0xac0,)),('victory-second',0x122c,1,0,(0xac0,)),('interview-later',0xbda6,7,0,(0xbf56,)),('continue-timeout',0x122c,0,0,(0xac0,0x6fc))]:
 mem=seed(progress);put(mem,0x1c0161,mode,1)
 if name=='continue-timeout':put(mem,0x1c0650,0x8001);put(mem,0x1c0080,0x12345678,4);put(mem,0x1c004f,0,1)
 reads={};written=set();machine.mem.set_trace_func(trace);machine.mem.set_trace_mode(True)
 regs=list(range(15));_,_,stop=execute(mem,regs,owner,stops)
 machine.mem.set_trace_mode(False)
 report.append({'case':name,'entry':hex(owner),'exit':hex(stop),'read_before_write_spans':spans(reads),'bytes':[{'offset':f'{at:04X}','observed_pc':[f'{pc:06X}'for pc in sorted(pcs)]}for at,pcs in sorted(reads.items())]})
 print(name,'initial-read-bytes',len(reads),'high-spans',spans([at for at in reads if at>=0x15de]),flush=True)
result={'rom_sha1':ns['hashlib'].sha1(ns['rom']).hexdigest(),'scope':'Intact ROM CPU memory tracing; high stack outside1C0000..1C3FFF; ACK/levels supplied by exact busy-poll tape. IRQ owners are independently audited and not injected here. A trace callback PC is the CPU current instruction position and can include fetched extension words. Initial reads are listed conservatively, without assuming zero.','cases':report}
if target:target.write_text(json.dumps(result,indent=2)+'\n')
