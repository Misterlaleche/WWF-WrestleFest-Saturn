#!/usr/bin/env python3
"""Exercise the exact loading/fail functions with a recording text surface."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, subprocess, os, re
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT
spec=importlib.util.spec_from_file_location('protocol',REPO/'tests/test_cd_protocol.py')
protocol=importlib.util.module_from_spec(spec);spec.loader.exec_module(protocol)
p=argparse.ArgumentParser();p.add_argument('--baseline',action='store_true');p.add_argument('--source-dir',type=Path);p.add_argument('--build-dir',type=Path);args=p.parse_args()
base=args.source_dir or ROOT
main=(base/'port/main.c').read_text();header=(ROOT/'sdk/jo_engine/jo/fs.h').read_text()
out=args.build_dir or ROOT/'build'/('cd-loading-ui-red' if args.baseline else 'cd-loading-ui');out.mkdir(parents=True,exist_ok=True)
kind=re.search(r'typedef struct\s*\{[^}]+\}\s*jo_fs_cd_diagnostics;',header)[0]
body='\n'.join(protocol.function(main,name) for name in
 (['loading_cd_summary'] if not args.baseline else [])+['loading_progress','fail'])
program=(ROOT/'tests/cd_loading_ui.c.in').read_text().replace('@TYPE@',kind).replace('@BODY@',body)
c=out/'actual-ui.c';c.write_text(program);binary=out/'ui'
cmd=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
 '-fsanitize=address,undefined','-fno-omit-frame-pointer',str(c),'-o',str(binary)]
run=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(run.stdout+run.stderr)
if run.returncode:print(run.stderr);run.check_returncode()
report={'status':'RUNNING','command':cmd,'runs':[]}
for case in ['visible','hidden','fail','wide']:
 r=subprocess.run([str(binary),case],text=True,capture_output=True,
  env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1'))
 expected_red=args.baseline and case!='hidden'
 assert(r.returncode!=0)==expected_red,(case,r.returncode,r.stdout,r.stderr)
 report['runs'].append({'case':case,'returncode':r.returncode,'output':r.stdout+r.stderr});print(case,'EXPECTED_RED' if expected_red else 'PASS')
report.update(status='EXPECTED_RED' if args.baseline else 'PASS',source_sha256={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in [base/'port/main.c',ROOT/'sdk/jo_engine/jo/fs.h',Path(__file__),ROOT/'tests/cd_loading_ui.c.in']})
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
