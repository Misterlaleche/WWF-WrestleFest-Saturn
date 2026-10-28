"""Freeze actual HOST284 CPU guard, without executing combat or rewinding it."""
from pathlib import Path
import ctypes,hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
evidence=ROOT.parents[1]/'validation/local-20261024/human-tag-input-correct-sweep'
policy=evidence/'policy-003';report=json.loads((policy/'report.json').read_text());dump=(policy/'blocked-runtime.bin').read_bytes()
assert report['block'].startswith('BLOCK frame=284 ') and 'dependency=01d3f4' in report['block']
for name,want in report['dump_sha256'].items():assert hashlib.sha256((policy/name).read_bytes()).hexdigest()==want
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()+['src/wf_r18_runtime_handoff.c']
with tempfile.TemporaryDirectory(prefix='wf-1d3f4-capture-',ignore_cleanup_errors=os.name=='nt')as t:
 frozen=Path(t)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 shutil.copy2(HERE/'cpu_1d3f4_oracle_bridge.c',frozen/'bridge.c')
 lib=frozen/'capture.dll';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*core,'bridge.c','-o',str(lib)],cwd=frozen,check=True)
 n=ctypes.CDLL(str(lib));n.cpu_1d3f4_capture.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_size_t)]
 ram=ctypes.create_string_buffer(0x4000);d=(ctypes.c_uint32*8)();layout=(ctypes.c_size_t*3)()
 assert n.cpu_1d3f4_capture(ctypes.create_string_buffer(dump),ram,d,layout)==0
 actors=(policy/'blocked-actors.bin').read_bytes();regs=(policy/'blocked-dregs.bin').read_bytes()
 assert ram.raw[0x5b0:0x5b0+9*0x10c]==actors and bytes(d)==regs
 assert dump[layout[1]:layout[1]+len(actors)]==actors and dump[layout[2]:layout[2]+32]==regs
 a=0x5b0+0x10c
 assert ram.raw[a+0x20:a+0x22]==bytes.fromhex('8001') and ram.raw[a+0xaf]==1
 assert ram.raw[0x5b0+0x26:0x5b0+0x2a]==bytes.fromhex('80000000')
 snapshot=bytes(d)+ram.raw;(HERE/'cpu_1d3f4_snapshot_host284.bin').write_bytes(snapshot)
 manifest={'frame':284,'slot':1,'phase':'1C150','dependency':'1D3F4','snapshot_sha256':hashlib.sha256(snapshot).hexdigest(),'layout_HOST':list(layout),'origin':str(evidence),'report_sha256':hashlib.sha256((policy/'report.json').read_bytes()).hexdigest(),'source_manifest_sha256':hashlib.sha256((evidence/'source-manifest.json').read_bytes()).hexdigest(),'archive_sha256':hashlib.sha256((evidence/'frozen-sources.zip').read_bytes()).hexdigest(),'dumps_sha256':report['dump_sha256'],'projection':'All9actors and D0-D7 byte equal to actual blocked state; export known globals/records, other arcadeRAMzero. Replay whole1C8F4 from this guard-state fixture without fabricated earlier state/registers. No native/trajectory claim.','rom_sha1':hashlib.sha1((ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()).hexdigest()}
 (HERE/'cpu_1d3f4_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('PASS HOST284 capture all9actors/D8; snapshot',manifest['snapshot_sha256'])
