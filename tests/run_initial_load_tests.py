#!/usr/bin/env python3
"""Host regressions for raw ISO sizes, cached WAV headers and CD diagnostics.

Requires a C/C++ compiler, Python and the packaged CD assets. No SH2 compiler,
emulator, ROM archive or sibling work directory is used. --rom-oracle optionally
adds machine68k validation of the combat audio traces against ROM68K.BIN.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT)
    parser.add_argument('--assets',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--rom-oracle',action='store_true')
    args=parser.parse_args()
    source=args.source.resolve();assets=(args.assets or source/'cd').resolve()
    out=(args.output or source/'build/initial-load-tests').resolve();out.mkdir(parents=True,exist_ok=True)
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    paths=[source/'makefile',source/'core_sources.mk']
    for folder in ('port','core','sdk/jo_engine','tests','tools'):
        paths += [p for p in (source/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.h','.py','.in','.cpp')]
    before={str(p.relative_to(source)):sha(p) for p in sorted(set(paths))}
    py=sys.executable
    def test(name,*arguments):return [py,str(source/'tests'/name),*map(str,arguments)]
    commands=[
        ('raw-metadata',test('test_raw_disc_metadata.py','--source',source,'--assets',assets,'--output',out/'raw-metadata')),
        ('raw-size-negative',test('test_raw_disc_metadata.py','--source',source,'--assets',assets,'--mutant','size-plus-one','--output',out/'raw-size-negative')),
        ('raw-allowlist-negative',test('test_raw_disc_metadata.py','--source',source,'--assets',assets,'--mutant','all-raw-names','--output',out/'raw-allowlist-negative')),
        ('raw-packaging',test('test_raw_disc_packaging.py','--source',source,'--assets',assets,'-v')),
        ('music-api',test('test_music_headers_api.py','--source',source,'--assets',assets,'--output',out/'music-api')),
        ('music-generation',test('test_music_headers_generation.py','--source',source,'--assets',assets,'--output',out/'music-generation')),
        ('music-order',test('test_music_headers_order.py','--source',source,'--assets',assets,'--output',out/'music-order')),
        ('cd-diagnostics',test('test_cd_diagnostics.py','--source-dir',source,'--build-dir',out/'cd-diagnostics')),
        ('cd-ui',test('test_cd_loading_ui.py','--source-dir',source,'--build-dir',out/'cd-ui')),
        ('cd-negatives',test('test_cd_diagnostic_mutants.py','--source-dir',source,'--input-dir',out/'cd-diagnostics','--build-dir',out/'cd-negatives')),
        ('cd-protocol',test('test_cd_protocol.py','--asset-dir',assets)),
        ('boot-progress',test('test_boot_load_progress.py')),
        ('audio-service',test('test_audio.py','--source',source,'--assets',assets,'--sanitize')),
        ('audio-decoder',test('test_audio_decoder.py','--source',source,'--pack',assets/'SFX.DAT','--assets',assets,'--sanitize','--output',out/'audio-decoder')),
        ('combat-audio',test('test_combat_audio.py','--assets',assets,*(['--rom-oracle'] if args.rom_oracle else []))),
        ('music-stream',test('test_music_stream.py')),
        ('music-deadline',test('test_music_deadline.py')),
        ('frontend-fault',test('test_main_frontend_fault.py','--source',source/'port/main.c','--output',out/'frontend-fault')),
    ]
    flags=['-std=c11','-fsyntax-only','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE','-DWF_JO_ASSETS_HOST',
           '-DWF_JO_FRONTEND_TEST','-DWF_JO_FRONTEND','-DWF_JO_AUTORUN','-DWF_JO_AUTORUN_MODE=2','-DZ_SOLO','-DNO_GZIP']
    flags+=['-I'+str(source/p) for p in ('tests/native/jo_host','tests','core/include','port','port/zlib','tests/native')]
    commands.append(('native-main-compile',[os.environ.get('CC','cc'),*flags,str(source/'tests/native/jo_main_probe.c')]))
    report={'status':'RUNNING','source':str(source),'assets':str(assets),'source_hashes':before,'runs':[],
            'scope':'Host source/ABI/PCM/I/O contracts; no Saturn timing or physical-disc acceptance.'}
    def save(): (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    for label,command in commands:
        log=out/(label+'.log')
        with log.open('w') as stream:
            result=subprocess.run(command,cwd=source,stdout=stream,stderr=subprocess.STDOUT,
                env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
        report['runs'].append({'name':label,'command':command,'returncode':result.returncode,'log':log.name,'log_sha256':sha(log)})
        save();print(label,'PASS' if result.returncode==0 else 'FAIL',flush=True)
        if result.returncode:print(log.read_text(),flush=True)
    report['source_drift']=[p for p,h in before.items() if not(source/p).exists() or sha(source/p)!=h]
    report['status']='PASS' if not report['source_drift'] and all(r['returncode']==0 for r in report['runs']) else 'FAIL'
    save();return report['status']!='PASS'

if __name__=='__main__':raise SystemExit(main())
