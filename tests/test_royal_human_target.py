#!/usr/bin/env python3
"""Actual Royal human-input path with target zero, compared with intact ROM.

The original target is in ROM: state FF, tier 0 and reset-vector X=0654.
The normal E33A category/action tables are still used.
Compare every cabinet byte and D register; no original code is patched.
ROM bus writes are ignored. Variants cover all 12 characters and fresh masks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import machine68k as mus
from run_tag_campaign_main import ROOT,sha

FIXTURE=ROOT/'tests/fixtures/royal_human_target'
SOURCE=ROOT/'core/src/wf_r17_human_raw23.c'
PREPASS=ROOT/'core/src/wf_actor_prepass.c'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/royal-pin-release/live-native')
    parser.add_argument('--output',type=Path,default=ROOT/'build/royal-human-target')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    manifest_path=build/'runtime-build.json';manifest=json.loads(manifest_path.read_text());assert not manifest['sources_changed_during_build']
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name not in ('wf_actor_prepass.o','wf_r17_human_raw23.o'):objects.append(str(build/name))
    binary=output/'facing-target';source_hash=sha(SOURCE);prepass_hash=sha(PREPASS)
    subprocess.run([os.environ.get('CC','cc'),*manifest['configuration']['flags'],str(ROOT/'tests/test_royal_facing_target.c'),str(SOURCE),str(PREPASS),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();regs=json.loads((FIXTURE/'state.json').read_text())['d'];actor=0x5b0
    assert raw[actor+0x7a:actor+0x7e]==bytes(4)
    cases=[('observed',raw)]
    for target in (0x80000000,0xff000000):
        data=bytearray(raw);data[actor+0x7a:actor+0x7e]=target.to_bytes(4,'big');cases.append((f'target-{target:08x}',bytes(data)))
    for state in (0x8000,0x8001):
        for ch in range(12):
            for half in (0,0x80):
                for fresh in (1,2,3):
                    data=bytearray(raw);data[actor+2:actor+4]=ch.to_bytes(2,'big')
                    data[actor+0x20:actor+0x22]=state.to_bytes(2,'big')
                    data[actor+0x62]=(data[actor+0x62]&0x7f)|half;data[actor+0xa3]=fresh
                    cases.append((f'state{state:x}-char{ch}-half{half}-fresh{fresh}',bytes(data)))
    for state in (0x8000,0x8001):
        for mask in (0,2):
            for fresh in (0x31,0x32,0x33):
                data=bytearray(raw);data[actor+0x20:actor+0x22]=state.to_bytes(2,'big')
                data[actor+0x34]=(data[actor+0x34]&~2)|mask;data[actor+0xa3]=fresh
                data[actor+0xaf]=1
                cases.append((f'state{state:x}-flag34-{mask}-fresh{fresh:x}-af1',bytes(data)))
    for state in (0x8001,):
        data=bytearray(raw);data[actor+0x20:actor+0x22]=state.to_bytes(2,'big')
        cases.append((f'state-{state:04x}',bytes(data)))
    for x,y in ((0x653,0),(0x654,0xffff),(0x655,0xfff0),(0x630,8)):
        for facing in (0x24,0xa4):
            data=bytearray(raw);data[actor+6:actor+8]=x.to_bytes(2,'big')
            data[actor+0xa:actor+0xc]=y.to_bytes(2,'big');data[actor+0x2e]=facing
            cases.append((f'xy-{x:x}-{y:x}-face-{facing:x}',bytes(data)))
    # P0-05: these previously guarded branches now require exact ROM continuation.
    for off,mask in ((0x34,0x20),(0x74,0x80)):
        data=bytearray(raw);data[actor+off]|=mask
        cases.append((f'closed-idle-flag-{off:x}-{mask:x}',bytes(data)))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;reports=[]
    mem.set_special_range_write_funcs(0,8,w8=lambda at,value:None,w16=lambda at,value:None,w32=lambda at,value:None)
    try:
        for name,data in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data);native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),'0',*map(str,regs)],text=True,capture_output=True)
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0xf18a);trace=[]
            while cpu.r_pc()!=0x70000:
                pc=cpu.r_pc();trace.append(pc);assert len(trace)<100000,(name,'ROM runaway',hex(pc));cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))
            (output/(name+'-oracle.bin')).write_bytes(expected);(output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes();assert actual==expected,(name,'state mismatch',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:16])
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),output_sha256=hashlib.sha256(actual).hexdigest()));print('PASS',name,flush=True) if len(reports)%25==0 or name=='observed' else None
        for target in (2,0x1c05b2,0x1c2000):
            name=f'unsupported-target-{target:x}'
            data=bytearray(raw);data[actor+0x7a:actor+0x7e]=target.to_bytes(4,'big')
            input_path=output/(name+'-input.bin');input_path.write_bytes(data);native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),'0',*map(str,regs)],text=True,capture_output=True)
            assert result.returncode==2 and 'dependency=ea42' in result.stdout,(name,result.stdout,result.stderr)
            assert native_path.read_bytes()[:0x4000]==data,(name,'unsupported target changed RAM')
            reports.append(dict(name=name,expected='blocked without RAM changes'));print('PASS',name,flush=True) if len(reports)%25==0 or name=='observed' else None
    finally:machine.cleanup()
    assert sha(SOURCE)==source_hash and sha(PREPASS)==prepass_hash,'source changed during test'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,source_sha256=source_hash,prepass_sha256=prepass_hash,dependency_manifest_sha256=sha(manifest_path),fixture=fixture,cases=reports),indent=2)+'\n')

if __name__=='__main__':main()
