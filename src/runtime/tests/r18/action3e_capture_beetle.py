"""Read-only projection of the pinned Stage7b raw3E guard, no emulator writes."""
from pathlib import Path
import ctypes,hashlib,json,os,re,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent;BASE=ROOT.parents[1]
E=BASE/'validation/local-20261024';CAP=E/'hardware-input/beetle-full-flow-stage7b-human-tag';BUILD=BASE/'validation/results/runtime18-full-flow-stage7b-20261024'
report_bytes=(CAP/'report.json').read_bytes();j=json.loads(report_bytes);raw=(CAP/'workram-at-block.bin').read_bytes()
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,1230)
assert len(raw)==0x200000 and not j['hardware_test']
decoded=bytearray(raw);decoded[::2],decoded[1::2]=raw[1::2],raw[::2]
actors=b''.join(bytes.fromhex(s)for s in j['actors_at_block']);assert len(actors)==9*0x10c
actors_at=decoded.find(actors);assert actors_at>=0 and decoded.find(actors,actors_at+1)<0
elf=(BUILD/'wrestlefest-runtime18.elf').read_bytes();assert elf[:6]==b'\x7fELF\x01\x02'
shoff=struct.unpack_from('>I',elf,32)[0];entsz,n,idx=struct.unpack_from('>HHH',elf,46)
sections=[struct.unpack_from('>10I',elf,shoff+i*entsz)for i in range(n)];symbols={}
for s in sections:
 if s[1]!=2:continue
 st=sections[s[6]];strings=elf[st[4]:st[4]+st[5]]
 for off in range(s[4],s[4]+s[5],s[9]):
  no,value,size,info,other,which=struct.unpack_from('>IIIBBH',elf,off);symbols[strings[no:].split(b'\0',1)[0].decode(errors='replace')]=(value,size)
rtname=next(name for name in symbols if name=='_rt'or name.startswith('_rt.'));rtaddr,rtsize=symbols[rtname];rtat=0x100000+rtaddr-0x06000000
sh2=(E/'hardware-input/actors-layout-stage7b.s').read_bytes();layout_sh2=[int(v)for v in re.findall(rb'\.long\s+(\d+)',sh2)];assert layout_sh2==[384,268,352,7340,0]
freeze=json.loads((E/'full-flow-stage7b-manifest.json').read_bytes())
for name in('wf_runtime.h','wf_actor.h'):
 assert hashlib.sha256((ROOT/'include'/name).read_bytes()).hexdigest()==freeze['inputs']['src/runtime/include/'+name]
with tempfile.TemporaryDirectory(prefix='wf-3e-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'layout.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/r18/action3e_layout.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ctypes.CDLL(str(lib));layout=list((ctypes.c_uint32*14).in_dll(native,'wf_action3e_layout'))
assert layout[:5]==layout_sh2 and layout[5]==24 and rtat+layout[0]==actors_at
assert decoded[rtat+24:rtat+28]==(1230).to_bytes(4,'big')
regs=j['shadow_d_at_block'];assert len(regs)==8 and bytes(decoded[rtat+layout[3]:rtat+layout[3]+32])==struct.pack('>8I',*regs)
assert int.from_bytes(actors[0x10c+2:0x10c+4],'big')==7 and int.from_bytes(actors[0x10c+0x60:0x10c+0x62],'big')==0x3e
for name,key in(('A.BIN','program_sha256'),('wrestlefest-runtime18.bin','bin_sha256'),('wrestlefest-runtime18.cue','cue_sha256')):assert hashlib.sha256((BUILD/name).read_bytes()).hexdigest()==j[key]
ram=bytearray(0x4000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,nbytes in[(0x5c,layout[2],4),(0x80,layout[5],4)]+list(zip((0x161,0x7c,0x7e,0x1670,0x1150,0x1180,0x11b0,0x11e0),layout[6:],(1,2,2,4,2,2,2,2))):ram[dest:dest+nbytes]=decoded[rtat+off:rtat+off+nbytes]
snapshot=struct.pack('=6I',1230,1,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
path=HERE/'action3e_snapshot_beetle_frame1230.bin';assert not path.exists()or path.read_bytes()==snapshot;path.write_bytes(snapshot)
meta={'header':[1230,1,1,0x1c03e,0,1],'snapshot_bytes':len(snapshot),'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','emulator_report':str(CAP.relative_to(BASE)/'report.json'),'emulator_report_sha256':hashlib.sha256(report_bytes).hexdigest(),'workram_sha256':hashlib.sha256(raw).hexdigest(),'program_sha256':j['program_sha256'],'bin_sha256':j['bin_sha256'],'cue_sha256':j['cue_sha256'],'core_sha256':j['core_sha256'],'core_version':j['core_version'],'schedule_sha256':j['schedule_sha256'],'elf_sha256':hashlib.sha256(elf).hexdigest(),'map_sha256':hashlib.sha256((BUILD/'wrestlefest-runtime18.map').read_bytes()).hexdigest(),'freeze_manifest_sha256':hashlib.sha256((E/'full-flow-stage7b-manifest.json').read_bytes()).hexdigest(),'sh2_layout_sha256':hashlib.sha256(sh2).hexdigest(),'sh2_layout':layout_sh2,'scalar_prefix_layout':layout,'elf_runtime_symbol':rtname,'elf_runtime_address':rtaddr,'elf_runtime_bytes':rtsize,'runtime_wram_offset':rtat,'actors_wram_offset':actors_at,'matched_actor_records':9,'matched_live_D_registers':8,'owner':'12614[3E] ->18314/1831A, 1174C/11710 and four original walk descriptors','limits':['Read-only Beetle Stage7b capture, not physical Saturn RAM/timing','Native typed actors/liveD8/supported scalar globals projected into arcadeRAM; all unrepresented original RAM explicitly zero','Default12614 guard occurs before callback and changes no actor/RAM/D fields; no state, seed, roster, health or result forcing']}
(HERE/'action3e_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta,indent=2))
