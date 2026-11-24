#!/usr/bin/env python3
"""Instrument the real cache validation count and two publication IRQ seams.

The unchanged baseline must pass semantics with --expected-checks2 and fail
its optimization goal with --expected-checks1. Only observation hooks are
inserted into generated build inputs; production files receive no test hooks.
"""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=ROOT);p.add_argument('--output',type=Path,required=True);p.add_argument('--expected-checks',type=int,default=1);a=p.parse_args()
 src=a.source.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 paths=['port/wf_jo_video.c','port/wf_jo_texture_cache.h','tests/test_jo_publish_prevalidated.c','tests/test_jo_pipeline.c','tests/wf_jo_video_test_platform.h','core/src/wf_saturn_memory.c']
 before={n:sha(src/n) for n in paths}
 cache=(src/'port/wf_jo_texture_cache.h').read_text();needle='static inline int wf_jo_cache_can_publish(const wf_jo_texture_cache_t *c)\n{';assert cache.count(needle)==1
 cache=cache.replace(needle,needle+'\n    ++wf_test_publish_checks;')
 (out/'wf_jo_texture_cache.h').write_text('extern unsigned wf_test_publish_checks;\n'+cache)
 video=(src/'port/wf_jo_video.c').read_text();needle='wf_status_t status=append_commands(commands,count);\n    if(status!=WF_OK)return status;';assert video.count(needle)==2
 video=video.replace(needle,'wf_test_submit_seam(0);\n    '+needle+'\n    wf_test_submit_seam(1);',1)
 (out/'instrumented_video.c').write_text(video)
 binary=out/'publish-video';command=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-int-to-pointer-cast','-fsanitize=address,undefined','-DWF_PLATFORM_SATURN=1','-DWF_JO_ENGINE=1','-DWF_JO_VIDEO_TEST=1','-I'+str(out),'-I'+str(src/'core/include'),'-I'+str(src/'port'),'-I'+str(src/'tests'),str(src/'tests/test_jo_publish_prevalidated.c'),str(src/'core/src/wf_saturn_memory.c'),'-o',str(binary)]
 r=subprocess.run(command,capture_output=True,text=True);(out/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');run=[str(binary),str(a.expected_checks)];r=subprocess.run(run,env=env,capture_output=True,text=True);(out/'run.log').write_text(r.stdout+r.stderr)
 assert before=={n:sha(src/n) for n in paths},'source changed'
 (out/'summary.json').write_text(json.dumps(dict(returncode=r.returncode,source_sha256=before,command=command,run=run,binary_sha256=sha(binary),output=r.stdout+r.stderr),indent=2)+'\n');print(r.stdout+r.stderr);raise SystemExit(r.returncode)
if __name__=='__main__':main()
