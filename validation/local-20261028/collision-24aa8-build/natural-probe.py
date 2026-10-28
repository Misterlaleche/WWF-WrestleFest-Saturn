from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path('src/runtime').resolve();out=Path('validation/local-20261028/collision-24aa8-natural-probe').resolve();out.mkdir(parents=True,exist_ok=True)
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split()
for s in ('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c','src/wf_r17_dependencies.c','src/wf_r17_referee.c'):core.remove(s)
wrappers=['tests/r18/'+s for s in ('allies_cpu_trace.c','allies_dispatch_trace.c','allies_trace.c','transition_trace.c','referee_trace.c','full_flow_combat_probe.c')]
with tempfile.TemporaryDirectory(prefix='wf-natural24aa8-',ignore_cleanup_errors=True) as td:
 t=Path(td)
 for name in ('src','include','generated'):shutil.copytree(ROOT/name,t/name)
 for name in wrappers+['tests/r18/frontend_driver_host.c','tests/r18/allies_trace.h']:
  dst=t/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dst)
 p=t/'src/wf_collision_dispatch.c';s=p.read_text();s='#include <stdio.h>\n'+s
 start=s.index(' case 0x06:\n  /* $24AA8');end=s.index(' case 0x07:',start);branch=s[start:end]
 old='  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));return WF_COLL_ACCEPT;'
 new='''  wf_set_u8(d,WF_A_LINK,(uint8_t)(wf_u8(d,WF_A_LINK)|0x80u));
  printf("TRACE_24AA8 frame=%u source=%u target=%u sid=%u tid=%u\\n",c->runtime->g.frame_counter,(unsigned)(c->src-c->runtime->actors),(unsigned)(d-c->runtime->actors),c->source_profile_id,c->target_profile_id);return WF_COLL_ACCEPT;'''
 assert branch.count(old)==1;s=s[:start]+branch.replace(old,new)+s[end:];p.write_text(s)
 exe=out/'instrumented.exe';cmd=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,*wrappers,'-o',str(exe)];subprocess.run(cmd,cwd=t,check=True)
 summary=[]
 for mode,profile in [(0,'grapple'),(0,'rush'),(1,'grapple'),(1,'rush')]:
  directory=out/('royal-' if mode else 'tag-');directory=Path(str(directory)+profile);directory.mkdir(exist_ok=True)
  env={**os.environ,'WF_ALLIES_OUT':str(directory),'WF_TRACE_ALL_ACTORS':'0'}
  args=[str(exe),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN','14000',profile,str(mode),str(directory)]
  p=subprocess.run(args,cwd=ROOT,env=env,capture_output=True,text=True);(directory/'host.log').write_text(p.stdout+p.stderr)
  hits=[line for line in p.stdout.splitlines() if line.startswith('TRACE_24AA8')];summary.append({'mode':'Royal' if mode else 'TAG','profile':profile,'returncode':p.returncode,'hits':hits})
  print(mode,profile,'exit',p.returncode,'hits',len(hits),flush=True)
 (out/'report.json').write_text(json.dumps({'cases':summary,'scope':'HOST instrumented debug build; no physical Saturn timing or hardware validation'},indent=2)+'\n')
