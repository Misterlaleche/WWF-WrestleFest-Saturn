#!/usr/bin/env python3
"""Compare state4 $1DA7E with intact ROM from a real TAG ringside failure.

The preserved frame1816 fixture came only from physical pad inputs. Variants
explicitly vary link tagging/subtype/mode to cover the same read-only route.
Full 16KiB game RAM and all eight D registers must match. No ROM instructions
are stubbed or patched, and ROM bytes must remain unchanged.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import machine68k as mus
from run_tag_campaign_main import ROOT,sha

FIXTURE=ROOT/'tests/fixtures/tag_cpu_link'

def put(data,offset,value,size):
    data[offset:offset+size]=value.to_bytes(size,'big')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/tag-campaign-20261113/cpu-runtime')
    parser.add_argument('--output',type=Path,default=ROOT/'build/tag-campaign-20261113/cpu-link')
    parser.add_argument('--source',type=Path,default=ROOT/'core/src/wf_cpu_runtime.c')
    parser.add_argument('--skip-build',action='store_true')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve()
    build.mkdir(parents=True,exist_ok=True);output.mkdir(parents=True,exist_ok=True)
    fixture_manifest=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture_manifest['files'].items():assert sha(FIXTURE/name)==digest
    if not args.skip_build:
        subprocess.run([sys.executable,str(ROOT/'tools/build_runtime_tests.py'),'--build-dir',str(build)],cwd=ROOT,check=True)
    manifest=json.loads((build/'runtime-build.json').read_text());assert not manifest['sources_changed_during_build']
    for name,digest in manifest['sources'].items():
        if name!='core/src/wf_cpu_runtime.c':assert sha(ROOT/name)==digest,('stale source',name)
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name!='wf_cpu_runtime.o':objects.append(str(build/name))
    flags=manifest['configuration']['flags'];binary=output/'cpu-link'
    command=[os.environ.get('CC','cc'),*flags,str(ROOT/'tests/test_tag_campaign_cpu.c'),str(args.source.resolve()),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)]
    subprocess.run(command,cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();metadata=json.loads((FIXTURE/'state.json').read_text());regs=metadata['d']
    actor=0x5b0+5*0x10c
    assert int.from_bytes(raw[actor+0x26:actor+0x2a],'big')==0x80000000
    cases=[('observed',raw)]
    for mode in (0,1,2):
        for sub in (8,9):
            for link in (0,0x80000000,0xff000000,0x1c06bc,0x801c06bc):
                variant=bytearray(raw);put(variant,0x161,mode,1);put(variant,actor+0x65,sub,1);put(variant,actor+0x26,link,4)
                variant[actor+0xb9]|=2
                cases.append((f'mode{mode}-sub{sub}-link{link:08x}',bytes(variant)))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;mem.w_block(0,rom)
    reports=[];source_hash=sha(args.source)
    try:
        for name,data in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data)
            native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),'5',*[str(r) for r in regs]],text=True,capture_output=True)
            mem.w_block(0x1c0000,data);cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_reg(8,0x1c0000+actor);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1da7e)
            trace=[]
            while cpu.r_pc()!=0x70000:
                trace.append(cpu.r_pc());assert len(trace)<10000,(name,'ROM runaway',hex(cpu.r_pc()))
                cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))
            (output/(name+'-oracle.bin')).write_bytes(expected)
            (output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes()
            assert actual==expected,(name,'RAM/register mismatch',[(hex(i+0x1c0000),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,flush=True)
    finally:machine.cleanup()
    assert sha(args.source)==source_hash,'CPU source changed during run'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,rom_sha256=hashlib.sha256(rom).hexdigest(),source_sha256=source_hash,binary_sha256=sha(binary),fixture=fixture_manifest,cases=reports),indent=2)+'\n')

if __name__=='__main__':main()
