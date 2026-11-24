#!/usr/bin/env python3
"""Test the actual SDK SMPC implementation with deterministic register/IRQ events.

Sega SMPC manual 3.1 requires IREG0 bit7 to toggle for continuation and expires
unfinished peripheral reads at VBlank-in:
https://www.infochunk.com/saturn/segahtml_en/hard/smpc/hon/p03_10.htm
Only includes, the SH2 ISR attribute, and empty busy-wait yields are adapted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=ROOT/'sdk/jo_engine/smpc.c')
    p.add_argument('--output',type=Path,default=ROOT/'build/sdk-smpc-test')
    p.add_argument('--cc',default='cc')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    raw=a.source.read_bytes();text=raw.decode()
    text=re.sub(r'^#include[^\n]*\n','',text,flags=re.M)
    text=text.replace('__attribute__((interrupt_handler))','')
    text,n=re.subn(r'(while \(__jo_smpc_intback_busy\)\s*)\;',r'\1host_poll();',text)
    assert n==1
    (a.output/'sdk_smpc.inc').write_text(text)
    binary=a.output/'test-sdk-smpc'
    cmd=[a.cc,'-std=gnu11','-O1','-g','-fms-extensions','-Wall','-Wextra','-Wno-int-to-pointer-cast',
         '-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',
         '-I'+str(ROOT/'sdk/jo_engine'),'-I'+str(a.output),str(ROOT/'tests/test_sdk_smpc.c'),'-o',str(binary)]
    subprocess.run(cmd,check=True)
    run=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
    report=dict(source_sha256=hashlib.sha256(raw).hexdigest(),command=cmd,returncode=run.returncode,
                stdout=run.stdout,stderr=run.stderr,instrumented_busy_waits=n)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(run.stdout+run.stderr,end='');raise SystemExit(run.returncode)

if __name__=='__main__':main()
