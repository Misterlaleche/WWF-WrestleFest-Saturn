"""Freeze three existing raw62 HOST guards; never run a combat probe."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
evidence=ROOT.parents[1]/'validation/local-20261024/human-tag-stage9-more-actions'
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c'];captures=[]
with tempfile.TemporaryDirectory(prefix='wf-action62-capture-',ignore_cleanup_errors=os.name=='nt')as t:
 frozen=Path(t)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'action62_oracle_bridge.c',frozen/'bridge.c')
 lib=frozen/'capture.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'bridge.c','-o',str(lib)],cwd=frozen,check=True)
 n=ctypes.CDLL(str(lib));n.action62_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 for index,frame in((0,8023),(2,8957),(5,7065)):
  policy=evidence/f'policy-{index:03}';report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
  assert report['block'].startswith(f'BLOCK frame={frame} ') and 'dependency=012614' in report['block']
  for name,want in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==want
  ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
  assert n.action62_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
  actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
  assert ram.raw[0x5b0:0x5b0+9*0x10c]==actors and bytes(d)==regs
  assert dump[layout[1]:layout[1]+len(actors)]==actors and dump[layout[2]:layout[2]+32]==regs
  slots=[i for i in range(9)if ram.raw[0x5b0+i*0x10c+0x60:0x5b0+i*0x10c+0x62]==bytes.fromhex('0062')]
  assert len(slots)==1;slot=slots[0];a=0x5b0+slot*0x10c;assert ram.raw[a+0x20:a+0x22]==bytes.fromhex('8005')
  snapshot=bytes(d)+ram.raw;name=f'action62_snapshot_host_policy{index:03}.bin';(HERE/name).write_bytes(snapshot)
  captures.append({'policy':index,'frame':frame,'slot':slot,'file':name,'snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'dumps_sha256':report['dump_sha256']})
 build=Path(report['command'][0]).parent
 assert build.name=='human-tag-stage9-final'
 manifest={'captures':captures,'origin':str(evidence),'build':str(build),'exe_sha256':hashlib.sha256(Path(report['command'][0]).read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((build/'source-manifest.json').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256((build/'frozen-sources.zip').read_bytes()).hexdigest(),'projection':'All9actors and D0-D7 byte equal to actual blocked states; export known globals/records, other arcadeRAMzero; no frame/HP/input/result/RNG substitution; no native trajectory claim.','rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest()}
 (HERE/'action62_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('PASS3 natural raw62 captures, all9actors/D8 exact')
