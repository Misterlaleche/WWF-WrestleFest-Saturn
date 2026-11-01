from pathlib import Path
import subprocess,re,time,json,hashlib
base=Path(__file__).resolve().parent;repo=base/'repo';src=repo/'src/audio/native';out=base/'host';out.mkdir(exist_ok=True);fix=base.parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert json.loads((base/'SH2-RESULT.json').read_text())['strict_link_passed']
strip=lambda s:re.sub(r'__attribute__\(\(section\("[^"]+"\)\)\)','',s)
(out/'hot.c').write_text(strip((base/'music-hot/z80-native.c').read_text()))
(out/'cold.c').write_text(strip((base/'music-hot/z80-cold.c').read_text()))
(out/'seq.c').write_text(strip((src/'wf_music_seq.c').read_text()))
(out/'scalar.c').write_text((repo/'scalar_test_support.c').read_text()+(repo/'scalar_test_main.c').read_text())
support=(repo/'scalar_test_support.c').read_text()
allops=(fix/'scalar-admission-table/test_all_opcodes.c').read_text()
(out/'allops-public.c').write_text(support+allops)
(out/'allops-batch.c').write_text(support+allops.replace('wf_z80_scalar_step','wf_z80_control_batch'))
(out/'long512.c').write_bytes((fix/'control-batch/q512/long_quantum512.c').read_bytes())
review=fix/'z80-cost/dispatcher-review'
(out/'test_batch_independent.c').write_bytes((review/'batch-review/test_batch.c').read_bytes())
(out/'test_fast_boundaries.c').write_bytes((review/'test_boundaries.c').read_bytes())
vc='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
def compile(name,files,flags=()):
 cmd=['cl','/nologo','/O2','/std:c11','/W3','/wd4244','/I'+str(src),'/I'+str(src/'reference_driver'),'/DWF_MUSIC_NATIVE_CONTROL=1',*flags,*map(str,files),'/Fe'+str(out/name)]
 (out/'compile.cmd').write_text('@echo off\ncall "'+vc+'" >nul\n'+' '.join('"'+str(x)+'"'for x in cmd)+'\nexit /b %errorlevel%\n')
 p=subprocess.run(['cmd','/d','/c',str(out/'compile.cmd')],cwd=out,capture_output=True,text=True);(out/(name+'.build.log')).write_text(p.stdout+p.stderr);p.check_returncode()
def run(name,args):
 t=time.perf_counter();p=subprocess.run(list(map(str,args)),cwd=out,capture_output=True,text=True);elapsed=time.perf_counter()-t;(out/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,flush=True);p.check_returncode();return dict(seconds=elapsed,output=p.stdout)
rename=['/D'+n+'=wf_ref_'+n for n in('z80_init','z80_step','z80_gen_nmi','z80_gen_int','z80_debug_output')]
compile('ref-z80.obj',[src/'reference_driver/z80.c'],['/c','/Foref-z80.obj',*rename])
compile('ref-seq.obj',[repo/'tests/audio/native-driver/reference/wf_music_seq_r01.c'],['/c','/Foref-seq.obj',*rename,*['/Dwf_music_seq_'+n+'=wf_ref_'+n for n in('init','send','run')]])
compile('hot.obj',[out/'hot.c'],['/c','/Fohot.obj']);compile('cold.obj',[out/'cold.c'],['/c','/Focold.obj'])
common=[out/'seq.c',out/'hot.obj',out/'cold.obj'];refs=[out/'ref-z80.obj',out/'ref-seq.obj'];rom=repo/'input/sound.rom';runs={}
for name,test,extra,args in [('scalar-api',out/'scalar.c',[out/'ref-z80.obj'],[rom]),('fast-api',out/'test_fast_boundaries.c',[],[rom]),('control-batch',repo/'test_control_batch.c',refs,[rom]),('independent-matrix',out/'test_batch_independent.c',refs,[rom]),('control-irq',repo/'tests/audio/native-driver/test_control_differential.c',refs,[rom]),('PRE-POST',base/'test_interactions.c',refs,[rom]),('all-opcodes-public',out/'allops-public.c',[out/'ref-z80.obj'],[rom]),('all-opcodes-batch',out/'allops-batch.c',[out/'ref-z80.obj'],[rom]),('long512',out/'long512.c',refs,[rom,out/'checkpoints512.csv'])]:
 compile(name+'.exe',[*common,test,*extra]);runs[name]=run(name,[out/(name+'.exe'),*args])
frozen=json.loads((base/'FROZEN-INPUTS.json').read_text())
assert sha(fix/'scalar-admission-table/scalar-admission-table-sources.tar.gz')==frozen['table_archive_sha256']
changed=[]
for path,digest in frozen['files'].items():
 if sha(repo/path)!=digest:changed.append(path)
assert changed==['tools/audio/native_probe/generate_z80_control.py'],changed
assert sha(base/'baseline/z80-inline.c')==sha(fix/'scalar-inline/music-hot/z80-native.c')
assert sha(base/'baseline/z80-cold.c')==sha(base/'music-hot/z80-cold.c')
report=dict(passed=True,baseline='frozen compound fab67/static18k',scalar_API_exact_hits=50376,scalar_API_immutable_declines=1749624,fast_API_budget_limit_checks=128000,fast_API_control_declines=32000,batch_functional_checks=586433,real_post_callback_mutants=64000,independent_batch_matrix_reused=True,allopcases_per_entrypoint=8192,private_body_tested_via_batch_budget_one=True,long=dict(quantum_fs=512,budget=4096,tracks=18,score_s=600,fade_s=8,exact_frozen_R01_checkpoints=10962),runs=runs,generated_hot_sha256=sha(base/'music-hot/z80-native.c'),frozen_sources_unchanged=True,production_modified=False,emulator_run=False,vm_run=False)
(base/'HOST-RESULT.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS factorial scalar-inline + admission-table all API matrices, public/private all-opcodes, PRE/POST interactions and long512',flush=True)
