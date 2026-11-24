#!/usr/bin/env python3
"""Compile and exercise the real Jo audio adapter against a fake hardware edge."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import os
import struct
import subprocess
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser()
    inputs=p.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--rom-zip',type=Path)
    inputs.add_argument('--assets',type=Path,help='Packaged CD assets, including SFX.DAT and the 18 WAVs')
    p.add_argument('--source',type=Path,default=ROOT)
    p.add_argument('--sanitize',action='store_true')
    a=p.parse_args()
    source=a.source.resolve();assets=(a.assets or source/'cd').resolve()
    if a.rom_zip:
        s=importlib.util.spec_from_file_location('build_audio',source/'tools/build_audio.py')
        m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
        with zipfile.ZipFile(a.rom_zip) as z:
            data=m.build_sfx(z.read('31a11-2.ic42'),z.read('31j10.ic73'))
    else:
        # Independent reference decoder plus its frozen PCM hash retains the
        # original sample oracle when the available input is the packaged CD.
        s=importlib.util.spec_from_file_location('pcm_reference',source/'tools/audio_reference/generate_resources.py')
        m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
        encoded=(assets/'SFX.DAT').read_bytes();pcm=bytearray();seen=set()
        assert encoded[:4]==b'WFSX'
        for row in range(1,116):
            if row==74:continue
            at,size=struct.unpack_from('>IH',encoded,32+row*8)
            if at in seen:continue
            assert at+size<=len(encoded);seen.add(at)
            pcm+=bytes(-len(pcm)%32);pcm+=m.decode(encoded[at:at+size])
        assert len(seen)==95 and hashlib.sha256(pcm).hexdigest()==m.PCM_SHA256
        data={'pack':encoded,'pcm16be':bytes(pcm)}
    with tempfile.TemporaryDirectory(prefix='wf-audio-test-') as folder:
        work=Path(folder)
        (work/'SFX.DAT').write_bytes(data['pack'])
        (work/'PCM16BE.PAK').write_bytes(data['pcm16be'])
        cmd=['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-DWF_JO_AUDIO_HOST',
            '-DWF_JO_ENGINE','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
            '-I'+str(source/'port'),'-I'+str(source/'core/include'),'-I'+str(source/'core/src'),
            '-I'+str(source/'sdk/jo_engine')]
        if a.sanitize:
            cmd+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        sources=['tests/test_audio.c','tests/test_audio_core_hooks.c','port/wf_jo_audio.c',
                 'core/src/wf_r18_ring.c','core/src/wf_r18_video.c']
        subprocess.run([*cmd,*(str((ROOT if f.startswith('tests/') else source)/f) for f in sources),'-o',str(work/'test_audio')],check=True)
        env=dict(os.environ,WF_TEST_CD=str(assets))
        if a.sanitize:
            # This workspace runs under ptrace; LeakSanitizer cannot enumerate
            # threads here. Address/undefined-behavior instrumentation stays on.
            env.setdefault('ASAN_OPTIONS','detect_leaks=0')
        subprocess.run([str(work/'test_audio'),str(work/'SFX.DAT'),str(work/'PCM16BE.PAK')],check=True,env=env)

if __name__=='__main__':main()
