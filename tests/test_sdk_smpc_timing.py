#!/usr/bin/env python3
"""Run the complete Wrestlefest SMPC implementation against a beam/FRT model.

Only includes, ISR attributes, MMIO reads, CPU masks and empty wait yields are
adapted. PAL/NTSC, display on/off, both CPU clocks, timer wrap, inherited video
modes and stale INTBACK races exercise the actual production control flow.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk-root',type=Path,default=ROOT/'sdk/jo_engine')
    p.add_argument('--output',type=Path,default=ROOT/'build/sdk-smpc-timing')
    p.add_argument('--cc',default='cc')
    p.add_argument('--mutations',action='store_true',help='Also reject removal of the four critical guards')
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);sdk=a.sdk_root.resolve()
    source=(sdk/'smpc.c').read_text();helper=(sdk/'internal/wf_smpc_timing.h').read_text()
    helper,n=re.subn(r'\*\(volatile unsigned (?:char|short) \*\)(0x[0-9a-f]+u)',r'host_mmio(\1)',helper)
    assert n==7,n
    source=source.replace('#include "internal/wf_smpc_timing.h"',helper)
    source=re.sub(r'^#include[^\n]*\n','',source,flags=re.M).replace('__attribute__((interrupt_handler))','')
    source,n=re.subn(r'(while \(__jo_smpc_intback_busy\)\s*)\;',r'\1host_poll();',source);assert n==1
    source,n=re.subn(r'(while \(__jo_smpc_intback_busy \|\| \(jo_smpc_read_byte\(StatusFlag\) & JO_SMPC_STATUS_FLAG_BUSY\)\)\s*)\;',r'\1host_poll();',source);assert n==1
    (out/'sdk_smpc_timing.inc').write_text(source)
    cmd=[a.cc,'-std=gnu11','-O1','-g','-DWF_JO_ENGINE','-fms-extensions','-Wall','-Wextra',
         '-Wno-unused-function','-Wno-int-to-pointer-cast','-fsanitize=address,undefined','-fno-sanitize-recover=all',
         '-I'+str(sdk),'-I'+str(ROOT/'sdk/jo_engine'),'-I'+str(out),str(HERE/'test_sdk_smpc_timing.c'),'-o',str(out/'test-sdk-smpc-timing')]
    subprocess.run(cmd,check=True)
    result=subprocess.run([cmd[-1]],capture_output=True,text=True,timeout=90)
    report={'command':cmd,'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr,
            'source_sha256':hashlib.sha256((sdk/'smpc.c').read_bytes()).hexdigest(),
            'helper_sha256':hashlib.sha256((sdk/'internal/wf_smpc_timing.h').read_bytes()).hexdigest()}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(result.stdout+result.stderr,end='')
    if result.returncode:raise SystemExit(result.returncode)
    if a.mutations:
        mutations={
            'missing_intback_guard':('if (!wf_smpc_wait_intback())','if (0)'),
            'missing_control_window':('int window=wf_smpc_control_window();','int window=1;'),
            'ignored_external_latch':('if(exten&0x0200u)','if(0)'),
            'late_write_window':('line<=0x1fc','line<=0x1fe'),
        }
        rejected={}
        for name,(old,new) in mutations.items():
            assert source.count(old)==1,(name,old)
            variant=out/'mutants'/name;variant.mkdir(parents=True,exist_ok=True)
            (variant/'sdk_smpc_timing.inc').write_text(source.replace(old,new))
            mutation_cmd=[('-I'+str(variant)) if x=='-I'+str(out) else x for x in cmd]
            mutation_cmd[-1]=str(variant/'test-sdk-smpc-timing')
            subprocess.run(mutation_cmd,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            r=subprocess.run([mutation_cmd[-1]],capture_output=True,text=True,timeout=90)
            assert r.returncode!=0 and 'Assertion' in r.stderr,(name,r.returncode,r.stderr)
            rejected[name]={'returncode':r.returncode,'failure':r.stderr.strip()}
        (out/'mutants/report.json').write_text(json.dumps(rejected,indent=2)+'\n')
        print('PASS four semantic mutants rejected by runtime assertions')

if __name__=='__main__':main()
