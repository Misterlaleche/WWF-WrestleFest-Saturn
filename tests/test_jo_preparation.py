#!/usr/bin/env python3
"""Exercise preparation reuse without modifying production sources for instrumentation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=ROOT)
    p.add_argument('--output', type=Path, default=ROOT/'build/preparation-tests')
    p.add_argument('--baseline', action='store_true')
    p.add_argument('--without-private-consume', action='store_true')
    a = p.parse_args()
    root, out = a.source.resolve(), a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    wrapper = out/'video-fixture.c'
    fields = 'entries index valid recent pinned ready frame repeat allocated hand frame_count repeat_count building repeat_valid'.split()
    wrapper.write_text('#include '+json.dumps(str(root/'port/wf_jo_video.c'))+'\n'
        'void wf_jo_test_reset_texture_cache(void){texture_reset();memset((void *)&wf_jo_video_stats,0,sizeof wf_jo_video_stats);}\n'
        'uint64_t wf_jo_test_cache_digest(void){uint64_t h=UINT64_C(14695981039346656037);\n'+
        ''.join('{const uint8_t *p=(const uint8_t *)&texture_cache.'+f+';for(unsigned i=0;i<sizeof texture_cache.'+f+';i++)h=(h^p[i])*UINT64_C(1099511628211);}\n' for f in fields)+
        'return h;}\n')
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    flags = ['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
             '-fsanitize=address,undefined', '-DWF_JO_ENGINE', '-ffunction-sections',
             '-fdata-sections', '-I'+str(root/'port'), '-I'+str(root/'core/include')]
    if a.baseline:
        flags.append('-DTEST_BASELINE')
    if a.without_private_consume:
        flags.append('-DTEST_NO_PREPARED_CONSUME')
    results = []
    sources = [root/'port/wf_jo_backend.c', root/'port/wf_jo_video.c',
               root/'port/wf_jo_texture_cache.h', root/'port/wf_jo_video.h',
               ROOT/'tests/test_jo_preparation_cache.c', ROOT/'tests/test_jo_preparation_backend.c']
    report = dict(scope=__doc__, source=str(root), source_sha256={str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in sources}, tests=results)
    cases = [
        ('cache', [ROOT/'tests/test_jo_preparation_cache.c'], []),
        ('backend', [ROOT/'tests/test_jo_preparation_backend.c', root/'port/wf_jo_backend.c', wrapper,
                     root/'core/src/wf_saturn_memory.c'], ['-Wl,--gc-sections', '-Wl,--wrap=wf_jo_video_texture_protect',
                                                         '-Wl,--wrap=wf_jo_video_texture_restore_preparation'])]
    if not a.baseline:
        assets = out/'assets-fixture.c'
        assets.write_text('#include '+json.dumps(str(root/'port/wf_jo_assets.c'))+'\n'
            'void wf_jo_test_generation(uint32_t n){wf_jo_assets_demand_end();binding_generation=n;preparation_generation_exhausted=0;}\n'
            'uint32_t wf_jo_test_binding_generation(void){return binding_generation;}\n'
            'uint32_t wf_jo_test_demand_generation(void){\n#ifdef WF_ASSETS_WORKER\nreturn demand_generation;\n#else\nreturn 0;\n#endif\n}\n')
        presenter = out/'presenter-fixture.c'
        presenter.write_text('#include '+json.dumps(str(root/'core/src/wf_r18_present.c'))+'\n'
            'static wf_r18_video_t test_video;\n'
            'void wf_jo_test_stage_capacity(size_t n){backend.hwr_stage_bytes=n;}\n'
            'wf_status_t wf_jo_test_records(uint16_t first,unsigned n){\n'
            ' for(unsigned i=0;i<n;i++){memset(&records[i],0,sizeof records[i]);\n'
            '  uint8_t *p=records[i].bytes;unsigned tile=(uint16_t)(first+i);\n'
            '  p[1]=40;p[3]=1;p[5]=(uint8_t)tile;p[7]=(uint8_t)(tile>>8);p[11]=32;}\n'
            ' return submit_records(&test_video,n);}\n')
        cases.append(('lifecycle', [ROOT/'tests/test_jo_preparation_lifecycle.c', assets, presenter, wrapper,
            root/'port/wf_jo_backend.c', root/'core/src/wf_saturn_memory.c',
            root/'core/src/wf_r18_metasprite.c', root/'core/src/wf_r18_video.c',
            *(root/'port/zlib'/n for n in ['inflate.c', 'inffast.c', 'inftrees.c', 'adler32.c', 'zutil.c'])],
            ['-DWF_JO_ASSETS_HOST', '-DZ_SOLO', '-DNO_GZIP', '-Wno-implicit-fallthrough',
             '-Wl,--gc-sections', '-Wl,--wrap=wf_jo_video_texture_protect']))
        _, paths, extra = cases[-1]
        cases.append(('lifecycle-worker', [*paths, root/'tests/native/assets_worker_host.c'],
                      [*extra, '-DWF_JO_ASSETS_TEST_WORKER']))
        cases.append(('lifecycle-worker-scalar', [*paths, root/'tests/native/assets_worker_host.c'],
                      [*extra, '-DWF_JO_ASSETS_TEST_WORKER', '-DWF_TEST_SCALAR_CONSUME',
                       '-Wl,--wrap=wf_jo_backend_consume_prepared_rom_tiles']))
        if not a.without_private_consume:
            cases.append(('private-consume', [ROOT/'tests/test_jo_prepared_consume.c',
                          root/'port/wf_jo_backend.c', wrapper, root/'core/src/wf_saturn_memory.c'],
                          ['-Wl,--gc-sections']))
    for name, paths, extra in cases:
        for source in [*paths, Path(__file__).resolve()]:
            report['source_sha256'][str(source)] = hashlib.sha256(source.read_bytes()).hexdigest()
        exe = out/name
        run_args = [str(root/'cd/ROM68K.BIN'), str(root/'cd/SPRITES.WFP')] if name.startswith('lifecycle') else []
        for phase, argv in [('build', [*flags, *map(str, paths), *extra, '-o', str(exe)]), ('run', [str(exe), *run_args])]:
            r = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
            (out/(name+'-'+phase+'.log')).write_text(r.stdout)
            results.append(dict(name=name, phase=phase, argv=argv, returncode=r.returncode, output=r.stdout))
            (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
            print(r.stdout, end='')
            r.check_returncode()
    assert all(hashlib.sha256(Path(f).read_bytes()).hexdigest() == digest
               for f, digest in report['source_sha256'].items())
    if not a.baseline:
        assert (out/'lifecycle-worker-run.log').read_bytes() == (out/'lifecycle-worker-scalar-run.log').read_bytes()


if __name__ == '__main__':
    main()
