"""Freeze the observed Beetle raw43 guard; no emulator execution or state writes."""
import ctypes as ct,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
CAP=ROOT.parents[1]/'validation/local-20261024/hardware-input/beetle-full-flow-stage3-royal-basic'
report_bytes=(CAP/'report.json').read_bytes();j=json.loads(report_bytes)
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,147)
raw=(CAP/'workram-at-block.bin').read_bytes();assert len(raw)==0x200000
decoded=bytearray(raw);decoded[::2],decoded[1::2]=raw[1::2],raw[::2]
actors=b''.join(bytes.fromhex(s)for s in j['actors_at_block']);assert len(actors)==9*0x10c
at=decoded.find(actors);assert at>=0 and decoded.find(actors,at+1)<0,'Actor bytes must uniquely match WRAM'
with tempfile.TemporaryDirectory(prefix='wf-raw43-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'layout.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/r18/action43_layout.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ct.CDLL(str(lib));layout=(ct.c_uint32*16)();native.wf_action43_layout(layout);offsets=list(layout)
observed=struct.unpack('>5I',(ROOT.parents[1]/'validation/local-20261023/actors-layout.bin').read_bytes())
assert tuple(offsets[:3])==tuple(observed[:3]),('Global prefix must match supplied SH2 observer layout',offsets,observed)
rt_at=at-offsets[0]
def read(off,n=2):return int.from_bytes(decoded[rt_at+off:rt_at+off+n],'big')
assert read(offsets[3],4)==j['fatal_frame'] and offsets[3]==24
ram=bytearray(0x2000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,n in[(0x5c,offsets[2],4),(0x80,offsets[3],4),(0x161,offsets[4],1),(0x66,offsets[5],2),(0x7c,offsets[6],2),(0x1150,offsets[7],2),(0x1180,offsets[8],2),(0x11b0,offsets[9],2),(0x11e0,offsets[10],2),(0x162,offsets[11],2),(0x15d2,offsets[12],1),(0x15d3,offsets[13],1),(0x1800,offsets[14],2)]:ram[dest:dest+n]=read(off,n).to_bytes(n,'big')
actor=ram[0x5b0+8*0x10c:0x5b0+9*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')&255==0x43
regs=j['shadow_d_at_block'];assert len(regs)==8
pattern=struct.pack('>8I',*regs);assert decoded.find(pattern,rt_at,rt_at+7648)>=rt_at
capture=struct.pack('=6I',147,8,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
out=HERE/'action43_snapshot_beetle_frame147.bin';assert not out.exists()or out.read_bytes()==capture;out.write_bytes(capture)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(capture).hexdigest(),'snapshot_bytes':len(capture),'header':[147,8,1,0x1c03e,0,1],'emulator_report':str(CAP.relative_to(ROOT.parents[1])/'report.json'),'emulator_report_sha256':hashlib.sha256(report_bytes).hexdigest(),'workram_sha256':hashlib.sha256(raw).hexdigest(),'program_sha256':j['program_sha256'],'bin_sha256':j['bin_sha256'],'schedule_sha256':j['schedule_sha256'],'core_sha256':j['core_sha256'],'core_version':j['core_version'],'decoded_runtime_wram_offset':rt_at,'decoded_actor_wram_offset':at,'global_layout_offsets':offsets,'matched_actor_blocks':9,'matched_d_registers':8,'rng':read(offsets[2],4),'provenance':'Read-only Beetle guard capture at F4C2/12614 before raw43 callback. Default dispatch guard mutates no actor/RAM/D fields. All nine actor blocks uniquely match byte-swapped WRAM, D0-D7 from report. Typed global prefix layout matches supplied SH2 observer actors/RNG offsets and observed frame147. Projected supported globals only; unrepresented arcade RAM explicitly zero. No HOST identity/seed/frame/register forcing and no physical Saturn dump.','owner':'12614[43] -> descriptor18518, callback1853C','limits':['Captured typed Saturn model is projected into supported arcade RAM fields, not full original arcade RAM','No independent HOST all-actors match has been claimed for this Beetle Royal roster']}
# Pin the exact linked program and its static runtime symbol; do not infer
# the current compiler pointer tail from a different native build.
BUILD=ROOT.parents[1]/'validation/results/runtime18-full-flow-stage3-20261024'
program=(BUILD/'A.BIN').read_bytes();assert hashlib.sha256(program).hexdigest()==j['program_sha256']
assert hashlib.sha256((BUILD/'wrestlefest-runtime18.bin').read_bytes()).hexdigest()==j['bin_sha256']
assert hashlib.sha256((BUILD/'wrestlefest-runtime18.cue').read_bytes()).hexdigest()==j['cue_sha256']
elf=(BUILD/'wrestlefest-runtime18.elf').read_bytes();end='>'if elf[5]==2 else '<'
shoff=struct.unpack_from(end+'I',elf,32)[0];entsz,n,idx=struct.unpack_from(end+'HHH',elf,46)
sections=[struct.unpack_from(end+'10I',elf,shoff+i*entsz)for i in range(n)];symbol=None
for section in sections:
 if section[1]!=2:continue
 strings_section=sections[section[6]];strings=elf[strings_section[4]:strings_section[4]+strings_section[5]]
 for off in range(section[4],section[4]+section[5],section[9]):
  no,value,size,info,other,which=struct.unpack_from(end+'IIIBBH',elf,off)
  if strings[no:].split(b'\0',1)[0]==b'_rt':symbol=(value,size)
assert symbol and rt_at==0x100000+symbol[0]-0x06000000,(rt_at,symbol)
shadow_at=decoded.find(pattern,rt_at,rt_at+symbol[1]);assert shadow_at>=rt_at and decoded.find(pattern,shadow_at+1,rt_at+symbol[1])<0
frozen=json.loads((ROOT.parents[1]/'validation/local-20261024/full-flow-stage3-manifest.json').read_text())
header_sha=hashlib.sha256((ROOT/'include/wf_runtime.h').read_bytes()).hexdigest()
assert frozen['inputs']['src/runtime/include/wf_runtime.h']==header_sha
meta.update(elf_sha256=hashlib.sha256(elf).hexdigest(),map_sha256=hashlib.sha256((BUILD/'wrestlefest-runtime18.map').read_bytes()).hexdigest(),elf_runtime_address=symbol[0],elf_runtime_bytes=symbol[1],frozen_runtime_header_sha256=header_sha,shadow_d_wram_offset=decoded.find(pattern,rt_at,rt_at+symbol[1]))

(HERE/'action43_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta,indent=2))
