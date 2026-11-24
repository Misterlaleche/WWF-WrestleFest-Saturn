#!/usr/bin/env python3
"""Compiled behavioral mutations; compile/runtime errors are NOT killed mutants."""
import argparse, json, os, subprocess, sys, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def mutations():
    return [
      ('mash-unmasked-bus', 'physical=wf_be32(a,O_TARGET)&0x00FFFFFFu;', 'physical=wf_be32(a,O_TARGET);', 'mash'),
      ('mash-skipped-completion', 'if(!t&&physical!=0u)return block(tr,0x0000EBF6u);', 'if(!t)return WF_OK;', 'mash'),
      ('mash-wrong-zero-bit4', 'if(t&&!bit(t,O_FLAGS34,4)){', 'if(!t||!bit(t,O_FLAGS34,4)){', 'state4'),
      ('mash-wrong-threshold', 'if(wf_be16(a,O_MASH)!=0x4000u)return WF_OK;', 'if(wf_be16(a,O_MASH)<0x4000u)return WF_OK;', 'threshold'),
      ('mash-not-reset', 'wf_set_be16(a,O_SELECTOR,out);wf_set_be16(a,O_MASH,0u);', 'wf_set_be16(a,O_SELECTOR,out);wf_set_be16(a,O_MASH,0x4000u);', 'mash'),
      ('alt-pointer-tag-lost', 'wf_set_be32(a,O_LINK,pointer);', 'wf_set_be32(a,O_LINK,physical);', 'alternate'),
      ('alt-missing-link', 'wf_set_be32(a,O_LINK,pointer);', '(void)pointer;', 'alternate'),
      ('alt-unconditional-reciprocal', 'if(t&&wf_be32(t,O_LINK)==0u)wf_set_be32(t,O_LINK,actor_ptr(actor_slot));', 'if(t)wf_set_be32(t,O_LINK,actor_ptr(actor_slot));', 'actor-target'),
      ('alt-wrong-category', '*category=0x13u;*target_out=t?t:&rom_zero_read_view;', '*category=0u;*target_out=t?t:&rom_zero_read_view;', 'alternate'),
      ('alt-wrong-AF-gate', 'if(st==1u&&wf_u8(a,O_AF)==1u){', 'if(st==1u&&wf_u8(a,O_AF)==2u){', 'prefix'),
      ('alt-invalid-address-accepted', 'if(!t&&physical!=0u)return block(tr,0x0000E496u);', 'if(!t&&physical!=0u)return WF_OK;', 'guard'),
      ('character-word-truncated', '    ch=wf_be16(a,O_CHAR);\n    if(ch>=12u||cat>=21u)', '    ch=(uint8_t)wf_be16(a,O_CHAR);\n    if(ch>=12u||cat>=21u)', 'char-guard'),
    ]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--only');a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    path=ROOT/'core/src/wf_r17_human_raw23.c';original=path.read_bytes();text=original.decode().replace('\r\n','\n');rows=[]
    selected=[m for m in mutations() if not a.only or m[0]==a.only];assert selected
    for name,old,new,group in selected:
        assert text.count(old)==1,(name,'ambiguous mutation',text.count(old))
        directory=out/name;directory.mkdir(exist_ok=True);source=directory/'mutant.c';source.write_text(text.replace(old,new))
        cmd=[sys.executable,str(ROOT/'tests/test_human_p0_06.py'),'--output',str(directory/'run'),'--source',str(source),'--group',group]
        cp=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True);(directory/'run.log').write_text(cp.stdout+cp.stderr)
        reportfile=directory/'run/report.json';r=json.loads(reportfile.read_text()) if reportfile.exists() else {}
        killed=(cp.returncode==1 and r.get('status')=='FAIL' and r.get('failed',0)>0 and not r.get('exceptions')
                and not r.get('source_drift') and r.get('evaluated')==r.get('total'))
        rows.append(dict(name=name,detected=killed,returncode=cp.returncode,group=group,failed=r.get('failed'),
                         evaluated=r.get('evaluated'),total=r.get('total'),mutant_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                         report=str(reportfile.relative_to(out))))
        print(json.dumps(rows[-1]),flush=True)
    assert path.read_bytes()==original
    report=dict(status='PASS' if all(r['detected'] for r in rows) else 'FAIL',total=len(rows),detected=sum(r['detected'] for r in rows),
                base_sha256=hashlib.sha256(original).hexdigest(),compiler=os.environ.get('CC','cc'),cases=rows)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return int(report['status']=='FAIL')
if __name__=='__main__':raise SystemExit(main())
