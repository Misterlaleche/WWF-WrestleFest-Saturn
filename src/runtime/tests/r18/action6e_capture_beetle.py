"""Project read-only Beetle stage4 raw6E guard into a new oracle fixture."""
from pathlib import Path
import ctypes as ct,hashlib,json,os,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
CAP=ROOT.parents[1]/'validation/local-20261024/hardware-input/beetle-full-flow-stage4-tag-basic'
report=(CAP/'report.json').read_bytes();j=json.loads(report)
assert (j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,1724)
raw=(CAP/'workram-at-block.bin').read_bytes();assert len(raw)==0x200000
decoded=bytearray(raw);decoded[::2],decoded[1::2]=raw[1::2],raw[::2]
actors=bytes.fromhex(''.join(j['actors_at_block']));assert len(actors)==9*0x10c
at=decoded.find(actors);assert at>=0 and decoded.find(actors,at+1)<0,'all9actors must uniquely match WRAM'
source='''#include <stddef.h>
#include "wf_runtime.h"
void layout(uint32_t o[9]){
o[0]=offsetof(wf_runtime_t,actors);o[1]=WF_ACTOR_BYTES;
o[2]=offsetof(wf_runtime_t,g.rng_state);o[3]=offsetof(wf_runtime_t,g.frame_counter);
o[4]=offsetof(wf_runtime_t,g.mode_0161);o[5]=offsetof(wf_runtime_t,g.config_0066);
o[6]=offsetof(wf_runtime_t,g.credited_route);o[7]=offsetof(wf_runtime_t,g.global_15d2);
o[8]=offsetof(wf_runtime_t,g.global_15d3);}
'''
with tempfile.TemporaryDirectory(prefix='wf-raw6e-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 c=Path(td)/'layout.c';lib=Path(td)/'layout.so';c.write_text(source)
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',str(c),'-o',str(lib)],cwd=ROOT,check=True)
 native=ct.CDLL(str(lib));offsets=(ct.c_uint32*9)();native.layout(offsets);offsets=list(offsets)
observed=struct.unpack('>5I',(ROOT.parents[1]/'validation/local-20261023/actors-layout.bin').read_bytes())
assert tuple(offsets[:3])==tuple(observed[:3]),'typed prefix must match supplied SH2 observer layout'
rt_at=at-offsets[0]
def read(off,n):return bytes(decoded[rt_at+off:rt_at+off+n])
assert int.from_bytes(read(offsets[3],4),'big')==1724 and read(offsets[4],1)==b'\0'
ram=bytearray(0x4000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,n in[(0x5c,offsets[2],4),(0x80,offsets[3],4),(0x161,offsets[4],1),(0x66,offsets[5],2),(0x7c,offsets[6],2),(0x15d2,offsets[7],1),(0x15d3,offsets[8],1)]:ram[dest:dest+n]=read(off,n)
assert ram[0x5b0+0x60:0x5b0+0x62]==bytes.fromhex('006e') and ram[0x5b0+2:0x5b0+4]==bytes.fromhex('0007')
regs=j['shadow_d_at_block'];assert len(regs)==8
capture=struct.pack('=6I',1724,0,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
out=HERE/'action6e_snapshot_beetle_stage4_frame1724.bin';assert not out.exists()or out.read_bytes()==capture;out.write_bytes(capture)
meta={'sha256':hashlib.sha256(capture).hexdigest(),'header':[1724,0,1,0x1c03e,0,1],'snapshot_bytes':len(capture),'source':str(CAP.relative_to(ROOT.parents[1])),'report_sha256':hashlib.sha256(report).hexdigest(),'workram_sha256':hashlib.sha256(raw).hexdigest(),'reported_program_sha256':j['program_sha256'],'reported_bin_sha256':j['bin_sha256'],'reported_core_sha256':j['core_sha256'],'core_version':j['core_version'],'decoded_actor_offset':at,'decoded_runtime_offset':rt_at,'layout_offsets':offsets,'matched_actors':9,'D0-D7':regs,'note':'Read-only real Beetle stage4 guard. All9actors uniquely match decodedWRAM, D0-D7 from report; typed prefix matches SH2 observer and frame. Supported globals projected, other arcadeRAM explicitly zero. No A registers available or fabricated. Not physical Saturn hardware or full original arcadeRAM.'}
(HERE/'action6e_beetle_stage4_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta))
