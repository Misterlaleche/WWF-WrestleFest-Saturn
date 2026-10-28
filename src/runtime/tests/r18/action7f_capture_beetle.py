"""Project the read-only Stage8b raw7F guard; pin linked SH2 image and layout."""
import ctypes as ct,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
EVIDENCE=ROOT.parents[1]/'validation/local-20261024'
CAP=EVIDENCE/'hardware-input/beetle-full-flow-stage8b-human-tag'
BUILD=ROOT.parents[1]/'validation/results/runtime18-full-flow-stage8b-20261024'
report=(CAP/'report.json').read_bytes();j=json.loads(report)
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,9016)
raw=(CAP/'workram-at-block.bin').read_bytes();assert len(raw)==0x200000
d=bytearray(raw);d[::2],d[1::2]=raw[1::2],raw[::2]
elf=(BUILD/'wrestlefest-runtime18.elf').read_bytes();assert elf[5]==2
shoff=struct.unpack_from('>I',elf,32)[0];entsz,n,_=struct.unpack_from('>HHH',elf,46)
sections=[struct.unpack_from('>10I',elf,shoff+i*entsz)for i in range(n)];symbols={}
for s in sections:
 if s[1]!=2:continue
 st=sections[s[6]];strings=elf[st[4]:st[4]+st[5]]
 for off in range(s[4],s[4]+s[5],s[9]):
  no,value,size,*_=struct.unpack_from('>IIIBBH',elf,off)
  symbols[strings[no:].split(b'\0',1)[0].decode(errors='replace')]=(value,size)
rtaddr,rtsize=symbols['_rt'];rtat=0x100000+rtaddr-0x06000000
for name,key in(('A.BIN','program_sha256'),('wrestlefest-runtime18.bin','bin_sha256'),('wrestlefest-runtime18.cue','cue_sha256')):
 assert hashlib.sha256((BUILD/name).read_bytes()).hexdigest()==j[key]
frozen=json.loads((EVIDENCE/'full-flow-stage8b-manifest.json').read_text())
header_sha=hashlib.sha256((ROOT/'include/wf_runtime.h').read_bytes()).hexdigest()
assert frozen['inputs']['src/runtime/include/wf_runtime.h']==header_sha
source='''#include <stddef.h>
#include "wf_runtime.h"
void layout(uint32_t*o){o[0]=offsetof(wf_runtime_t,actors);o[1]=WF_ACTOR_BYTES;
o[2]=offsetof(wf_runtime_t,g.rng_state);o[3]=offsetof(wf_runtime_t,g.frame_counter);
o[4]=offsetof(wf_runtime_t,g.mode_0161);o[5]=offsetof(wf_runtime_t,g.config_0066);
o[6]=offsetof(wf_runtime_t,g.credited_route);o[7]=offsetof(wf_runtime_t,g.global_15d2);
o[8]=offsetof(wf_runtime_t,g.global_15d3);o[9]=offsetof(wf_runtime_t,m68k_regs);}
'''
with tempfile.TemporaryDirectory(prefix='wf-raw13-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 c=Path(td)/'layout.c';lib=Path(td)/'layout.so';c.write_text(source)
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',str(c),'-o',str(lib)],cwd=ROOT,check=True)
 native=ct.CDLL(str(lib));offsets=(ct.c_uint32*10)();native.layout(offsets);offsets=list(offsets)
# Pointer-free prefix checked against Stage8b frozen header and fresh SH2 layout.
assert offsets==[384,268,352,24,31,4,12,210,211,7340] and rtsize==7648
sh2=(EVIDENCE/'hardware-input/actors-layout-stage8b.s').read_bytes()
import re
assert [int(x)for x in re.findall(rb'\.long\s+(\d+)',sh2)]==[384,268,352,7340,0]
actors=b''.join(bytes.fromhex(s)for s in j['actors_at_block']);assert len(actors)==9*0x10c
at=rtat+offsets[0];assert d[at:at+len(actors)]==actors and d.find(actors)==at and d.find(actors,at+1)<0
def read(off,n):return bytes(d[rtat+off:rtat+off+n])
assert int.from_bytes(read(offsets[3],4),'big')==9016
regs=j['shadow_d_at_block'];assert read(offsets[9],32)==struct.pack('>8I',*regs)
ram=bytearray(0x4000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,n in((0x5c,offsets[2],4),(0x80,offsets[3],4),(0x161,offsets[4],1),(0x66,offsets[5],2),(0x7c,offsets[6],2),(0x15d2,offsets[7],1),(0x15d3,offsets[8],1)):ram[dest:dest+n]=read(off,n)
a=0x5b0+1*0x10c;assert ram[a+2:a+4]==b'\0\x07' and ram[a+0x60:a+0x62]==bytes.fromhex('007f') and ram[a+0x66:a+0x68]==b'\0\0'
capture=struct.pack('=6I',9016,1,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
out=HERE/'action7f_snapshot_beetle_stage8b_frame9016.bin';assert not out.exists()or out.read_bytes()==capture;out.write_bytes(capture)
meta={'sha256':hashlib.sha256(capture).hexdigest(),'header':[9016,1,1,0x1c03e,0,1],'snapshot_bytes':len(capture),'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','source':str(CAP.relative_to(ROOT.parents[1])),'report_sha256':hashlib.sha256(report).hexdigest(),'workram_sha256':hashlib.sha256(raw).hexdigest(),'program_sha256':j['program_sha256'],'bin_sha256':j['bin_sha256'],'core_sha256':j['core_sha256'],'core_version':j['core_version'],'elf_sha256':hashlib.sha256(elf).hexdigest(),'elf_runtime_address':rtaddr,'elf_runtime_bytes':rtsize,'runtime_wram_offset':rtat,'layout_offsets':offsets,'frozen_runtime_header_sha256':header_sha,'reference_sh2_layout_sha256':hashlib.sha256(sh2).hexdigest(),'matched_actors':9,'D0-D7':regs,'limits':['Read-only Beetle observation, no emulator RAM writes or input/seed forcing','Typed native model projected to all9actors and supported globals; other arcade RAM explicitly zero','No A registers available or fabricated; not physical Saturn or complete original arcade RAM']}
(HERE/'action7f_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta,indent=2))
