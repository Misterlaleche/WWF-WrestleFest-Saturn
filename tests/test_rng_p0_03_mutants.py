#!/usr/bin/env python3
"""Behavioral mutations of P0-03; source copies only, production is never edited.

A detected mutant must compile and run normally, then differ in the original-ROM
comparison. A compiler error, crash, timeout or interpreter error is not a PASS.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'core/src/wf_r17_rng1129e.c'
MUTATIONS = [
 ('restore-null-block', b'if(!p&&(target&0x00ffffffu))return blocked(tr,0x000112C8u);',
  b'if(!p)return blocked(tr,0x000112C8u);'),
 ('reject-high-byte-alias', b'if(!p&&(target&0x00ffffffu))return blocked(tr,0x000112C8u);',
  b'if(!p&&target)return blocked(tr,0x000112C8u);'),
 ('wrong-null-probability-table', b'if(!p||bit(p,O_FLAGS56,7)){',
  b'if(p&&bit(p,O_FLAGS56,7)){'),
 ('skip-rng-advance', b'out=wf_r17_rng21b4_step(rng->state_1c005c,rng->global_1c0080,d);',
  b'out=*rng->state_1c005c;'),
 ('carry-inclusive-instead-of-strict', b'(out&0x000Fu)<(uint16_t)threshold',
  b'(out&0x000Fu)<=(uint16_t)threshold'),
 ('accept-unmapped-nonzero-pointer', b'if(!p&&(target&0x00ffffffu))return blocked(tr,0x000112C8u);',
  b'if(0)return blocked(tr,0x000112C8u);'),
]

def main():
 p = argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output', type=Path, required=True)
 args=p.parse_args(); out=args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
 original=SOURCE.read_bytes(); runs=[]
 from test_resolver_rng_p0_03 import cases_for
 expected_cases=len(cases_for('rng'))
 for name, old, new in MUTATIONS:
  if old not in original: raise AssertionError(('mutation site missing', name))
  folder=out/name; folder.mkdir(exist_ok=True)
  mutant=folder/'rng-mutant.c'; mutant.write_bytes(original.replace(old,new,1))
  cmd=[sys.executable,str(ROOT/'tests/test_resolver_rng_p0_03.py'), '--group','rng',
       '--rng-source',str(mutant),'--sanitize','--output',str(folder/'run')]
  with (folder/'run.log').open('w') as log:
   cp=subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=180)
  rp=folder/'run/report.json'; report=json.loads(rp.read_text()) if rp.exists() else {}
  detected=(cp.returncode==1 and report.get('status')=='FAIL' and
            report.get('evaluated')==expected_cases and report.get('failed',0)>0 and
            not report.get('exceptions') and not report.get('source_drift'))
  runs.append(dict(name=name,detected=detected,returncode=cp.returncode,
      mismatches=report.get('failed'),evaluated=report.get('evaluated'),
      mutant_sha256=hashlib.sha256(mutant.read_bytes()).hexdigest(),command=cmd))
  print(name,'DETECTED' if detected else 'FAIL',report.get('failed'),flush=True)
  (out/'report.json').write_text(json.dumps({'status':'RUNNING','runs':runs},indent=2)+'\n')
 if SOURCE.read_bytes()!=original:raise AssertionError('production source changed')
 report=dict(status='PASS_MUTATION_DETECTION' if all(x['detected'] for x in runs) else 'FAIL',
    production_sha256=hashlib.sha256(original).hexdigest(),total=len(runs),
    detected=sum(x['detected'] for x in runs),runs=runs,
    limitation='Same limited opcode interpreter as the main test; not independent CPU verification')
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return int(report['status']=='FAIL')
if __name__=='__main__':raise SystemExit(main())
