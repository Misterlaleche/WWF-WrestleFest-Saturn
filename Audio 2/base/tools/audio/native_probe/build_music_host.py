"""Rebuild/run native music host tests with MSVC. No disc-generation command."""
import argparse,json,subprocess
from pathlib import Path
from generate_z80_control import generate
from adapt_dsp_oracle import generate as adapt_dsp

def build(a):
 repo=a.repo.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
 src=repo/'src/audio/native';tests=repo/'tests/audio/native-driver';logs=[]
 def run(name,args):
  r=subprocess.run(list(map(str,args)),cwd=out,capture_output=True,text=True)
  (out/(name+'.txt')).write_text(r.stdout+r.stderr,encoding='utf-8');logs.append(dict(name=name,exit_code=r.returncode))
  print(name,r.returncode,flush=True)
  if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 def compile(name,sources,extra=(),cpp=False):
  command=['cl','/nologo','/O2',('/std:c++17' if cpp else '/std:c11'),'/I'+str(src),*extra,*map(str,sources),'/Fe'+name]
  script=out/'compile.cmd';script.write_text('@echo off\ncall "'+str(a.vcvars)+'" >nul\nif errorlevel 1 exit /b 1\n'+' '.join('"'+str(x)+'"' for x in command)+'\nexit /b %errorlevel%\n')
  run('build-'+Path(name).stem,['cmd','/d','/c',script])
 adapted=adapt_dsp(tests/'dsp_oracle/scspdsp.cpp',out/'dsp-hardware-adapter')
 compile('dsp_oracle.dll',[adapted,tests/'dsp_oracle/bridge.cpp'],['/EHsc','/LD','/I'+str(tests/'dsp_oracle')],True)
 compile('ym_oracle.dll',[src/'wf_scsp_ym.c',tests/'ym_bridge.c'],['/LD','/W4','/WX'])
 # Upstream Z80 warnings are isolated from the project's /W4 /WX builds.
 generate(repo,a.rom,out/'z80-native.c')
 compile('z80.obj',[out/'z80-native.c'],['/c','/W3','/wd4244','/Foz80.obj','/I'+str(src/'reference_driver')])
 cases=[('music_seq',[src/'wf_music_seq.c'],[]),('music_catalogue',[src/'wf_music_seq.c',src/'wf_scsp_ym.c'],[]),('music_module',[src/'wf_music_seq.c',src/'wf_scsp_ym.c',src/'wf_music_module.c'],['/DWF_MUSIC_MODULE_TEST=1']),('ym',[src/'wf_scsp_ym.c'],[]),('music_elf',[src/'wf_music_elf.c'],[]),('music_time',[],[])]
 for name,sources,flags in cases:
  deps=[out/'z80.obj'] if name in ('music_seq','music_catalogue','music_module') else []
  compile('test_'+name+'.exe',[*sources,tests/('test_'+name+'.c'),*deps],['/W4','/WX','/DWF_MUSIC_NATIVE_CONTROL=1',*flags])
  args=[out/('test_'+name+'.exe')]
  if name in ('music_seq','music_catalogue','music_module'):args.append(a.rom.resolve())
  if name=='music_elf':args.append(a.module_elf.resolve())
  run(name,args)
 # Freeze the R01 wrapper and interpreter separately: the differential must
 # detect regressions shared by today's generic and specialized paths.
 rename=['/D'+n+'=wf_ref_'+n for n in ['z80_init','z80_step','z80_gen_nmi','z80_gen_int','z80_debug_output']]
 compile('ref-z80.obj',[src/'reference_driver/z80.c'],['/c','/W3','/wd4244','/Foref-z80.obj',*rename])
 seqrename=['/Dwf_music_seq_'+n+'=wf_ref_'+n for n in ['init','send','run']]
 compile('ref-seq.obj',[tests/'reference/wf_music_seq_r01.c'],['/c','/W4','/WX','/Foref-seq.obj',*rename,*seqrename])
 compile('test_control_differential.exe',[src/'wf_music_seq.c',tests/'test_control_differential.c',out/'ref-z80.obj',out/'ref-seq.obj',out/'z80.obj'],['/W4','/WX','/DWF_MUSIC_NATIVE_CONTROL=1'])
 run('control-differential',[out/'test_control_differential.exe',a.rom.resolve()])
 run('dsp-mixer',[a.python,'-B',tests/'test_dsp_mixer.py',out/'dsp_oracle.dll','-v'])
 (out/'host-tests.json').write_text(json.dumps(dict(passed=True,groups=logs,physical_validation=False),indent=2)+'\n')
if __name__=='__main__':
 import sys
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--vcvars',type=Path,default=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'));p.add_argument('--python',default=sys.executable)
 for n in ['out','rom','module-elf']:p.add_argument('--'+n,type=Path,required=True)
 build(p.parse_args())
