"""Export existing HOST6724 guard; no combat execution or invented A registers."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
EVIDENCE=ROOT.parents[1]/'validation/local-20261024';source=EVIDENCE/'human-tag-stage9-final'
policy=EVIDENCE/'human-tag-stage9-more-actions/policy-003'
sources=json.loads((source/'source-manifest.json').read_text())['sources_sha256']
for name in('include/wf_runtime.h','include/wf_actor.h'):
 assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sources[name.replace('/','\\')],name
report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
assert report['block'].startswith('BLOCK frame=6724 ')and 'dependency=012614'in report['block']
for name,want in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==want,name
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-action54-capture-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'action54_oracle_bridge.c',frozen/'bridge.c')
 lib=frozen/'capture.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'bridge.c','-o',str(lib)],cwd=frozen,check=True)
 n=ctypes.CDLL(str(lib));n.action54_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
 assert n.action54_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
 actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
 assert list(layout)==[7664,384,7340] and len(dump)==7664 and len(actors)==9*0x10c and len(regs)==32
 assert dump[:384]==(policy/'blocked-globals.bin').read_bytes()
 assert ram.raw[0x5b0:0xf1c]==actors and bytes(d)==regs
 assert dump[384:384+len(actors)]==actors and dump[7340:7340+32]==regs
 a=0x5b0+0x10c;assert ram.raw[a+0x20:a+0x22]==bytes.fromhex('8005')and ram.raw[a+0x60:a+0x62]==bytes.fromhex('0054')
 assert int.from_bytes(ram.raw[a+2:a+4],'big')==7 and int.from_bytes(ram.raw[a+0x66:a+0x68],'big')==1
 assert ram.raw[a+0x26:a+0x2a]==bytes.fromhex('001c0aec')
 header=[6724,1,1,0x1c03e,0,1];snapshot=struct.pack('=6I',*header)+bytes(d)+ram.raw
 (HERE/'action54_snapshot_host6724.bin').write_bytes(snapshot)
 manifest={'frame':6724,'slot':1,'action':'54','header':header,'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'origin':str(policy),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256((source/'frozen-sources.zip').read_bytes()).hexdigest(),'dumps_sha256':report['dump_sha256'],'projection':'All9actors/D8 exact; represented typed globals exported over zero raw. No A registers available. Existing blocked HOST dump only; no combat execution.','rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest()}
 (HERE/'action54_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('PASS HOST6724 raw54 all9actors/D8/layout',manifest['snapshot_sha256'])
