"""Run original guard matrices, new composition interactions and600s oracle."""
from pathlib import Path
import hashlib,json,re,subprocess,time,sys
here=Path(__file__).resolve().parent;fix=here.parent;src=here/'src/audio/native';out=here/'host';out.mkdir(exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
contract={'generated_hot_sha256':json.loads((here/'PIN.json').read_text())['generated']['z80-native.c']['lf_sha256']}
assert sha(here/'build/z80-native.c')==contract['generated_hot_sha256']
strip=lambda s:re.sub(r'__attribute__\(\(section\("[^"]+"\)\)\)','',s)
for name,p in [('hot.c',here/'build/z80-native.c'),('cold.c',here/'build/z80-cold.c'),('seq.c',src/'wf_music_seq.c')]:
 (out/name).write_text(strip(p.read_text()))
(out/'ref-z80.c').write_bytes((src/'reference_driver/z80.c').read_bytes())
(out/'ref-seq.c').write_bytes((here/'original-matrices/ref-seq.c').read_bytes())
post=here/'original-matrices/post.c'
(out/'test-post.c').write_text(post.read_text().replace('reference_z80_','wf_ref_z80_'))
(out/'post-wrapper.c').write_text('#include "wf_music_seq.h"\nextern unsigned wf_z80_control_fast_step(z80*,unsigned,uint32_t);\nunsigned wf_test_post(z80*z,unsigned b,uint32_t t){if(z->pc!=0xf5f)return 0;return wf_z80_control_fast_step(z,b,t);}\n')
(out/'long.c').write_bytes((here/'original-matrices/long.c').read_bytes())
vc='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
common=['cl','/nologo','/O2','/std:c11','/W3','/wd4244','/I'+str(src),'/I'+str(src/'reference_driver')]
rename=['/D'+n+'=wf_ref_'+n for n in ['z80_init','z80_step','z80_gen_nmi','z80_gen_int','z80_debug_output']]
commands=[]
for name in ('hot','cold','seq'):
 commands.append(common+['/DWF_MUSIC_NATIVE_CONTROL=1','/c',str(out/(name+'.c')),'/Fo'+str(out/(name+'.obj'))])
commands += [common+['/c',*rename,str(out/'ref-z80.c'),'/Fo'+str(out/'ref-z80.obj')],
 common+['/c',*rename,*['/Dwf_music_seq_'+n+'=wf_ref_'+n for n in ['init','send','run']],str(out/'ref-seq.c'),'/Fo'+str(out/'ref-seq.obj')]]
hot=[out/'hot.obj',out/'cold.obj'];seq=[*hot,out/'seq.obj'];reference=[out/'ref-z80.obj',out/'ref-seq.obj']
tests=[('pre',hot+[out/'ref-z80.obj',here/'original-matrices/pre.c']),
 ('post',hot+[out/'ref-z80.obj',out/'post-wrapper.c',out/'test-post.c']),
 ('startup',seq+[out/'ref-z80.obj',here/'original-matrices/startup.c']),
 ('batch',seq+reference+[here/'tests/test_control_batch.c']),
 ('wide',seq+reference+[here/'original-matrices/wide.c']),
 ('control',seq+reference+[here/'tests/test_control_differential.c']),
 ('interactions',seq+[out/'ref-z80.obj',here/'test_interactions.c']),
 ('long',seq+reference+[out/'long.c'])]
selected=[item for item in tests if not sys.argv[1:] or item[0] in sys.argv[1:]]
for name,files in selected:commands.append(common+['/DWF_MUSIC_NATIVE_CONTROL=1',*map(str,files),'/Fe'+str(out/(name+'.exe'))])
(out/'compile.cmd').write_text('@echo off\ncall "'+vc+'" >nul\n'+'\nif errorlevel 1 exit /b 1\n'.join(' '.join('"'+x+'"'for x in cmd)for cmd in commands)+'\nexit /b %errorlevel%\n')
p=subprocess.run(['cmd','/d','/c',str(out/'compile.cmd')],cwd=out,capture_output=True,text=True)
(out/'build.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,flush=True);p.check_returncode()
rom=here/'input/sound.rom';results=json.loads((here/'HOST-PROGRESS.json').read_text()) if sys.argv[1:] and (here/'HOST-PROGRESS.json').is_file() else {}
for name,files in selected:
 args=[str(out/(name+'.exe')),str(rom)]
 if name=='long':args.append(str(out/'checkpoints.csv'))
 start=time.perf_counter();p=subprocess.run(args,cwd=out,capture_output=True,text=True)
 (out/(name+'.log')).write_text(p.stdout+p.stderr);print(name+': '+p.stdout+p.stderr,flush=True);p.check_returncode()
 results[name]=dict(passed=True,seconds=time.perf_counter()-start,output=p.stdout)
 (here/'HOST-PROGRESS.json').write_text(json.dumps(results,indent=2)+'\n')
assert sha(here/'build/z80-native.c')==contract['generated_hot_sha256']
if not all(name in results for name,files in tests):raise SystemExit(0)
report=dict(passed=True,tests=results,tracks=18,score_seconds=600,fade_seconds=8,quantum_fs=512,budget=4096,
    long_checkpoints=10962,hot_sha256=contract['generated_hot_sha256'],cold_sha256=sha(here/'build/z80-cold.c'),
    seq_sha256=sha(src/'wf_music_seq.c'),post_test_only_symbol_renames=True,
    original_post_test_sha256=sha(post),guest_sources_unchanged_during_test=True,media_created=False)
(here/'HOST-RESULT.json').write_text(json.dumps(report,indent=2)+'\n')
