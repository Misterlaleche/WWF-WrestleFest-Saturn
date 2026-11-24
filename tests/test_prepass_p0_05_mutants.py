#!/usr/bin/env python3
"""Compile mutations of the F244 D1 prefix; all native core dependencies real."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);build=a.build_dir.resolve();path=ROOT/'core/src/wf_actor_prepass.c';original=path.read_bytes();s=original.decode().replace('\r\n','\n')
 line='wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)(i*4u));'
 cases=[('missing-d1',line,'wf_r17_m68k_move_w(&rt->m68k_regs,1u,(uint16_t)rt->m68k_regs.d[1]);'),('wrong-scale',line,'wf_r17_m68k_move_w(&rt->m68k_regs,1u,i);'),('clobber-high-word',line,'wf_r17_m68k_clr_l(&rt->m68k_regs,1u);'+line),('unmasked-index','uint16_t i=(uint16_t)(wf_be16(a,WF_A_AE)&0x003fu);','uint16_t i=wf_be16(a,WF_A_AE);')]
 rows=[]
 for name,old,new in cases:
  assert s.count(old)==1,(name,s.count(old));d=out/name;d.mkdir(exist_ok=True);m=d/'mutant.c';m.write_text(s.replace(old,new))
  cmd=[sys.executable,str(ROOT/'tests/test_human_p0_05.py'),'--build-dir',str(build),'--output',str(d/'run'),'--prepass-source',str(m),'--group','state1-dispatch']
  cp=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True);(d/'run.log').write_text(cp.stdout+cp.stderr)
  f=d/'run/report.json';r=json.loads(f.read_text()) if f.exists() else {}
  detected=cp.returncode==1 and r.get('failed',0)>0 and not r.get('exceptions') and not r.get('source_drift') and r.get('evaluated')==r.get('total')
  rows.append(dict(name=name,detected=detected,returncode=cp.returncode,failed=r.get('failed'),evaluated=r.get('evaluated'),total=r.get('total'),mutant_sha256=hashlib.sha256(m.read_bytes()).hexdigest()));print(json.dumps(rows[-1]),flush=True)
 assert path.read_bytes()==original
 report=dict(status='PASS' if all(r['detected'] for r in rows) else 'FAIL',total=len(rows),detected=sum(r['detected'] for r in rows),base_sha256=hashlib.sha256(original).hexdigest(),cases=rows)
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');return int(report['status']=='FAIL')
if __name__=='__main__':raise SystemExit(main())
