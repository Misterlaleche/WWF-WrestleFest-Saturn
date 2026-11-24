#!/usr/bin/env python3
"""Check the real chunk decoder and audio service against exact PCM identities."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT)
    parser.add_argument('--pack',type=Path,default=ROOT/'cd/SFX.DAT')
    parser.add_argument('--assets',type=Path,help='WAV assets for the real adapter header boundary (default: source/cd)')
    parser.add_argument('--output',type=Path,default=ROOT/'build/audio-decoder')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args()
    source=args.source.resolve();pack=args.pack.resolve();out=args.output.resolve()
    assets=(args.assets or source/'cd').resolve()
    out.mkdir(parents=True,exist_ok=True)
    reference=ROOT/'tools/audio_reference/generate_resources.py'
    spec=importlib.util.spec_from_file_location('audio_reference',reference)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    encoded=pack.read_bytes();pcm=bytearray();seen=set()
    for row in range(1,116):
        if row==74:continue
        at,size=struct.unpack_from('>IH',encoded,32+row*8)
        if at in seen:continue
        seen.add(at);pcm+=bytes(-len(pcm)%32)
        pcm+=module.decode(encoded[at:at+size])
    assert len(seen)==95 and hashlib.sha256(pcm).hexdigest()==module.PCM_SHA256
    expected=out/'PCM16BE.PAK';expected.write_bytes(pcm)
    flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-DWF_JO_AUDIO_HOST',
           '-DWF_JO_ENGINE','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    if args.sanitize:
        flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    flags+=['-I'+str(source/p) for p in ['port','core/include','core/src','sdk/jo_engine']]
    inputs=[source/'port/wf_jo_audio.c',source/'port/wf_jo_audio.h',
            source/'core/src/wf_r18_ring.c',source/'core/src/wf_r18_video.c',
            ROOT/'tests/test_audio_decoder.c',ROOT/'tests/test_audio.c',
            ROOT/'tests/test_audio_core_hooks.c',ROOT/'tests/audio_header_stub.h',
            source/'port/wf_jo_music_headers.h',reference,Path(__file__).resolve(),pack]
    inputs += [assets/f'MU{cue:02X}.WAV' for cue in range(1,19)]
    identities={str(p):sha(p) for p in inputs}
    commands=[]
    env=dict(os.environ,WF_TEST_CD=str(assets))
    if args.sanitize:env['ASAN_OPTIONS']='detect_leaks=0'
    def run(argv,name):
        log=out/(name+'.log')
        with log.open('w') as stream:
            result=subprocess.run(argv,cwd=source,env=env,stdout=stream,stderr=subprocess.STDOUT)
        commands.append(dict(argv=list(map(str,argv)),returncode=result.returncode,log=log.name))
        if result.returncode:print(log.read_text())
        result.check_returncode()
    cc=os.environ.get('CC','cc')
    run([cc,*flags,str(ROOT/'tests/test_audio_decoder.c'),'-o',str(out/'decoder')],'compile-decoder')
    run([str(out/'decoder'),str(pack)],'decoder')
    service=[ROOT/'tests/test_audio.c',ROOT/'tests/test_audio_core_hooks.c',
             source/'port/wf_jo_audio.c',source/'core/src/wf_r18_ring.c',source/'core/src/wf_r18_video.c']
    run([cc,*flags,*map(str,service),'-o',str(out/'service')],'compile-service')
    run([str(out/'service'),str(pack),str(expected)],'service')
    changed=[str(p) for p in inputs if sha(p)!=identities[str(p)]]
    assert not changed,changed
    report=dict(status='PASS',scope='Host PCM, upload-boundary state and complete audio-service behavior; not Saturn timing',
                sanitize=args.sanitize,inputs=identities,commands=commands,
                pcm_sha256=sha(expected),unique_samples=len(seen),sources_changed=changed,
                decoder=(out/'decoder.log').read_text().strip(),service=(out/'service.log').read_text().strip())
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(report['decoder']);print(report['service'])

if __name__=='__main__':main()
