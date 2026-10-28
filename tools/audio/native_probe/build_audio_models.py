"""Build pinned SH2/SCSP host models for software tests; never target firmware."""
import argparse,hashlib,json,subprocess,shutil,sys
from pathlib import Path
from build_dsp_mixer import generate
from prepare_probe import PINNED
def build(a):
 repo=a.repo.resolve();src=repo/'tests/audio/models';out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);native=repo/'src/audio/native'
 for name in ['opm.c','opm.h']:
  if hashlib.sha256((src/'upstream'/name).read_bytes()).hexdigest()!=PINNED[name]:raise ValueError('Pinned R04 dependency mismatch: '+name)
 for tag in ['yabause','mame']:
  provenance=json.loads((src/'upstream'/tag/'PROVENANCE.json').read_text())
  for name,entry in provenance['files'].items():
   if hashlib.sha256((src/'upstream'/tag/Path(name).name).read_bytes()).hexdigest()!=entry['sha256']:raise ValueError('Upstream hash mismatch: '+name)
 for name in ['generate_sh2.py','generate_scsp_model.py']:subprocess.run([sys.executable,'-B',str(src/name),'--out',str(out)],check=True)
 generate(out)
 for name in ['sh2_benchmark.cpp','audio_renderer.cpp','opm_bridge.c']:shutil.copyfile(src/name,out/name)
 commands=[]
 def cl(args):
  cmd=['cl','/nologo','/O2',*args];script=out/'compile.cmd'
  script.write_text('@echo off\ncall "'+str(a.vcvars)+'" >nul\nif errorlevel 1 exit /b 1\n'+' '.join('"'+str(x)+'"' for x in cmd)+'\nexit /b %errorlevel%\n')
  r=subprocess.run(['cmd','/d','/c',str(script)],cwd=out,capture_output=True,text=True);commands.append(dict(command=list(map(str,cmd)),exit_code=r.returncode,output=r.stdout+r.stderr))
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 cl(['/std:c++17','/EHsc','sh2_benchmark.cpp','/Fesh2_benchmark.exe'])
 subprocess.run([str(out/'sh2_benchmark.exe'),'--selftest'],cwd=out,check=True)
 for delay in [4,0]:cl(['/std:c++17','/EHsc','/LD','/DSCSP_FM_DELAY='+str(delay),'scsp_model.cpp','scspdsp.cpp','/Fescsp_model'+str(delay)+'.dll'])
 cl(['/std:c11','/I'+str(src/'upstream'),'/LD',src/'upstream/opm.c',out/'opm_bridge.c','/Feopm_reference.dll'])
 cl(['/std:c11','/I'+str(native),'/I'+str(out),'/c',*[native/n for n in ['wf_scsp_sfx.c','wf_scsp_dsp.c','wf_scsp_ym.c','wf_music_seq.c']]])
 cl(['/std:c11','/I'+str(native/'reference_driver'),'/c',native/'reference_driver/z80.c'])
 for delay in [4,0]:cl(['/std:c++17','/EHsc','/I'+str(native),'/DSCSP_FM_DELAY='+str(delay),'audio_renderer.cpp','scspdsp.cpp','wf_scsp_sfx.obj','wf_scsp_dsp.obj','wf_scsp_ym.obj','wf_music_seq.obj','z80.obj','/Feaudio_renderer'+str(delay)+'.exe'])
 (out/'models-build.json').write_text(json.dumps(dict(passed=True,physical_validation=False,source_sha256={n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ['sh2_handlers.inc','sh2_benchmark.cpp','scsp_model.cpp','audio_renderer.cpp']}),indent=2)+'\n')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--out',type=Path,required=True)
 p.add_argument('--vcvars',type=Path,default=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'));build(p.parse_args())
