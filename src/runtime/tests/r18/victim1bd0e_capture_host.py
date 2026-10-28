"""Export existing HOST6733 guard; no combat execution or invented A registers."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
EVIDENCE=ROOT.parents[1]/'validation/local-20261024';source=EVIDENCE/'human-tag-9b-actions-sweep'
policy=source/'policy-002'
sources=json.loads((source/'source-manifest.json').read_text())['sources_sha256']
for name in('include/wf_runtime.h','include/wf_actor.h'):
 assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sources[name.replace('/','\\')],name
report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
assert report['block'].startswith('BLOCK frame=6733 ')and 'dependency=01bd0e'in report['block']
for name,want in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==want,name
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-victim1bd0e-capture-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'victim1bd0e_oracle_bridge.c',frozen/'bridge.c')
 lib=frozen/'capture.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'bridge.c','-o',str(lib)],cwd=frozen,check=True)
 n=ctypes.CDLL(str(lib));n.victim1bd0e_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
 assert n.victim1bd0e_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
 actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
 assert list(layout)==[7664,384,7340] and len(dump)==7664 and len(actors)==9*0x10c and len(regs)==32
 assert dump[:384]==(policy/'blocked-globals.bin').read_bytes()
 assert ram.raw[0x5b0:0xf1c]==actors and bytes(d)==regs
 assert dump[384:384+len(actors)]==actors and dump[7340:7340+32]==regs
 a=0x5b0+5*0x10c;assert ram.raw[a+0x20:a+0x22]==bytes.fromhex('8004')and ram.raw[a+0x65]==0x1e
 assert int.from_bytes(ram.raw[a+0x66:a+0x68],'big')==0
 header=[6733,5,1,0x1c03e,0,1];snapshot=struct.pack('=6I',*header)+bytes(d)+ram.raw
 (HERE/'victim1bd0e_snapshot_host6733.bin').write_bytes(snapshot)
 manifest={'frame':6733,'slot':5,'owner':'1BD0E','header':header,'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'origin':str(policy),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256((source/'frozen-sources.zip').read_bytes()).hexdigest(),'dumps_sha256':report['dump_sha256'],'projection':'All9actors/D8 exact; represented typed globals exported over zero raw. No A registers available. Existing blocked HOST dump only; no combat execution.','rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest()}
 (HERE/'victim1bd0e_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('PASS HOST6733 victim1BD0E all9actors/D8/layout',manifest['snapshot_sha256'])
