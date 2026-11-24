#!/usr/bin/env python3
"""P0-04 mutation gates. Each detected mutant must compile, run and produce
an actual behavioral mismatch. Crashes/compile errors do not count as detection.
The production file is never edited. --start/--end allow bounded local runs.
"""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
from test_human_p0_04 import ROOT,cases_for,sha
SOURCE=ROOT/'core/src/wf_r17_human_raw23.c'
MUTANTS=[
 ('omit-error-trace-fallback',b'if(!tr)tr=&local_trace;',b'(void)local_trace;','guard'),
 ('restore-primary-null-guard',b'if(!(st==5u&&(wf_be32(a,O_TARGET)&0x00FFFFFFu)==0u)){',b'if(1){','linked'),
 ('use-primary-instead-of-link',b'uint32_t physical=wf_be32(a,O_LINK)&0x00FFFFFFu;',b'uint32_t physical=wf_be32(a,O_TARGET)&0x00FFFFFFu;','linked'),
 ('reject-tagged-link',b'uint32_t physical=wf_be32(a,O_LINK)&0x00FFFFFFu;',b'uint32_t physical=wf_be32(a,O_LINK);','linked'),
 ('toggle-bit7-not-tier',b'mask=(uint8_t)(1u<<tier);',b'mask=0x80u;','linked'),
 ('use-new-not-previous-bit',b'old=(value&mask)!=0u;',b'old=(value&mask)==0u;','linked'),
 ('leave-timer44',b'wf_set_be16(a,O_TIMER44,0u);wf_set_be16(a,0x46u,0u);*d3_nonzero=1;',b'wf_set_be16(a,0x46u,0u);*d3_nonzero=1;','linked'),
 ('leave-timer46',b'wf_set_be16(a,O_TIMER44,0u);wf_set_be16(a,0x46u,0u);*d3_nonzero=1;',b'wf_set_be16(a,O_TIMER44,0u);*d3_nonzero=1;','linked'),
 ('skip-remap',b'wf_set_be16(a,O_TIMER44,0u);wf_set_be16(a,0x46u,0u);*d3_nonzero=1;',b'wf_set_be16(a,O_TIMER44,0u);wf_set_be16(a,0x46u,0u);*d3_nonzero=0;','rom-link'),
 ('accept-unmapped-link',b'if(physical!=0u)return block(tr,0x0000E1D4u);',b'if(0)return block(tr,0x0000E1D4u);','guard'),
 ('truncate-character-word',b'tier=wf_be16(link,O_TIER);ch=wf_be16(a,O_CHAR);',b'tier=wf_be16(link,O_TIER);ch=(uint8_t)wf_be16(a,O_CHAR);','guard'),
]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--start',type=int,default=0);p.add_argument('--end',type=int,default=len(MUTANTS));a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);original=SOURCE.read_bytes();rows=[]
    for name,old,new,group in MUTANTS[a.start:a.end]:
        start=0 if name in ('restore-primary-null-guard','omit-error-trace-fallback') else original.index(b'static wf_status_t classify_e1d4')
        end=len(original) if start==0 else original.index(b'/* Ordered $E002',start)
        region=original[start:end];assert region.count(old)==1,(name,'missing or ambiguous site')
        mutated=original[:start]+region.replace(old,new,1)+original[end:]
        folder=out/name;folder.mkdir(exist_ok=True);source=folder/'human-mutant.c';source.write_bytes(mutated)
        cmd=[sys.executable,str(ROOT/'tests/test_human_p0_04.py'),'--output',str(folder/'run'),'--source',str(source),'--group',group]
        if name=='omit-error-trace-fallback':cmd+=['--no-trace']
        with (folder/'run.log').open('w') as log:cp=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=180)
        rp=folder/'run/report.json';r=json.loads(rp.read_text()) if rp.exists() else {};expected=sum(c['group']==group for c in cases_for())
        detected=cp.returncode==1 and r.get('evaluated')==expected and r.get('failed',0)>0 and not r.get('exceptions') and not r.get('source_drift')
        row=dict(name=name,detected=detected,returncode=cp.returncode,failed=r.get('failed'),evaluated=r.get('evaluated'),group=group,command=cmd,source_sha256=sha(source.read_bytes()))
        (folder/'result.json').write_text(json.dumps(row,indent=2)+'\n');rows.append(row);print(name,'DETECTED' if detected else 'NOT_DETECTED',row['failed'],flush=True)
        assert SOURCE.read_bytes()==original
    allrows=[]
    for name,*_ in MUTANTS:
        p=out/name/'result.json'
        if p.exists():allrows.append(json.loads(p.read_text()))
    full=len(allrows)==len(MUTANTS);ok=all(r['detected'] for r in allrows)
    (out/'report.json').write_text(json.dumps(dict(status=('PASS_MUTATION_DETECTION' if ok else 'FAIL') if full else 'PARTIAL',total_expected=len(MUTANTS),executed=len(allrows),detected=sum(r['detected'] for r in allrows),production_sha256=sha(original),cases=allrows),indent=2)+'\n')
    return int(not all(r['detected'] for r in rows))
if __name__=='__main__':raise SystemExit(main())
