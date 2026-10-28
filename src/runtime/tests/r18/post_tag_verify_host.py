"""Natural native-F8C TAG combat -> result -> interview -> next world5 F8C."""
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--captured-handoff',type=Path);p.add_argument('--profile',choices=('basic','idle','a600','pressure','punch','grapple','rush','pin'),default='pressure');p.add_argument('--frames',type=int,default=6000);args=p.parse_args()
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);assert not(out/'host.log').exists(),'Use a new evidence directory'
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
for source in('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c','src/wf_r17_dependencies.c','src/wf_r17_referee.c'):core.remove(source)
if 'src/wf_r18_post_tag.c'not in host:host.append('src/wf_r18_post_tag.c')
wrappers=['tests/r18/allies_cpu_trace.c','tests/r18/allies_dispatch_trace.c','tests/r18/allies_trace.c','tests/r18/transition_trace.c','tests/r18/referee_trace.c','tests/r18/full_flow_combat_probe.c']
binary=out/'post_tag_probe.exe'
with tempfile.TemporaryDirectory(prefix='wf-post-tag-natural-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 for name in(*wrappers,'tests/r18/frontend_driver_host.c','tests/r18/allies_trace.h','tests/r18/post_tag_host_tail.inc'):
  path=frozen/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,path)
 probe=frozen/'tests/r18/full_flow_combat_probe.c';s=probe.read_text();assert s.count('int main(int argc,char**argv)')==1
 s=s.replace('int main(int argc,char**argv)','static int post_tag_combat_main(int argc,char**argv)')
 result_begin=s.index('   uint32_t before=combat_waits;',s.index('wf_r18_tag_match_complete(&h.runtime)'))
 result_end=s.index('\n  }',result_begin)
 assert 'wf_r18_result_in_ring' in s[result_begin:result_end]
 s=s[:result_begin]+'   printf("MATCH COMPLETE frame=%u endpoint=0011B6 native post pending\\n",h.runtime.g.frame_counter);return 0;'+s[result_end:]
 join='combat_frontend(royal);combat_setup();';assert s.count(join)==1
 restore='for(unsigned side=0;side<2;side++)for(unsigned i=0;i<16;i++){combat_world.cache[side][i].source=read_be(h.ram+0x16d6+side*128u+i*8u,4);combat_world.cache[side][i].destination=read_be(h.ram+0x16da+side*128u+i*8u,4);}'
 s=s.replace(join,join+restore);s+='\n#include "post_tag_host_tail.inc"\n';probe.write_text(s)
 hashes={str(f.relative_to(frozen)):hashlib.sha256(f.read_bytes()).hexdigest()for f in sorted(frozen.rglob('*'))if f.is_file()}
 command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,*wrappers,'-o',str(binary)]
 subprocess.run(command,cwd=frozen,check=True)
 (out/'source-manifest.json').write_text(json.dumps({'sources_sha256':hashes,'compile_command':command,'test_only_transformations':['rename HOST probe main and append standalone post-tag composition','replace typed result segment with return at actual terminal11B6; native controller executes all result waits with IRQ','import original world cache from raw at gameplay handoff'],'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()},indent=2)+'\n')
manifest=None;env={**os.environ,'WF_ALLIES_OUT':str(out),'WF_TRACE_ALL_ACTORS':'1'}
if args.captured_handoff:
 cap=args.captured_handoff.resolve();manifest=json.loads((cap/'report.json').read_text())['handoff_capture'];assert manifest['valid']
 for name,key in [('handoff-after-f8c-ram.bin','ram_sha256'),('handoff-after-f8c-dregs.bin','dregs_sha256')]:assert hashlib.sha256((cap/name).read_bytes()).hexdigest()==manifest[key]
 env['WF_CAPTURED_HANDOFF_RAM']=str(cap/'handoff-after-f8c-ram.bin');env['WF_CAPTURED_HANDOFF_DREGS']=str(cap/'handoff-after-f8c-dregs.bin')
else:
 env.pop('WF_CAPTURED_HANDOFF_RAM',None);env.pop('WF_CAPTURED_HANDOFF_DREGS',None)
assert 1<=args.frames<=20000
command=[str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN',str(args.frames),args.profile,'0',str(out)]
with(out/'host.log').open('w')as f:r=subprocess.run(command,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT)
log=(out/'host.log').read_text();print(log[-7000:],end='',flush=True)
report={'status':'PASS'if r.returncode==0 else'ERROR','returncode':r.returncode,'captured_native_handoff':manifest,'command':command,'limits':['HOST device tape; no physical Saturn timing claim','Combat executes real controls and typed owners; retained raw16KB is overlaid by production scene_resume','Production original-record count accessor, not chain-expanded VDP commands','Starts11B6 at natural TAG terminal and executes all original result/scene waits with live frontend IRQ; no reset or seed change']}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(r.returncode)
