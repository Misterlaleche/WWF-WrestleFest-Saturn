#!/usr/bin/env python3
"""Check scalar/bulk prefixes and byte aliases with identical initial GPU caches.

The video wrapper adds only a host fixture reset entry point. No test function
is compiled into the Saturn game and production sources are never modified.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=ROOT)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    source, out = a.source.resolve(), a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    video = source/'port/wf_jo_video.c'
    wrapper = out/'video-fixture.c'
    wrapper.write_text('#include '+json.dumps(str(video))+'\n'
                       'void wf_jo_test_reset_texture_cache(void) {\n'
                       ' texture_reset();\n'
                       ' memset((void *)&wf_jo_video_stats,0,sizeof wf_jo_video_stats);\n'
                       '}\n')
    # The execution sandbox uses ptrace; LeakSanitizer cannot run under it.
    # Address/undefined checks remain enabled. These fixtures allocate no heap.
    env = os.environ.copy();env['ASAN_OPTIONS'] = 'detect_leaks=0'
    report = {'scope': __doc__, 'source': str(source), 'commands': [], 'sources': {},
              'environment': {'ASAN_OPTIONS': env['ASAN_OPTIONS']}}
    inputs = [source/'port/wf_jo_backend.c', video, source/'port/wf_jo_texture_cache.h',
              source/'core/src/wf_saturn_memory.c', ROOT/'tests/test_jo_backend_consume.c',
              ROOT/'tests/test_jo_backend_consume_alias.c', wrapper]
    report['sources'] = {str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs}
    for name in ['test_jo_backend_consume', 'test_jo_backend_consume_alias']:
        exe = out/name
        cmd = ['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               '-fsanitize=address,undefined', '-DWF_JO_ENGINE', '-DWF_JO_TEST_RESET_CACHE',
               '-ffunction-sections', '-fdata-sections', '-I'+str(source/'core/include'),
               '-I'+str(source/'port'), str(ROOT/'tests'/(name+'.c')),
               str(source/'port/wf_jo_backend.c'), str(wrapper),
               str(source/'core/src/wf_saturn_memory.c'), '-Wl,--gc-sections', '-o', str(exe)]
        for argv, log in [(cmd, out/(name+'-build.log')), ([str(exe)], out/(name+'.log'))]:
            with log.open('w') as f:
                run = subprocess.run(argv, stdout=f, stderr=subprocess.STDOUT, env=env)
            report['commands'].append({'argv': argv, 'log': str(log), 'returncode': run.returncode})
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
            run.check_returncode()
        print((out/(name+'.log')).read_text().strip())
    report['passed'] = True
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
