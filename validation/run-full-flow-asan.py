"""Compile shared frontend composition with ASan/UBSan and enabled assertions."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[1]/'src/runtime';out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
make=(root/'Makefile').read_text();sources=[]
for key in('CORE','R18_HOST'):sources+=re.search(r'^'+key+r'=(.*)$',make,re.M)[1].split()
sources+=['tests/r18/match_driver_host.c']
flags=['-std=c11','-O1','-g','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude','-fsanitize=address,undefined','-fno-omit-frame-pointer']
def compile_one(source):
 obj=out/(source.replace('/','_')+'.o');r=subprocess.run(['cc',*flags,'-c',source,'-o',str(obj)],cwd=root,text=True,capture_output=True)
 (out/(obj.name+'.log')).write_text(r.stdout+r.stderr)
 if r.returncode:raise RuntimeError(source+'\n'+r.stdout+r.stderr)
 return obj
with ThreadPoolExecutor(max_workers=4)as pool:objects=list(pool.map(compile_one,sources))
binary=out/'match-driver-asan';subprocess.run(['cc',*flags,*map(str,objects),'-o',str(binary)],cwd=root,check=True)
env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1','UBSAN_OPTIONS':'halt_on_error=1'}
r=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN'],cwd=root,env=env,text=True,capture_output=True)
(out/'host.log').write_text(r.stdout+r.stderr);print(r.stdout,end='');print(r.stderr,end='')
report={'status':'PASS'if r.returncode==0 else'FAIL','exit':r.returncode,'compiler':'cc','sanitizers':['address','undefined'],'assertions':'enabled','sources':{s:hashlib.sha256((root/s).read_bytes()).hexdigest()for s in sources}}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(r.returncode)
