#!/usr/bin/env python3
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=ROOT);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();src=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
text=a.reference.read_text();text=re.sub(r'\bwf_jo_\w+',lambda m:'reference_'+m.group(),text);text=re.sub(r'\bWF_JO_\w+',lambda m:'REFERENCE_'+m.group(),text);(out/'reference_cache.h').write_text(text)
binary=out/'publish-state';cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(out),'-I'+str(src/'port'),str(src/'tests/test_jo_publish_state.c'),'-o',str(binary)]
before=sha(src/'port/wf_jo_texture_cache.h');r=subprocess.run(cmd,capture_output=True,text=True);(out/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
r=subprocess.run([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),capture_output=True,text=True);(out/'run.log').write_text(r.stdout+r.stderr);assert sha(src/'port/wf_jo_texture_cache.h')==before
(out/'summary.json').write_text(json.dumps(dict(returncode=r.returncode,source_sha256=before,reference_sha256=sha(a.reference),binary_sha256=sha(binary),command=cmd,output=r.stdout+r.stderr),indent=2)+'\n');print(r.stdout+r.stderr);raise SystemExit(r.returncode)
