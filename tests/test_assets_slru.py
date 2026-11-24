#!/usr/bin/env python3
"""Real sprite/scene bytes and independent cache-policy state under sanitizers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/cache-slru-tests')
    parser.add_argument('--assets-root',type=Path,default=ROOT)
    args=parser.parse_args();build=args.build_dir.resolve();build.mkdir(parents=True,exist_ok=True)
    assets=args.assets_root.resolve()
    for name in ('SPRITES.WFP','SCENES.WFP'):shutil.copyfile(assets/'cd'/name,build/name)
    for name in ('ROM68K.BIN','SPRITES.PAK','R18RING.BIN'):shutil.copyfile(assets/'source_assets'/name,build/name)
    inputs=[Path(__file__),ROOT/'tests/test_assets_slru.c',ROOT/'port/wf_jo_assets.c',
            ROOT/'port/wf_jo_assets.h',ROOT/'tests/native/assets_worker_host.c']
    sources=[ROOT/'port/zlib'/(name+'.c') for name in ('inflate','inffast','inftrees','adler32','zutil')]
    inputs+=sources+list((ROOT/'port/zlib').glob('*.h'))
    report=dict(status='RUNNING',commands=[],source_sha256={str(p):sha(p) for p in inputs},
        input_sha256={name:sha(build/name) for name in ('SPRITES.WFP','SCENES.WFP','ROM68K.BIN','SPRITES.PAK','R18RING.BIN')})
    def run(command,label):
        command=[str(p) for p in command]
        result=subprocess.run(command,cwd=build,text=True,capture_output=True,
            env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1'))
        log=build/(label+'.log');log.write_text(result.stdout+result.stderr)
        report['commands'].append(dict(argv=command,cwd=str(build),returncode=result.returncode,log=str(log)))
        if result.returncode:print(log.read_text(),flush=True)
        result.check_returncode();return result.stdout
    try:
        flags=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-implicit-fallthrough',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
            '-DWF_JO_ASSETS_HOST','-DZ_SOLO','-DNO_GZIP','-I'+str(ROOT/'port'),'-I'+str(ROOT/'port/zlib')]
        for mode,extra in [('serial',[]),('worker',['-DWF_JO_ASSETS_TEST_WORKER'])]:
            worker=[ROOT/'tests/native/assets_worker_host.c'] if extra else []
            binary=build/('slru-'+mode)
            run([*flags,*extra,ROOT/'tests/test_assets_slru.c',*sources,*worker,'-o',binary],'compile-'+mode)
            print(run([binary,build/'SPRITES.WFP',build/'ROM68K.BIN',build/'SPRITES.PAK'],'run-'+mode),end='',flush=True)
        report['status']='PASS'
    except Exception as error:
        report['status']='FAIL';report['error']=repr(error);raise
    finally:
        report['changed_sources']=[str(p) for p in inputs if sha(p)!=report['source_sha256'][str(p)]]
        if report['changed_sources']:report['status']='FAIL'
        (build/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    assert not report['changed_sources']


if __name__=='__main__':main()
