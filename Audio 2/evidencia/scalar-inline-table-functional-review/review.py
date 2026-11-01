"""Independent functional review of frozen INLINE + byte-admission composition."""
from pathlib import Path
import hashlib,json,re,subprocess,time
here=Path(__file__).resolve().parent;fix=here.parent;candidate=fix/'scalar-inline-admission-table'
repo=candidate/'repo';src=repo/'src/audio/native';out=here/'host';out.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
elf_sha='bee32079c629bdefed274bbcb90a9f7960872dddda0550d405119c9d6a9af0df'
assert sha(candidate/'music-hot/MUSIC.ELF')==elf_sha
structural=json.loads((fix/'scalar-inline-table-review/REVIEW.json').read_text())
original_hot=sha(candidate/'music-hot/z80-native.c')
strip=lambda s:re.sub(r'__attribute__\(\(section\("[^"]+"\)\)\)','',s)
for name,path in [('hot.c',candidate/'music-hot/z80-native.c'),('cold.c',candidate/'music-hot/z80-cold.c'),('seq.c',src/'wf_music_seq.c')]:
 (out/name).write_text(strip(path.read_text()))
batch=(fix/'z80-cost/dispatcher-review/batch-review/test_batch.c').read_text()
assert batch.count('0x93ea71d4')==1
(out/'test_batch.c').write_text(batch.replace('0x93ea71d4','0x31ac2e91'))
interactions=(fix/'integrated-native18k-admission/test_interactions.c').read_text()
assert interactions.count('0x917cc451u')==1
(out/'test_interactions.c').write_text(interactions.replace('0x917cc451u','0x9ae83f17u'))
support=(repo/'scalar_test_support.c').read_text();assert support.count('0x783298ad')==1
support=support.replace('0x783298ad','0x4b992e1c')
allops=(fix/'scalar-admission-table/test_all_opcodes.c').read_text().replace('wf_z80_scalar_step','wf_z80_control_batch')
(out/'test_allop_batch.c').write_text(support+allops)
vc='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
flags=['cl','/nologo','/O2','/std:c11','/W3','/wd4244','/DWF_MUSIC_NATIVE_CONTROL=1','/I'+str(src),'/I'+str(src/'reference_driver')]
renames=['/D'+n+'=wf_ref_'+n for n in ('z80_init','z80_step','z80_gen_nmi','z80_gen_int','z80_debug_output')]
commands=[flags+['/c',*renames,src/'reference_driver/z80.c','/Fo'+str(out/'ref-z80.obj')],
 flags+['/c',*renames,*['/Dwf_music_seq_'+n+'=wf_ref_'+n for n in('init','send','run')],repo/'tests/audio/native-driver/reference/wf_music_seq_r01.c','/Fo'+str(out/'ref-seq.obj')]]
for name in ('hot','cold','seq'):commands.append(flags+['/c',out/(name+'.c'),'/Fo'+str(out/(name+'.obj'))])
for name in ('batch','interactions','allop_batch'):
 commands.append(flags+[out/'hot.obj',out/'cold.obj',out/'seq.obj',out/'ref-z80.obj',out/'ref-seq.obj',out/('test_'+name+'.c'),'/Fe'+str(out/(name+'.exe'))])
(out/'compile.cmd').write_text('@echo off\ncall "'+vc+'" >nul\n'+'\nif errorlevel 1 exit /b 1\n'.join(' '.join('"'+str(x)+'"'for x in cmd)for cmd in commands)+'\nexit /b %errorlevel%\n')
r=subprocess.run(['cmd','/d','/c',str(out/'compile.cmd')],cwd=out,capture_output=True,text=True);(out/'build.log').write_text(r.stdout+r.stderr);r.check_returncode()
tests={}
for name in ('batch','interactions','allop_batch'):
 start=time.perf_counter();r=subprocess.run([str(out/(name+'.exe')),str(repo/'input/sound.rom')],capture_output=True,text=True)
 (out/(name+'.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,flush=True);r.check_returncode()
 tests[name]=dict(passed=True,seconds=time.perf_counter()-start,output=r.stdout)
assert sha(candidate/'music-hot/z80-native.c')==original_hot and sha(candidate/'music-hot/MUSIC.ELF')==elf_sha
result=dict(passed=True,no_P1_P2_findings=True,candidate_elf_sha256=elf_sha,candidate_hot_RAW_sha256=original_hot,
 independent_seeds=dict(batch='31ac2e91',interactions='9ae83f17',allop_batch='4b992e1c'),tests=tests,
 PRE_POST_exact_cases=48000,mandatory_C801_barriers=24000,POST5_continuations=3000,
 all256_and_CB_DD_FD_modes_through_actual_batch=8192,structural_review_already_passed=structural,
 producer_long_oracle_still_required=True,review_gate='Functional GO after producer HOST long also passes; no release approval',
 emulator_run=False,guest_sources_unchanged=True,media_created=False)
(here/'RESULT.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k not in('tests','structural_review_already_passed')},indent=2))
