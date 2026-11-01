#!/usr/bin/env python3
"""Reproduce host logic/deadline gates; these are not Saturn performance tests."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys,time

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('rom-zip','resources','firmware','out'):p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--seconds',type=int,default=600)
    p.add_argument('--cc',default='cc')
    a=p.parse_args();root=Path(__file__).resolve().parents[1];out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
    if not 1<=a.seconds<=1200:p.error('seconds must be1..1200')
    env=dict(os.environ,WF_ROM_ZIP=str(a.rom_zip.resolve()),PYTHONDONTWRITEBYTECODE='1')
    files=[q for folder in ('src','include','tests') for q in (root/folder).rglob('*') if q.is_file() and q.suffix in ('.c','.h','.inc','.S','.py')]
    before={str(q.relative_to(root)):sha(q) for q in sorted(files)}
    report={'scope':'HOST regression and original-ROM event logic; not emulated Saturn waveform or combat performance',
            'source_sha256':before,'inputs':{str(q):sha(q) for q in (a.rom_zip,a.resources/'SOUND.ROM',a.resources/'SFX.DAT',a.firmware)},
            'sequence_seconds_per_command':a.seconds,'tests':[],'commands':[]}
    def run(name,args,test=True):
        started=time.monotonic();r=subprocess.run(list(map(str,args)),cwd=root,env=env,text=True,capture_output=True)
        item={'name':name,'command':list(map(str,args)),'exit_code':r.returncode,'seconds':time.monotonic()-started,'stdout':r.stdout,'stderr':r.stderr}
        report['commands'].append(item)
        if test:report['tests'].append(item)
        (out/'host-tests.json').write_text(json.dumps(report,indent=2)+'\n')
        if r.returncode:raise RuntimeError(name+' failed\n'+r.stdout+r.stderr)
        if test:print(name+': PASS',flush=True)
    common=[a.cc,'-std=c11','-O2','-Wall','-Wextra','-Iinclude','-Isrc','-Isrc/reference_driver']
    run('resource_identity_and_corruption',[sys.executable,'-B','-m','unittest','discover','-s','tests','-p','test_resources.py'])
    run('build_input_aliases',[sys.executable,'-B','-m','unittest','discover','-s','tests','-p','test_build_safety.py'])
    run('source_profile_snapshot',[sys.executable,'-B','-m','unittest','discover','-s','tests','-p','test_source_snapshot.py'])
    cases=[
      ('api',['tests/test_audio2_api.c','src/wf_audio2.c','src/wf_native_host.c','src/wf_native_elf.c','src/wf_scsp_sfx.c'],[],[a.firmware.resolve(),(a.resources/'SFX.DAT').resolve()]),
      ('module_deadlines',['tests/test_music_module_deadline.c','src/wf_music_seq.c','src/wf_scsp_ym.c','src/reference_driver/z80.c'],['-fno-pie','-no-pie'],[]),
      ('firmware_deadlines',['tests/test_music_firmware_deadline.c','src/wf_scsp_sfx.c','src/wf_scsp_ym.c'],[],[]),
      ('firmware_deadlines_no_timing',['tests/test_music_firmware_deadline.c','src/wf_scsp_sfx.c','src/wf_scsp_ym.c'],['-DWF_MUSIC_TIMING=0'],[]),
      ('sfx_refill_deadlines',['tests/test_sfx_refill_deadline.c','src/wf_scsp_sfx.c'],[],[])]
    for name,sources,flags,args in cases:
        exe=out/name;run('build_'+name,[*common,'-Werror',*flags,*sources,'-o',exe],False);run(name,[exe,*args])
    aliases=['z80_init=wf_ref_z80_init','z80_step=wf_ref_z80_step','z80_gen_nmi=wf_ref_z80_gen_nmi','z80_gen_int=wf_ref_z80_gen_int','z80_debug_output=wf_ref_z80_debug_output']
    defs=['-D'+v for v in aliases]
    ref_z80=out/'ref-z80.o';ref_seq=out/'ref-seq.o'
    run('build_original_z80',[*common,*defs,'-c','src/reference_driver/z80.c','-o',ref_z80],False)
    run('build_frozen_r01',[*common,*defs,'-Dwf_music_seq_init=wf_ref_init','-Dwf_music_seq_send=wf_ref_send','-Dwf_music_seq_run=wf_ref_run','-c','tests/oracle/wf_music_seq_r01.c','-o',ref_seq],False)
    exe=out/'seq_oracle'
    run('build_serviced_native_oracle',[*common,'-Werror','-Wno-unused-const-variable','-Wno-unused-function','-Wno-misleading-indentation','-DWF_MUSIC_NATIVE_CONTROL=1',
        'tests/test_music_seq_service.c','src/wf_music_seq.c','src/reference_driver/z80-native.c','src/reference_driver/z80-cold.c',ref_z80,ref_seq,'-o',exe],False)
    run('all18_full_z80_and_live_transitions',[exe,(a.resources/'SOUND.ROM').resolve(),a.seconds])
    after={str(q.relative_to(root)):sha(q) for q in sorted(files)}
    report['source_stable_during_tests']=before==after
    report['passed']=before==after and all(t['exit_code']==0 for t in report['tests'])
    report['compiler']=subprocess.run([a.cc,'--version'],capture_output=True,text=True,check=True).stdout.splitlines()[0]
    (out/'host-tests.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
