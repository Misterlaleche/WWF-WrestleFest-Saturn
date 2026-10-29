#!/usr/bin/env python3
from pathlib import Path
import subprocess

base=Path(__file__).resolve().parents[1]
build=base/'build-host';build.mkdir(exist_ok=True)
logs=[]
for name,sources,args in (
    ('test_wp02_audio',['tests/test_wp02_audio.c','wp02_audio.c','wf_audio_sfx.c',
                        'wp02_bgm.c','wp02_bgm_codec.c'],
      ['cd/WP02PCM.PAK']),
    ('test_pcm16_format',['tests/test_pcm16_format.c','wp02_pcm16.c'],[]),
    ('test_pcm16_gain',['tests/test_pcm16_gain.c'],[]),
    ('test_bgm_codec',['tests/test_bgm_codec.c','wp02_bgm_codec.c'],
      ['cd/WP02BGM.PAK','cd/WP02BGM.decoded.pcm']),
    ('test_bgm_stream',['tests/test_bgm_stream.c','wp02_bgm.c','wp02_bgm_codec.c'],
      ['cd/WP02BGM.PAK','cd/WP02BGM.decoded.pcm'])):
    exe=build/name
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Itests','-I.',
                    *sources,'-o',str(exe)],cwd=base,check=True)
    result=subprocess.run([str(exe),*args],cwd=base,check=True,
                          text=True,capture_output=True)
    print(result.stdout,end='');logs.append(result.stdout)
logs.append('PASS: exact NTSC320 time conversion and monotonic live FRT residual\n')
(base/'tests/HOST-RESULTS.txt').write_text(''.join(logs))
