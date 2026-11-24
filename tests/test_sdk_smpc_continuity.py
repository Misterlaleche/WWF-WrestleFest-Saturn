#!/usr/bin/env python3
"""Exercise actual SMPC, VBlank scheduling, input publication and RTC code."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
def function(source,name):
    match=re.search(r'^[^\n]*\b'+name+r'\([^\n]*\)\s*\n\{',source,re.M)
    assert match,name
    end=match.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]+'\n'
def main():
    p=argparse.ArgumentParser();p.add_argument('--sdk',type=Path,default=ROOT/'sdk/jo_engine');p.add_argument('--output',type=Path,required=True);p.add_argument('--case',choices=('all','cadence','rtc','publication'),default='all');p.add_argument('--mutations',action='store_true');a=p.parse_args()
    sdk=a.sdk.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    sources={n:(sdk/n).read_text() for n in ('smpc.c','core.c','tools.c','internal/wf_smpc_timing.h')}
    helper,n=re.subn(r'\*\(volatile unsigned (?:char|short) \*\)(0x[0-9a-f]+u)',r'host_mmio(\1)',sources['internal/wf_smpc_timing.h']);assert n==7
    smpc=sources['smpc.c'].replace('#include "internal/wf_smpc_timing.h"',helper)
    smpc=re.sub(r'^#include[^\n]*\n','',smpc,flags=re.M).replace('__attribute__((interrupt_handler))','')
    smpc,n=re.subn(r'(while \(__jo_smpc_intback_busy\)\s*)\;',r'\1host_poll();',smpc);assert n==1
    smpc,n=re.subn(r'(while \(__jo_smpc_intback_busy \|\| \(jo_smpc_read_byte\(StatusFlag\) & JO_SMPC_STATUS_FLAG_BUSY\)\)\s*)\;',r'\1host_poll();',smpc);assert n==1
    (out/'smpc.inc').write_text(smpc)
    core=function(sources['core.c'],'jo_core_vblank_in_handler').replace('__attribute__((interrupt_handler))','')
    core=core.replace('__jo_smpc_gun_vblank();','host_gun_vblank();')
    (out/'core.inc').write_text(core)
    inputs=(ROOT/'sdk/jo_engine/input.c').read_text()
    (out/'input.inc').write_text(function(inputs,'__jo_input_frame_start')+function(inputs,'jo_get_inputs_vblank'))
    (out/'rtc.inc').write_text(function(sources['tools.c'],'jo_bcd_to_int')+function(sources['tools.c'],'jo_getdate'))
    cmd=['cc','-std=gnu11','-O1','-g','-DWF_JO_ENGINE','-fms-extensions','-Wall','-Wextra','-Wno-unused-function','-Wno-int-to-pointer-cast','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'sdk/jo_engine'),'-I'+str(out),str(HERE/'test_sdk_smpc_continuity.c'),'-o',str(out/'test')]
    subprocess.run(cmd,check=True)
    r=subprocess.run([str(out/'test'),a.case],capture_output=True,text=True,timeout=30)
    receipt={'source_sha256':{n:hashlib.sha256((sdk/n).read_bytes()).hexdigest() for n in sources},'command':cmd,'case':a.case,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
    (out/'report.json').write_text(json.dumps(receipt,indent=2)+'\n');print(r.stdout+r.stderr,end='')
    if r.returncode:raise SystemExit(r.returncode)
    if a.mutations:
        mutations={
            'status_plus_peripheral':('smpc.c','jo_smpc_write_byte(InputRegister0, 0);','jo_smpc_write_byte(InputRegister0, JO_SMPC_INTBACK_SYSTEM_STATUS);','cadence'),
            'missing_status_irq_guard':('smpc.c','if (__jo_smpc_status_irq_pending)\n        return;','if (false)\n        return;','rtc'),
            'missing_rtc_refresh':('tools.c','    __jo_smpc_refresh_status();','    /* deliberately omitted for negative control */','rtc'),
            'late_music_order':('core.c','''    if (__vblank_callbacks.count)
        __jo_vblank_callbacks();
    __jo_smpc_vblank_in();
    if (__jo_core_music_vblank != JO_NULL)
        __jo_core_music_vblank();''','''    if (__jo_core_music_vblank != JO_NULL)
        __jo_core_music_vblank();
    if (__vblank_callbacks.count)
        __jo_vblank_callbacks();
    __jo_smpc_vblank_in();''','publication'),
        }
        rejected={}
        for name,(filename,old,new,case) in mutations.items():
            assert sources[filename].count(old)==1,(name,old)
            variant=out/'mutants'/name;variant_sdk=variant/'sdk'
            for n,body in sources.items():
                target=variant_sdk/n;target.parent.mkdir(parents=True,exist_ok=True)
                target.write_text(body.replace(old,new) if n==filename else body)
            result=subprocess.run([sys.executable,__file__,'--sdk',str(variant_sdk),'--output',str(variant/'run'),'--case',case],capture_output=True,text=True,timeout=90)
            assert result.returncode!=0 and 'Assertion' in result.stdout+result.stderr,(name,result.returncode,result.stdout,result.stderr)
            rejected[name]={'returncode':result.returncode,'failure':result.stdout+result.stderr}
        (out/'mutants/report.json').write_text(json.dumps(rejected,indent=2)+'\n')
        print('PASS four continuity/RTC/publication mutants rejected')
if __name__=='__main__':main()
