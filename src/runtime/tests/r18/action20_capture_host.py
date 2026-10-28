"""Read existing HOST5845 raw20 guard; all9actors/D8 retained, no combat."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
E=ROOT.parents[1]/'validation/local-20261024';policy=E/'human-tag-stage9-more-actions/policy-004'
report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
assert report['block'].startswith('BLOCK frame=5845 ')and'dependency=012614'in report['block']
for name,h in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==h
source=E/'human-tag-stage9-final/source-manifest.json';meta=json.loads(source.read_text())
assert hashlib.sha256(Path(report['command'][0]).read_bytes()).hexdigest()==meta['binary_sha256']
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
with tempfile.TemporaryDirectory(prefix='wf-20-capture-',ignore_cleanup_errors=os.name=='nt')as td:
 t=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,t/name)
 shutil.copy2(HERE/'action20_oracle_bridge.c',t/'bridge.c');lib=t/'capture.dll'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'src/wf_r18_runtime_handoff.c','bridge.c','-o',str(lib)],cwd=t,check=True)
 n=ctypes.CDLL(str(lib));n.action20_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
 assert n.action20_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
 actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
 assert list(layout)==[7664,384,7340]and len(actors)==9*0x10c and len(regs)==32
 assert ram.raw[0x5b0:0xf1c]==actors and bytes(d)==regs
 assert dump[384:384+len(actors)]==actors and dump[7340:7372]==regs
 assert ram.raw[0x5b0+5*0x10c+0x61]==0x20
 header=[5845,5,1,0x1c03e,0,1];snapshot=struct.pack('=6I',*header)+bytes(d)+ram.raw
 (HERE/'action20_snapshot_host5845.bin').write_bytes(snapshot)
 result={'header':header,'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'origin':str(policy),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'binary_sha256':meta['binary_sha256'],'dumps_sha256':report['dump_sha256'],'rom_sha1':'a726c46f350723ea73158bc78055d17d418b14e2','limits':['Typed HOST projection; unrepresented RAM zero','No Aregs captured; no state/value substitutions','No native/physical trajectory equivalence claim']}
 (HERE/'action20_manifest.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS all9actors/D8/layout',result['snapshot_sha256'])
