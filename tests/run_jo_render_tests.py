#!/usr/bin/env python3
"""Portable renderer gates; baseline source copies are byte-exact B12 inputs.

Run from the source ZIP with a Linux C compiler and Python standard library.
No git, SDK compiler, historical build or external checkout is required.
The actual-SDK asynchronous dual-CPU gate is a separate fixture.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import zlib

ROOT=Path(__file__).resolve().parent.parent
CASES=('cache','crop','parser','consume','pixels','pipeline','batch')

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT)
    parser.add_argument('--output',type=Path,default=ROOT/'build/jo-render-tests')
    parser.add_argument('--cc',default=os.environ.get('WF_TEST_CC',os.environ.get('CC','cc')))
    parser.add_argument('--case',action='append',choices=CASES)
    args=parser.parse_args();root=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    cc=shlex.split(args.cc);assert cc
    base=[*cc,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
          '-I'+str(root/'core/include'),'-I'+str(root/'port')]
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
    results=[];tracked={}
    for folder in ('port','core/include'):
        for path in (root/folder).rglob('*.h'):tracked[str(path.relative_to(root))]=sha(path)
    def compile_run(name,sources,flags=(),extra=(),run_args=(),defines=()):
        paths=[root/s for s in sources]
        for p in paths:tracked[str(p.relative_to(root))]=sha(p)
        command=[*base,*flags,*defines,*map(str,paths),*extra,'-o',str(out/name)]
        subprocess.run(command,cwd=root,check=True,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        run=subprocess.run([str(out/name),*map(str,run_args)],cwd=root,check=True,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (out/(name+'.log')).write_text(run.stdout)
        results.append(dict(name=name,build_command=command,output=run.stdout,returncode=run.returncode))
        print(run.stdout,end='')
    def reference(name,source,defines):
        path=root/source;tracked[source]=sha(path);target=out/(name+'.o')
        subprocess.run([*base,*defines,'-c',str(path),'-o',str(target)],cwd=root,check=True,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        return str(target)
    selected=args.case or CASES
    try:
        if 'cache' in selected:compile_run('cache',['tests/test_jo_texture_cache.c'])
        if 'crop' in selected:
            ref=reference('crop-reference','tests/render_reference/b12_wf_jo_video.c',[
                '-Dwf_jo_crop_tile=wf_b12_crop_tile','-Dwf_jo_bank_reusable=wf_b12_bank_reusable',
                '-Dwf_jo_texture_offset=wf_b12_texture_offset','-Dwf_jo_sprite_build=wf_b12_sprite_build',
                '-Dwf_jo_video_stats=wf_b12_video_stats'])
            pack=(root/'cd/SPRITES.WFP').read_bytes();assert pack[:4]==b'WFSZ'
            version,block,raw_bytes,count,data_at,pack_bytes,index_adler=struct.unpack_from('>7I',pack,4)
            assert version==1 and block in (4096,8192) and raw_bytes==8388608 and count*block==raw_bytes
            assert len(pack)==pack_bytes and data_at>=32+4*(count+1) and data_at%32==0
            assert zlib.adler32(pack[32:data_at])&0xffffffff==index_adler
            offsets=struct.unpack_from('>'+str(count+1)+'I',pack,32)
            assert offsets[0]==data_at and offsets[-1]==len(pack)
            raw=bytearray()
            for lo,hi in zip(offsets,offsets[1:]):
                assert lo<hi;decoder=zlib.decompressobj();part=decoder.decompress(pack[lo:hi])
                assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and len(part)==block
                raw+=part
            assert hashlib.sha256(raw).hexdigest()=='130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31'
            raw_path=out/'b12-sprite-pixels.bin';raw_path.write_bytes(raw)
            compile_run('crop',['tests/test_jo_crop_differential.c','port/wf_jo_video.c'],extra=[ref],run_args=[raw_path])
        if 'parser' in selected:
            ref=reference('parser-reference','tests/render_reference/b12_wf_r18_metasprite.c',[
                '-DWF_JO_ENGINE','-Dwf_r18_metasprite_emit=wf_b12_metasprite_emit',
                '-Dwf_r18_metasprite_cache_reset=wf_b12_metasprite_cache_reset'])
            compile_run('parser',['tests/test_jo_metasprite_replay.c','core/src/wf_r18_metasprite.c'],
                        flags=['-DWF_JO_ENGINE'],extra=[ref],run_args=[root/'cd/ROM68K.BIN'])
        for case in ('consume','batch'):
            if case in selected:
                compile_run(case,['tests/test_jo_backend_'+case+'.c','port/wf_jo_backend.c','port/wf_jo_video.c','core/src/wf_saturn_memory.c'],
                    flags=['-DWF_JO_ENGINE','-ffunction-sections','-fdata-sections'],extra=['-Wl,--gc-sections'])
        if 'pixels' in selected:compile_run('pixels',['tests/test_jo_video.c','port/wf_jo_video.c'])
        if 'pipeline' in selected:compile_run('pipeline',['tests/test_jo_pipeline.c','port/wf_jo_video.c','core/src/wf_saturn_memory.c'],
            flags=['-Wno-int-to-pointer-cast','-DWF_PLATFORM_SATURN=1','-DWF_JO_ENGINE=1','-DWF_JO_VIDEO_TEST=1','-I'+str(root/'tests')])
    except subprocess.CalledProcessError as error:
        print(error.stdout.decode() if isinstance(error.stdout,bytes) else error.stdout or '')
        raise
    for name,digest in tracked.items():assert sha(root/name)==digest,name
    (out/'summary.json').write_text(json.dumps(dict(scope=__doc__,source=str(root),tests=results,source_sha256=tracked),indent=2)+'\n')

if __name__=='__main__':main()
