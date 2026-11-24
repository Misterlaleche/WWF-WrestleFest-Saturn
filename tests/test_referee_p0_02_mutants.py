#!/usr/bin/env python3
"""Each temporary regression must compile/run and fail behavioral comparisons.

No mutation is applied to the production file. Reports distinguish a detected
regression from a build failure, timeout or unusable oracle.
"""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--rom',type=Path,default=ROOT/'source_assets/ROM68K.BIN');args=p.parse_args()
 source=ROOT/'core/src/wf_r17_referee.c';before=source.read_bytes();text=source.read_text()
 mutations=[
 ('reject-primary-null', 'if((rt->g.global_1688&0x00ffffffu)&&!aptr(rt,rt->g.global_1688))return block(c,0x001C1688u);', 'if(!aptr(rt,rt->g.global_1688))return block(c,0x001C1688u);'),
 ('normalize-register-compare','if(wf_be32(r,R_LINK)==rt->g.global_1688)','if((wf_be32(r,R_LINK)&0x00ffffffu)==rt->g.global_1688)'),
 ('reject-count-null','if(!a)return (pointer&0x00ffffffu)?block(c,dependency):WF_OK;','if(!a)return block(c,dependency);'),
 ('reject-link-null','if(!a&&(wf_be32(r,R_LINK)&0x00ffffffu))return block(c,0x000202D6u);','if(!a)return block(c,0x000202D6u);'),
 ('clear-only-facing-bit','wf_set_u8(r,R_FACING,0u);x=a?','clrbit(r,R_FACING,7);x=a?'),
 ('omit-word-register-write','wf_r17_m68k_move_w(&rt->m68k_regs,0u,x);\n  if(x>=','(void)x;\n  if(x>='),
 ('clobber-upper-register','wf_r17_m68k_move_w(&rt->m68k_regs,0u,x);\n  if(x>=','wf_r17_m68k_move_l(&rt->m68k_regs,0u,x);\n  if(x>='),
 ('wrong-target-flag','clrbit(a,WF_A_FLAGS34,0);\n return WF_OK;','setbit(a,WF_A_FLAGS34,0);\n return WF_OK;'),
 ('hide-primary-diagnostic','if((rt->g.global_1688&0x00ffffffu)&&!aptr(rt,rt->g.global_1688))return block(c,0x001C1688u);','if((rt->g.global_1688&0x00ffffffu)&&!aptr(rt,rt->g.global_1688))return WF_OK;'),
 ('hide-count-diagnostic','if(!a)return (pointer&0x00ffffffu)?block(c,dependency):WF_OK;','if(!a){(void)c;(void)dependency;return WF_OK;}'),
 ]
 out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);runs=[]
 for name,old,new in mutations:
  assert text.count(old)==1,(name,text.count(old))
  folder=out/name;folder.mkdir(exist_ok=True);mutant=folder/'referee.c';mutant.write_text(text.replace(old,new))
  cmd=[sys.executable,str(ROOT/'tests/test_referee_p0_02.py'),'--rom',str(args.rom),'--source',str(mutant),'--output',str(folder)]
  with (folder/'run.log').open('w') as log: r=subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=60)
  report=json.loads((folder/'report.json').read_text()) if (folder/'report.json').exists() else {}
  detected=r.returncode==1 and report.get('status')=='FAIL' and report.get('failed',0)>0
  runs.append(dict(name=name,detected=detected,returncode=r.returncode,failures=report.get('failed'),
                   source_sha256=hashlib.sha256(mutant.read_bytes()).hexdigest(),inputs_sha256=report.get('inputs_sha256')))
  print(name, 'DETECTED' if detected else 'NOT_PROVEN',report.get('failed'),flush=True)
 assert source.read_bytes()==before,'production source modified during negative controls'
 summary=dict(status='PASS' if all(r['detected'] for r in runs) else 'FAIL',source_sha256=hashlib.sha256(before).hexdigest(),runs=runs)
 (out/'report.json').write_text(json.dumps(summary,indent=2)+'\n')
 return summary['status']!='PASS'
if __name__=='__main__':raise SystemExit(main())
