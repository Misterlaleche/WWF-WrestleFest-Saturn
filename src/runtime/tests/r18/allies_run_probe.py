#!/usr/bin/env python3
"""Build/run the regular HOST match with test-only ally observation wrappers."""
import argparse,os,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('output',type=Path);p.add_argument('--frames',type=int,default=5000);p.add_argument('--require-complete',action='store_true');p.add_argument('--attack',action='store_true');p.add_argument('--binary',type=Path)
args=p.parse_args();root=Path(__file__).resolve().parents[2];out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
assert not (out/'trace.csv').exists(),'Use a new evidence directory to preserve the previous observations'
makefile=(root/'Makefile').read_text()
core=re.search(r'^CORE=(.*)$',makefile,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',makefile,re.M)[1].split()
for source in ('src/wf_cpu_runtime.c','src/wf_r17_dispatch_1c03e.c'):core.remove(source)
binary=args.binary.resolve() if args.binary else out/'allies_probe'
if not args.binary:subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O2','-Iinclude',*core,*host,
                'tests/r18/allies_cpu_trace.c','tests/r18/allies_dispatch_trace.c','tests/r18/allies_trace.c',
                'tests/r17/direct_combat_probe.c','-o',str(binary)],cwd=root,check=True)
env={**os.environ,'WF_ALLIES_OUT':str(out),'WF_PROBE_FRAMES':str(args.frames),'WF_PROBE_REQUIRE_RESIDENT':'1'}
env.pop('WF_PROBE_ATTACK',None);env.pop('WF_PROBE_REQUIRE_COMPLETE',None)
if args.attack:env['WF_PROBE_ATTACK']='1'
if args.require_complete:env['WF_PROBE_REQUIRE_COMPLETE']='1'
with (out/'host.log').open('w') as log:
 result=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
print((out/'host.log').read_text()[-5000:])
raise SystemExit(result.returncode)
