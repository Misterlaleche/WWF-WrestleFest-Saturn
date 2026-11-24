from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
source=args.source.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
flags=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DWF_JO_ENGINE',
       '-I'+str(source/'core/include'),'-I'+str(source/'port')]
commands=[]
def run(command,log):
    r=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    log.write_text(r.stdout);commands.append(dict(command=command,returncode=r.returncode,log=str(log)))
    if r.returncode:print(r.stdout)
    r.check_returncode();return r.stdout
reference=ROOT/'tests/render_reference/b12_wf_r18_metasprite.c'
run([*flags,'-Dwf_r18_metasprite_emit=wf_b12_metasprite_emit','-Dwf_r18_metasprite_cache_reset=wf_b12_metasprite_cache_reset',
    '-c',str(reference),'-o',str(out/'reference.o')],out/'reference-build.log')
run([*flags,str(HERE/'test_metasprite_hint.c'),str(source/'core/src/wf_r18_metasprite.c'),str(out/'reference.o'),'-o',str(out/'parser-test')],out/'build.log')
output=run([str(out/'parser-test'),str(source/'cd/ROM68K.BIN')],out/'run.log');print(output,end='')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
(out/'result.json').write_text(json.dumps(dict(source=str(source),commands=commands,output=output,
    sources={str(p):sha(p) for p in [HERE/'test_metasprite_hint.c',reference,source/'core/src/wf_r18_metasprite.c',source/'core/include/wf_r18_metasprite.h']}),indent=2)+'\n')
