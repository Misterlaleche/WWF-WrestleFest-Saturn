#!/usr/bin/env python3
"""Run actual SDK command/presentation functions against delayed host devices.

Only SH-2 assembly helpers, MMIO/cache-through addresses and empty busy waits
are substituted. Scheduling predicates and the C sort/DMA/presentation bodies
come directly from the selected frozen SDK sources. ucontext provides two
cooperative CPU stacks, so this target-ownership test uses UBSan and guarded
stacks; the separate port lifecycle test runs ASan/UBSan without coroutines.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

B=Path(__file__).resolve().parent
REQUIRED_SOURCE_FILES=(
    'sdk/jo_engine/vdp1_command_pipeline.c',
    'sdk/jo_engine/core.c',
    'port/wf_jo_video.c',
    'port/wf_jo_backend.c',
    'core/src/wf_saturn_memory.c',
)

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def source_root(explicit):
    # Installed in tests/: its parent is the source ZIP root. An explicit
    # source also lets this same fixture audit a separate frozen source tree.
    candidates=[explicit.expanduser().resolve()] if explicit else [B.parent,B.parent.parent,Path.cwd().resolve()]
    for root in candidates:
        if all((root/name).is_file() for name in REQUIRED_SOURCE_FILES):return root
    raise ValueError('Cannot locate SDK/port/core source files; pass --source SOURCE_ROOT')

def source_manifest(root):
    # Record portable names, including the fixture and headers actually made
    # available to the compiler. No git metadata or prior build is required.
    files={root/name for name in REQUIRED_SOURCE_FILES}
    for folder in ('core/include','port','sdk/jo_engine'):
        files.update(p for p in (root/folder).rglob('*')
                     if p.is_file() and p.suffix in ('.h','.inc'))
    result={'source/'+str(p.relative_to(root)):sha(p) for p in sorted(files)}
    for name in ('sdk_test.py','sdk_device_test.c','wf_jo_video_test_platform.h'):
        result['fixture/'+name]=sha(B/name)
    return result

def function(source,name):
    # A definition starts at the beginning of a non-comment declaration line.
    match=re.search(r'^\w[^;{}\n]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source,re.M)
    assert match,name
    start=match.start();at=source.index('{',match.start());depth=0
    masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m:' '*len(m.group()),source,flags=re.S)
    for end in range(at,len(source)):
        depth+=(masked[end]=='{')-(masked[end]=='}')
        if depth==0:return source[start:end+1]+'\n'
    raise AssertionError(name)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,
                        help='source ZIP/checkout root; default: source root above tests/, then current directory')
    parser.add_argument('--output',type=Path,default=Path('build/test-jo-sdk-pipeline'),
                        help='generated files and evidence directory, relative to current directory')
    parser.add_argument('--cc',default=os.environ.get('CC','cc'),
                        help='host compiler command; default: CC environment variable or cc')
    parser.add_argument('--mode',choices=['normal','overlap','drain','edges','lost-frame'],default='normal')
    parser.add_argument('--frames',type=int,default=1200)
    parser.add_argument('--seed',type=int,default=1)
    parser.add_argument('--preparation',action='store_true',
                        help='Reuse immutable preparation across fresh frames with changing commands and delayed devices')
    parser.add_argument('--private-consume',action='store_true',
                        help='Also consume restored preparations through the one-use private all-hit path')
    args=parser.parse_args()
    args.preparation |= args.private_consume
    if not 0<args.frames<12000:parser.error('--frames must be between 1 and 11999')
    if not 0<args.seed<=0xffffffff:parser.error('--seed must be between 1 and 4294967295')
    compiler=shlex.split(args.cc)
    if not compiler:parser.error('--cc must name a host C compiler')
    try:
        v=source_root(args.source)
        before=source_manifest(v)
    except (ValueError,OSError) as error:
        parser.error(str(error))
    out=args.output.expanduser().resolve();out.mkdir(parents=True,exist_ok=True)
    pipeline=v/'sdk/jo_engine/vdp1_command_pipeline.c'
    core=v/'sdk/jo_engine/core.c'
    text=pipeline.read_text().split('/** @brief A command as 8 words of 32 bits')[0]
    text=re.sub(r'^#include[^\n]*\n','',text,flags=re.M)
    text,n=re.subn(r'__asm__\(\n.*?\n\);','/* SH-2 assembly: use the SDK C sort/DMA path. */',text,flags=re.S)
    assert n==2
    text,waits=re.subn(r'^([ \t]*);[ \t]*$',r'\1host_poll();',text,flags=re.M)
    assert waits>=7
    # Private getters declared by SDK headers remain their original bodies.
    (out/'sdk_pipeline.inc').write_text(text)
    c=core.read_text()
    candidate='jo_core_set_frame_overlap' in c
    functions=['jo_core_set_frame_overlap','jo_core_synch_frame','jo_core_synch_callbacks'] if candidate else []
    functions+=['__jo_core_synch']
    body='\n'.join(function(c,name) for name in functions)
    # The one-shot flag is the only extra file-scope SDK state in this variant.
    if candidate:
        declaration=re.search(r'^static[^;\n]*__jo_core_frame_overlap[^;]*;',c,re.M)
        assert declaration
        body=declaration.group()+'\n'+body
    (out/'sdk_core.inc').write_text(body)
    flags=['-std=gnu11','-O1','-g','-fms-extensions','-Wall','-Wextra','-Wno-unused-function',
           '-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-Wno-unused-variable',
           '-fsanitize=undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie',
           '-ffunction-sections','-fdata-sections','-DWF_PLATFORM_SATURN=1',
           '-DWF_JO_ENGINE=1','-DWF_JO_VIDEO_TEST=1','-DJO_COMPILE_WITH_DUAL_CPU_SUPPORT=1',
           '-DWF_JO_DUAL_COMMAND_BUFFERS=1','-DTEST_CANDIDATE='+str(int(candidate)),
           '-DTEST_PREPARATION='+str(int(args.preparation)),
           '-DTEST_PRIVATE_CONSUME='+str(int(args.private_consume)),
           '-I'+str(out),'-I'+str(v/'core/include'),'-I'+str(v/'port'),
           '-I'+str(B),'-I'+str(v/'sdk/jo_engine')]
    binary=out/'sdk-device-test'
    sources=[B/'sdk_device_test.c',v/'port/wf_jo_video.c',v/'port/wf_jo_backend.c',
             v/'core/src/wf_saturn_memory.c']
    command=[*compiler,*flags,*map(str,sources),'-Wl,--gc-sections','-o',str(binary)]
    if args.preparation:command+=['-Wl,--wrap=wf_jo_video_texture_restore_preparation']
    with (out/'build.log').open('w') as log:
        built=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    if built.returncode:
        print((out/'build.log').read_text()[-14000:]);raise SystemExit(built.returncode)
    name=f'{args.mode}-{args.seed}-{args.frames}'+('-preparation' if args.preparation else '')
    if args.private_consume:name+='-consume'
    run_command=[str(binary),args.mode,str(args.frames),str(args.seed)]
    with(out/(name+'.log')).open('w')as log:
        run=subprocess.run(run_command,stdout=log,stderr=subprocess.STDOUT)
    after=source_manifest(v)
    assert before==after
    result=dict(scope=__doc__,source=str(v),fixture=str(B),output_directory=str(out),
                mode=args.mode,frames=args.frames,seed=args.seed,preparation=args.preparation,
                private_consume=args.private_consume,
                candidate=candidate,build_command=command,run_command=run_command,
                returncode=run.returncode,source_sha256=before,source_changed=False,
                instrumentation=dict(assembly_blocks_replaced=n,busy_wait_yields=waits),
                output=(out/(name+'.log')).read_text())
    (out/(name+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(result['output'][-5000:])
    raise SystemExit(run.returncode)

if __name__=='__main__':main()
