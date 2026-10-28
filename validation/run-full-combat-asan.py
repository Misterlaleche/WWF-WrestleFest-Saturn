"""Compile the production full-flow combat probe with ASan/UBSan and assertions."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--profile',default='basic',choices=['idle','basic','a600','pressure','punch','grapple']);p.add_argument('--royal',action='store_true');p.add_argument('--frames',type=int,default=6000);a=p.parse_args()
root=Path(__file__).resolve().parents[1]/'src/runtime';out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
make=(root/'Makefile').read_text();sources=[]
for key in('CORE','R18_HOST'):sources+=re.search(r'^'+key+r'=(.*)$',make,re.M)[1].split()
for source in('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c','src/wf_r17_dependencies.c','src/wf_r17_referee.c'):sources.remove(source)
sources+=['tests/r18/'+s for s in('allies_cpu_trace.c','allies_dispatch_trace.c','allies_trace.c','transition_trace.c','referee_trace.c','full_flow_combat_probe.c')]
flags=['-std=c11','-O1','-g','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','-fsanitize=address,undefined','-fno-omit-frame-pointer']
def compile_one(source):
 obj=out/(source.replace('/','_')+'.o');r=subprocess.run(['cc',*flags,'-c',source,'-o',str(obj)],cwd=root,text=True,capture_output=True)
 (out/(obj.name+'.log')).write_text(r.stdout+r.stderr)
 if r.returncode:raise RuntimeError(source+'\n'+r.stdout+r.stderr)
 return obj
with ThreadPoolExecutor(max_workers=4)as pool:objects=list(pool.map(compile_one,sources))
binary=out/'full-combat-asan';subprocess.run(['cc',*flags,*map(str,objects),'-o',str(binary)],cwd=root,check=True)
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1','UBSAN_OPTIONS':'halt_on_error=1','WF_ALLIES_OUT':str(out)}
# Trace wrappers require their output directory even without all-actor mode.
# Keep their default two-slot sampling; production owners remain unchanged.
env.pop('WF_TRACE_ALL_ACTORS',None)
cmd=[str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN',str(a.frames),a.profile,str(int(a.royal)),str(out)]
r=subprocess.run(cmd,cwd=root,env=env,text=True,capture_output=True)
(out/'host.log').write_text(r.stdout+r.stderr);print(r.stdout,end='');print(r.stderr,end='')
report={'status':'PASS'if r.returncode==0 else'FAIL','exit':r.returncode,'command':cmd,'sanitizers':['address','undefined'],'assertions':'enabled','sources':{s:hashlib.sha256((root/s).read_bytes()).hexdigest()for s in sources}}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(r.returncode)
