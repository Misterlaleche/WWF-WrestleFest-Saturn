"""Rerun existing full-ROM owners after raw47 negative moved to raw35."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
names=['25','2b','36','46','5c','60','63','6e','6f','8a']
def run(name):
 script=HERE/f'action{name}_verify_rom.py';before=hashlib.sha256(script.read_bytes()).hexdigest()
 assert 'put(m,a+0x60,0x47)'not in script.read_text()and 'put(m,o+0x60,0x47)'not in script.read_text()
 target=out/f'action{name}.json';log=out/f'action{name}.log';assert not target.exists()and not log.exists(),'Use a fresh output directory'
 with log.open('w')as f:r=subprocess.run([sys.executable,str(script),'--report',str(target)],cwd=ROOT,stdout=f,stderr=subprocess.STDOUT)
 assert hashlib.sha256(script.read_bytes()).hexdigest()==before,('Oracle changed during run',name)
 result={'owner':name,'returncode':r.returncode,'oracle_sha256':before,'output':log.read_text()[-1500:]}
 if r.returncode==0:
  data=json.loads(target.read_bytes());result.update(cases=data['cases'],guards=data.get('guards',data.get('retained_guard_checks')),source_sha256=data['source_sha256'])
 return result
with concurrent.futures.ThreadPoolExecutor(max_workers=2)as pool:results=list(pool.map(run,names))
report={'status':'PASS'if all(r['returncode']==0 for r in results)else'FAIL','total_cases':sum(r.get('cases',0)for r in results),'total_guards':sum(r.get('guards',0)for r in results),'results':results,'limits':['Existing full-ROM oracle coverage and intercept scopes preserved','Only obsolete unowned-selector negative changed; raw47 has separate complete positive proof','No guard removed or expected failure converted to PASS']}
(out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
for r in results:print('owner',r['owner'],'returncode',r['returncode'],'cases',r.get('cases'),'guards',r.get('guards'))
sys.exit(0 if report['status']=='PASS'else 1)
