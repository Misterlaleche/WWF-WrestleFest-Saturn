from pathlib import Path
import json,subprocess,sys,hashlib
here=Path(__file__).resolve().parent;repo=here.parents[4]
mode=sys.argv[1];src=(repo/'src/audio/native')if mode=='red' else here/'src'
out=here/mode;out.mkdir(parents=True,exist_ok=True)
vc='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
cmd=['cl','/nologo','/O2','/std:c11','/W4','/WX','/I'+str(src),'/I'+str(repo/'src/audio/native')]
if mode=='red':cmd+=['/DWF_TEST_LEGACY=1','/wd4505']
cmd+=[str(src/'wf_music_elf.c'),str(here/'test_loader.c'),'/Fetest_loader.exe']
script=out/'build.cmd';script.write_text('@echo off\ncall "'+vc+'" >nul\nif errorlevel 1 exit /b 1\n'+' '.join('"'+a+'"'for a in cmd)+'\nexit /b %errorlevel%\n')
b=subprocess.run(['cmd','/d','/c',str(script)],cwd=out,text=True,capture_output=True)
(out/'build.log').write_text(b.stdout+b.stderr,encoding='utf-8');assert b.returncode==0,b.stdout+b.stderr
r=subprocess.run([str(out/'test_loader.exe')],cwd=out,text=True,capture_output=True)
report=dict(mode=mode,exit_code=r.returncode,output=r.stdout,loader_sha256=hashlib.sha256((src/'wf_music_elf.c').read_bytes()).hexdigest())
(out/'RESULT.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
