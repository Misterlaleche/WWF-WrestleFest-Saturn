#!/usr/bin/env python3
"""Build/run the regular HOST match with test-only ally observation wrappers."""
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--frames',type=int,default=5000);p.add_argument('--require-complete',action='store_true');p.add_argument('--attack',action='store_true');p.add_argument('--binary',type=Path);p.add_argument('--all-actors',action='store_true');p.add_argument('--trace-tiles',action='store_true')
args=p.parse_args();root=Path(__file__).resolve().parents[2];out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
assert not (out/'trace.csv').exists(),'Use a new evidence directory to preserve the previous observations'
makefile=(root/'Makefile').read_text()
core=re.search(r'^CORE=(.*)$',makefile,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',makefile,re.M)[1].split()
for source in ('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c','src/wf_r17_dependencies.c','src/wf_r17_referee.c'):core.remove(source)
binary=args.binary.resolve() if args.binary else out/'allies_probe'
wrappers=['tests/r18/allies_cpu_trace.c','tests/r18/allies_dispatch_trace.c','tests/r18/allies_trace.c','tests/r17/direct_combat_probe.c','tests/r18/transition_trace.c','tests/r18/referee_trace.c']
if args.trace_tiles:
 host.remove('src/wf_r18_present.c');wrappers.append('tests/r18/present_tile_trace.c')
if not args.binary:
 # Keep concurrent owner work out of a compiler's sequential source reads.
 # The manifest describes exactly the frozen files used for this binary.
 with tempfile.TemporaryDirectory(prefix='wf-match-build-') as temp:
  frozen=Path(temp)
  for name in ('src','include','generated'):shutil.copytree(root/name,frozen/name)
  for name in [*wrappers,'tests/r18/allies_trace.h']:
   target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(root/name,target)
  sources={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(frozen.rglob('*')) if p.is_file()}
  command=['cc','-std=c11','-Wall','-Wextra','-Werror','-O2','-Iinclude',*core,*host,*wrappers,'-o',str(binary)]
  subprocess.run(command,cwd=frozen,check=True)
  (out/'source-manifest.json').write_text(json.dumps({'sources_sha256':sources,'compile_command':command,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()},indent=2)+'\n')
env={**os.environ,'WF_ALLIES_OUT':str(out),'WF_PROBE_FRAMES':str(args.frames),'WF_PROBE_REQUIRE_RESIDENT':'1'}
env.pop('WF_PROBE_ATTACK',None);env.pop('WF_PROBE_REQUIRE_COMPLETE',None)
env.pop('WF_TRACE_ALL_ACTORS',None)
env.pop('WF_PROBE_TILE_DEMAND',None)
if args.trace_tiles:env['WF_PROBE_TILE_DEMAND']=str(out/'tile-demands.txt')
if args.all_actors:env['WF_TRACE_ALL_ACTORS']='1'
if args.attack:env['WF_PROBE_ATTACK']='1'
if args.require_complete:env['WF_PROBE_REQUIRE_COMPLETE']='1'
with (out/'host.log').open('w') as log:
 result=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
print((out/'host.log').read_text()[-5000:])
raise SystemExit(result.returncode)
