"""Freeze the observed Beetle raw2A guard; no emulator execution or state writes."""
import ctypes as ct,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
CAP=ROOT.parents[1]/'validation/local-20261024/hardware-input/beetle-full-flow-stage3-tag-basic'
report_bytes=(CAP/'report.json').read_bytes();j=json.loads(report_bytes)
assert(j['status'],j['fatal_owner'],j['fatal_dependency'],j['fatal_frame'])==('blocked',0xf4c2,0x12614,327)
raw=(CAP/'workram-at-block.bin').read_bytes();assert len(raw)==0x200000
decoded=bytearray(raw);decoded[::2],decoded[1::2]=raw[1::2],raw[::2]
actors=b''.join(bytes.fromhex(s)for s in j['actors_at_block']);assert len(actors)==9*0x10c
at=decoded.find(actors);assert at>=0 and decoded.find(actors,at+1)<0,'Actor bytes must uniquely match WRAM'
with tempfile.TemporaryDirectory(prefix='wf-raw2A-layout-',ignore_cleanup_errors=os.name=='nt')as td:
 lib=Path(td)/'layout.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','tests/r18/action41_layout.c','-o',str(lib)],cwd=ROOT,check=True)
 native=ct.CDLL(str(lib));layout=(ct.c_uint32*11)();native.wf_action41_layout(layout);offsets=list(layout)
observed=struct.unpack('>5I',(ROOT.parents[1]/'validation/local-20261023/actors-layout.bin').read_bytes())
assert tuple(offsets[:3])==tuple(observed[:3]),('Global prefix must match supplied SH2 observer layout',offsets,observed)
rt_at=at-offsets[0]
def read(off,n=2):return int.from_bytes(decoded[rt_at+off:rt_at+off+n],'big')
assert read(offsets[3],4)==j['fatal_frame'] and offsets[3]==24
ram=bytearray(0x2000);ram[0x5b0:0x5b0+len(actors)]=actors
for dest,off,n in[(0x5c,offsets[2],4),(0x80,offsets[3],4),(0x161,offsets[4],1),(0x66,offsets[5],2),(0x7c,offsets[6],2),(0x1150,offsets[7],2),(0x1180,offsets[8],2),(0x11b0,offsets[9],2),(0x11e0,offsets[10],2)]:ram[dest:dest+n]=read(off,n).to_bytes(n,'big')
actor=ram[0x5b0+4*0x10c:0x5b0+5*0x10c];assert int.from_bytes(actor[0x60:0x62],'big')&255==0x2a
regs=j['shadow_d_at_block'];assert len(regs)==8
capture=struct.pack('=6I',327,4,1,0x1c03e,0,1)+struct.pack('=8I',*regs)+ram
out=HERE/'action2a_snapshot_beetle_frame327.bin';assert not out.exists()or out.read_bytes()==capture;out.write_bytes(capture)
meta={'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','snapshot_sha256':hashlib.sha256(capture).hexdigest(),'snapshot_bytes':len(capture),'header':[327,4,1,0x1c03e,0,1],'emulator_report':str(CAP.relative_to(ROOT.parents[1])/'report.json'),'emulator_report_sha256':hashlib.sha256(report_bytes).hexdigest(),'workram_sha256':hashlib.sha256(raw).hexdigest(),'program_sha256':j['program_sha256'],'bin_sha256':j['bin_sha256'],'schedule_sha256':j['schedule_sha256'],'core_sha256':j['core_sha256'],'core_version':j['core_version'],'decoded_runtime_wram_offset':rt_at,'decoded_actor_wram_offset':at,'global_layout_offsets':offsets,'matched_actor_blocks':9,'matched_d_registers':8,'rng':read(offsets[2],4),'provenance':'Read-only Beetle guard capture at F4C2/12614 before raw2A callback. Default dispatch guard mutates no actor/RAM/D fields. All nine actor blocks uniquely match byte-swapped WRAM, D0-D7 from report. Typed global prefix layout matches supplied SH2 observer actors/RNG offsets and observed frame327. Projected supported globals only; unrepresented arcade RAM explicitly zero. No HOST identity/seed/frame/register forcing and no physical Saturn dump.','owner':'12614[2A] -> descriptor16614, callback1662C','limits':['Captured typed Saturn model is projected into supported arcade RAM fields, not full original arcade RAM','No independent HOST all-actors match has been claimed for this Beetle-specific7/2 roster']}
(HERE/'action2a_manifest.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(meta,indent=2))
