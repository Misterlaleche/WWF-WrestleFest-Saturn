#!/usr/bin/env python3
"""Compile each CORE source once and run every test target in the shipped Makefile."""
import argparse, concurrent.futures, hashlib, json, pathlib, re, subprocess, sys
p=argparse.ArgumentParser();p.add_argument('runtime',type=pathlib.Path);p.add_argument('--cc',default='clang');p.add_argument('--sanitize',action='store_true');p.add_argument('--out',required=True,type=pathlib.Path);a=p.parse_args()
r=a.runtime.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
m=(r/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',m,re.M).group(1).split();tests=re.findall(r'^build/(test_[\w]+):',m,re.M)
flags=['-std=c11','-Wall','-Wextra','-Werror','-Iinclude', '-O1' if a.sanitize else '-O2']
if a.sanitize:flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
def command(cmd,name):
 q=subprocess.run(cmd,cwd=r,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (out/(name+'.log')).write_text('COMMAND '+repr(cmd)+'\n'+q.stdout+'\nEXIT '+str(q.returncode)+'\n')
 return q.returncode
assert command([a.cc,'--version'],'compiler')==0
def compile_one(s):
 obj=out/(s.replace('/','_')+'.o');return s,obj,command([a.cc,*flags,'-c',s,'-o',str(obj)],'compile-'+s.replace('/','_'))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: compiled=list(pool.map(compile_one,core))
if any(x[2] for x in compiled):sys.exit('Compilation failed; inspect logs')
objects=[str(x[1]) for x in compiled]
def run_test(t):
 binary=out/t;c=command([a.cc,*flags,*objects,'tests/'+t+'.c','-o',str(binary)],'link-'+t)
 status=c or command([str(binary)],t);return {'test':t,'exit':status}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:results=list(pool.map(run_test,tests))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
(out/'source-manifest.json').write_text(json.dumps({str(x.relative_to(r)):hashlib.sha256(x.read_bytes()).hexdigest() for d in ['src','include','generated','tests'] for x in sorted((r/d).rglob('*')) if x.is_file()},indent=2)+'\n')
print(json.dumps({'compiler':a.cc,'sanitize':a.sanitize,'tests':len(results),'failed':[x for x in results if x['exit']]}));sys.exit(any(x['exit'] for x in results))
