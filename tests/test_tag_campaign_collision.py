#!/usr/bin/env python3
"""Compare ringside relation release $24A90 with the intact 68000 ROM.

The natural TAG stage1/world6 frame1954 fixture is a post-block capture.
The first and only tested pair was restored on failure; it is not claimed
to be a whole-frame checkpoint. Callback tests compare all 16KiB RAM and D
registers. Full-scan replay compares all RAM, not untranslated scan registers.
No original instruction, actor outcome or collision response is stubbed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import machine68k as mus
from run_tag_campaign_main import ROOT,sha

FIXTURE=ROOT/'tests/fixtures/tag_collision_relation'
SOURCES=('core/src/wf_collision_dispatch.c','core/src/wf_r17_collision_24062.c')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/tag-campaign-20261113/cpu-replay-runtime')
    parser.add_argument('--output',type=Path,default=ROOT/'build/tag-collision-relation')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    manifest=json.loads((build/'runtime-build.json').read_text());assert not manifest['sources_changed_during_build']
    for name,digest in manifest['sources'].items():
        if name not in SOURCES:assert sha(ROOT/name)==digest,('stale source',name)
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name not in ('wf_collision_dispatch.o','wf_r17_collision_24062.o'):objects.append(str(build/name))
    binary=output/'collision-relation'
    source_hashes={name:sha(ROOT/name) for name in SOURCES}
    subprocess.run([os.environ.get('CC','cc'),*manifest['configuration']['flags'],
                    str(ROOT/'tests/test_tag_campaign_collision.c'),*[str(ROOT/name) for name in SOURCES],
                    str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();regs=json.loads((FIXTURE/'state.json').read_text())['d']
    source,target=1,5;actor=0x5b0+target*0x10c
    assert int.from_bytes(raw[actor+0x76:actor+0x7a],'big')==0x1c0f1c
    cases=[('observed','second',raw)]
    for link in (0x801c0f1c,0x1c1028,0x801c1028,0x1c05b0):
        data=bytearray(raw);data[actor+0x76:actor+0x7a]=link.to_bytes(4,'big')
        cases.append((f'relation-{link:08x}','second',bytes(data)))
    cases.append(('observed-scan','scan',raw))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    reports=[]
    try:
        for name,operation,data in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data)
            native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),operation,str(source),str(target),*map(str,regs)],text=True,capture_output=True)
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_reg(8,0x1c05b0+source*0x10c);cpu.w_reg(9,0x1c05b0+target*0x10c)
            cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x24a90 if operation=='second' else 0x24062)
            trace=[]
            while cpu.r_pc()!=0x70000:
                trace.append(cpu.r_pc());assert len(trace)<100000,(name,'ROM runaway',hex(cpu.r_pc()))
                cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))
            (output/(name+'-oracle.bin')).write_bytes(expected)
            (output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes();compared=0x4020 if operation=='second' else 0x4000
            assert actual[:compared]==expected[:compared],(name,'RAM/register mismatch',[(hex(i+0x1c0000),a,b) for i,(a,b) in enumerate(zip(actual[:compared],expected[:compared])) if a!=b][:12])
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),compared_bytes=compared,output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,result.stdout.strip(),flush=True)
    finally:machine.cleanup()
    # Invalid relation addresses must still block before any callback write;
    # the enclosing scanner must restore the first callback's pair changes.
    for operation in ('second','scan'):
        for link in (0,0x1c0f1d):
            data=bytearray(raw);data[actor+0x76:actor+0x7a]=link.to_bytes(4,'big')
            name=f'invalid-{operation}-{link:08x}'
            input_path=output/(name+'-input.bin');input_path.write_bytes(data)
            native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),operation,str(source),str(target),*map(str,regs)],text=True,capture_output=True)
            assert result.returncode==2,(name,'invalid relation accepted',result.stdout,result.stderr)
            assert native_path.read_bytes()[:0x4000]==data,(name,'blocked operation modified RAM')
            reports.append(dict(name=name,expected='blocked without RAM changes'))
            print('PASS',name,flush=True)
    for name,digest in source_hashes.items():assert sha(ROOT/name)==digest,'source changed during run'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,fixture=fixture,sources=source_hashes,binary_sha256=sha(binary),cases=reports),indent=2)+'\n')

if __name__=='__main__':main()
