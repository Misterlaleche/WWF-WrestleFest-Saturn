from pathlib import Path
import hashlib,json,subprocess,sys
here=Path(__file__).resolve().parent
score=here.parents[1]
phase=sys.argv[1]
out=here/phase;out.mkdir(parents=True,exist_ok=True)
src=here/'repo/src/audio/native'
fixture=score/'sfx-test-contract/green'
pack=score.parent.parent/'pcm/firmware-tests/input/inputs/SFX.DAT'
gold=Path('C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/work/starter/WF_SCSP_NATIVE_OPT_R01/assets/WF_OKI_PCM16BE.PAK')
assert hashlib.sha256(gold.read_bytes()).hexdigest()=='45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee'
def quote(value):return '"'+str(value)+'"'
compile=['cl','/nologo','/O2','/W4','/WX','/std:c11','/I'+str(src),'/I'+str(fixture),src/'wf_scsp_sfx.c',here/'test_prepare.c','/Fetest_prepare.exe']
cmd=out/'run.cmd'
cmd.write_text('@echo off\ncall "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b 1\n'+' '.join(map(quote,compile))+'\nif errorlevel 1 exit /b 1\n'+ ' '.join(map(quote,[out/'test_prepare.exe',pack,gold]))+'\nexit /b %errorlevel%\n')
process=subprocess.run(['cmd','/d','/c',str(cmd)],cwd=out,text=True,capture_output=True)
report=dict(exit_code=process.returncode,stdout=process.stdout,stderr=process.stderr)
(out/'RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
print(process.stdout+process.stderr)
raise SystemExit(process.returncode)
