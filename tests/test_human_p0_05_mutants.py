#!/usr/bin/env python3
"""Compiled behavioral mutations; compile/runtime errors are NOT killed mutants."""
import argparse, json, os, subprocess, sys, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def mutations():
    return [
      ('relationship-priority','if(bit(a,O_REL_FLAGS,7)){\n        cat=0x11u;',
       'if(0){\n        cat=0x11u;','table'),
      ('flags34-wrong-bit','}else if(!bit(a,O_FLAGS34,5)){\n        unsigned half=',
       '}else if(!bit(a,O_FLAGS34,4)){\n        unsigned half=','all-flags'),
      ('fallback-gate-wrong-bit','if(!bit(a,O_REL_FLAGS,7)&&bit(a,O_FLAGS34,5)&&bit(a,O_FLAGS34,1))',
       'if(!bit(a,O_REL_FLAGS,7)&&bit(a,O_FLAGS34,5)&&bit(a,O_FLAGS34,0))','all-flags'),
      ('missing-no-action','if(!bit(a,O_REL_FLAGS,7)&&bit(a,O_FLAGS34,5)&&bit(a,O_FLAGS34,1))',
       'if(!bit(a,O_REL_FLAGS,7)&&bit(a,O_FLAGS34,5)&&bit(a,O_FLAGS34,1)&&0)','table'),
      ('wrong-relationship-category','cat=0x11u;\n        if(tr)tr->last_stage_rom=0x0000E002u;',
       'cat=0u;\n        if(tr)tr->last_stage_rom=0x0000E002u;','table'),
      ('wrong-fallback-category','cat=0u;\n        if(tr)tr->last_stage_rom=0x0000E4BAu;',
       'cat=1u;\n        if(tr)tr->last_stage_rom=0x0000E4BAu;','table'),
      ('missing-toggle','wf_set_u8(a,0x62u,(uint8_t)(wf_u8(a,0x62u)^0x80u));\n        /* $F178 retains',
       'wf_set_u8(a,0x62u,wf_u8(a,0x62u));\n        /* $F178 retains','table'),
      ('unwanted-remap','if(remap){s=e830_remap(a,&raw,tr);if(s!=WF_OK)return s;}',
       'if(remap||cat==0u||cat==0x11u){s=e830_remap(a,&raw,tr);if(s!=WF_OK)return s;}','no-remap'),
      ('unwanted-link-write','cat=0u;\n        if(tr)tr->last_stage_rom=0x0000E4BAu;',
       'cat=0u;wf_set_be32(a,O_LINK,wf_be32(a,O_TARGET));\n        if(tr)tr->last_stage_rom=0x0000E4BAu;','table'),
      ('reject-null-alias','if(!alt&&(wf_be32(a,O_TARGET)&0x00ffffffu)==0u&&(st==0u||st==1u)){',
       'if(!alt&&wf_be32(a,O_TARGET)==0u&&(st==0u||st==1u)){','table'),
      ('allow-invalid-target','if(!alt&&(wf_be32(a,O_TARGET)&0x00ffffffu)==0u&&(st==0u||st==1u)){',
       'if(!alt&&(wf_be32(a,O_TARGET)&0x00ffffffu)<0x001c0000u&&(st==0u||st==1u)){','guard'),
    ]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--only');a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    path=ROOT/'core/src/wf_r17_human_raw23.c';original=path.read_bytes();text=original.decode().replace('\r\n','\n');rows=[]
    selected=[m for m in mutations() if not a.only or m[0]==a.only];assert selected
    for name,old,new,group in selected:
        assert text.count(old)==1,(name,'ambiguous mutation',text.count(old))
        directory=out/name;directory.mkdir(exist_ok=True);source=directory/'mutant.c';source.write_text(text.replace(old,new))
        cmd=[sys.executable,str(ROOT/'tests/test_human_p0_05.py'),'--output',str(directory/'run'),'--source',str(source),'--group',group]
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
