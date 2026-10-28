"""Reproducible Windows/MSVC tests for the resident native SFX profile.

Requires the user's audited PCM16BE PAK (SHA256 checked before comparison).
Does not generate any disc image. CPU tests separately require machine68k.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path

def verify(repo,pcm,pack,out,vcvars):
    repo,pcm,pack,out,vcvars=[p.resolve() for p in (repo,pcm,pack,out,vcvars)]
    out.mkdir(parents=True,exist_ok=True)
    ref=repo/'tests/audio/native-driver/reference'
    audit=json.loads((ref/'WF_OKI_AUDIT.json').read_text())
    if hashlib.sha256(pcm.read_bytes()).hexdigest()!=audit['pak_sha256']:raise ValueError('Audited PCM PAK hash mismatch')
    h=['typedef struct {unsigned bank,id,offset,frames;} fixture_t;','static const fixture_t fixtures[114]={']
    h+=['{%d,%d,%d,%d},'%(s['bank'],s['sample_id'],s['offset'],s['frames']) for s in audit['samples']]
    (out/'sfx_fixture.h').write_text('\n'.join(h+['};'])+'\n')
    src=repo/'src/audio/native';tests=repo/'tests/audio/native-driver'
    cases=[('sfx',[src/'wf_scsp_sfx.c',ref/'wf_audio_sfx.c',tests/'test_sfx.c'],[pack,pcm]),
           ('upload',[src/'wf_native_host.c',src/'wf_sfx_upload.c',tests/'test_upload.c'],[pack]),
           ('queue',[src/'wf_native_queue.c',tests/'test_queue.c'],[])]
    logs=[]
    for name,sources,args in cases:
        commands=['@echo off','call "'+str(vcvars)+'" >nul','if errorlevel 1 exit /b 1',
                  'cd /d "'+str(out)+'"']
        command=['cl','/nologo','/W4','/WX','/O2','/std:c11','/I'+str(src),'/I'+str(ref),'/I'+str(out),*sources,'/Fe'+str(out/(name+'.exe'))]
        def quote(p):
            p=str(p)
            if '"' in p or '\n' in p:raise ValueError('Invalid command argument')
            return '"'+p+'"'
        commands+=[' '.join(map(quote,command)),'if errorlevel 1 exit /b 1',
                   ' '.join(map(quote,[out/(name+'.exe'),*args])),'exit /b %errorlevel%']
        cmd=out/(name+'.cmd');cmd.write_text('\r\n'.join(commands)+'\r\n')
        result=subprocess.run(['cmd','/c',str(cmd)],text=True,capture_output=True)
        logs.append(dict(name=name,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        (out/'c-tests.json').write_text(json.dumps(logs,indent=2)+'\n')
        print(result.stdout+result.stderr,end='',flush=True)
        if result.returncode:raise RuntimeError(name+' failed')
    return logs
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    for n in ['pcm','pack','out','vcvars']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();verify(a.repo,a.pcm,a.pack,a.out,a.vcvars)
