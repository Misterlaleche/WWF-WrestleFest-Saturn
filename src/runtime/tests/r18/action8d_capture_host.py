"""Export actual HOST guards8D/87; no combat execution or invented A registers."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
EVIDENCE=ROOT.parents[1]/'validation/local-20261024'
source=EVIDENCE/'human-tag-input-correct-sweep'
sources=json.loads((source/'source-manifest.json').read_text())['sources_sha256']
for name in ('include/wf_runtime.h','include/wf_actor.h','include/wf_world.h'):
 key=name.replace('/','\\')
 if key in sources:assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sources[key],name
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-action8d-capture-',ignore_cleanup_errors=os.name=='nt')as t:
 frozen=Path(t)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'action8d_oracle_bridge.c',frozen/'bridge.c')
 lib=frozen/'capture.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,'bridge.c','-o',str(lib)],cwd=frozen,check=True)
 n=ctypes.CDLL(str(lib));n.action8d_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 for action,directory,policy_name,frame,slot in (('8d','human-tag-input-correct-sweep','policy-002',7856,0),('87','human-tag-input-reach-policies','policy-000',7493,1)):
  policy=EVIDENCE/directory/policy_name;report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
  assert report['block'].startswith(f'BLOCK frame={frame} ')and 'dependency=012614' in report['block']
  for name,want in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==want,name
  ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
  assert n.action8d_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
  actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
  assert dump[:384]==(policy/'blocked-globals.bin').read_bytes()
  assert list(layout)==[7664,384,7340] and len(actors)==9*0x10c and len(regs)==32
  assert ram.raw[0x5b0:0xf1c]==actors and bytes(d)==regs
  assert dump[layout[1]:layout[1]+len(actors)]==actors and dump[layout[2]:layout[2]+32]==regs
  a=0x5b0+slot*0x10c;assert ram.raw[a+0x61]==int(action,16)
  header=[frame,slot,1,0x1c03e,0,1];snapshot=struct.pack('=6I',*header)+bytes(d)+ram.raw
  output=HERE/f'action{action}_snapshot_host{frame}.bin';output.write_bytes(snapshot)
  manifest={'frame':frame,'slot':slot,'action':action,'header':header,'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'origin':str(policy),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((source/'source-manifest.json').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256((source/'frozen-sources.zip').read_bytes()).hexdigest(),'dumps_sha256':report['dump_sha256'],'projection':'All9actors and D0-D7 byte equal. Typed represented globals/records exported over zero raw; no Aregs capture, no native trajectory claim. No combat execution.','rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest()}
  (HERE/f'action{action}_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
  print(f'PASS HOST{frame} raw{action} all9actors/D8/layout',manifest['snapshot_sha256'])
