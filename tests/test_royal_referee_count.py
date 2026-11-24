#!/usr/bin/env python3
"""Royal count completion from a natural pre-call snapshot, against intact ROM.

Compare all cabinet RAM, D registers, FG0 count tiles and sound hardware words.
Stop the original at $1F934, the same logic/render boundary as the native API.
Program-ROM writes are ignored by the memory bus, never by instruction skips.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import machine68k as mus
from run_tag_campaign_main import ROOT,sha

FIXTURE=ROOT/'tests/fixtures/royal_referee_count'
SOURCE=ROOT/'core/src/wf_r17_referee.c'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/royal-policy-search/native')
    parser.add_argument('--output',type=Path,default=ROOT/'build/royal-referee-count')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    manifest_path=build/'runtime-build.json';manifest=json.loads(manifest_path.read_text());assert not manifest['sources_changed_during_build']
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name!='wf_r17_referee.o':objects.append(str(build/name))
    binary=output/'referee-count';source_hash=sha(SOURCE)
    subprocess.run([os.environ.get('CC','cc'),*manifest['configuration']['flags'],str(ROOT/'tests/test_royal_referee_count.c'),str(SOURCE),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();regs=json.loads((FIXTURE/'state.json').read_text())['d']
    holder=int.from_bytes(raw[0x124a:0x124e],'big')-0x1c0000
    assert holder==0xbf8 and raw[holder+0x26:holder+0x2a]==bytes(4)
    cases=[('observed',raw)]
    for link in (0x80000000,0x100,0x80000100,0x7fe00,0x1c0aec,0x801c0aec):
        data=bytearray(raw);data[holder+0x26:holder+0x2a]=link.to_bytes(4,'big')
        cases.append((f'link-{link:08x}',bytes(data)))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    discarded=[]
    def rom_write(at,value):discarded.append((at,value))
    mem.set_special_range_write_funcs(0,8,w8=rom_write,w16=rom_write,w32=rom_write)
    reports=[]
    try:
        for name,data in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data);native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),str(ROOT/'source_assets/ROM68K.BIN'),*map(str,regs)],text=True,capture_output=True)
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);mem.w_block(0xc0000,bytes(0x4000));cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1f914)
            trace=[];sounds=[];discarded.clear()
            while cpu.r_pc()!=0x1f934:
                pc=cpu.r_pc();trace.append(pc);assert len(trace)<100000,(name,'ROM runaway',hex(pc))
                if pc==0x206c:sounds.append(cpu.r_reg(0)&0xffff)
                cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))+mem.r_block(0xc0000,0x4000)+b''.join(s.to_bytes(2,'big') for s in sounds)
            (output/(name+'-oracle.bin')).write_bytes(expected);(output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes();assert actual==expected,(name,'state mismatch',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:16],len(actual),len(expected))
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),discarded_rom_writes=list(discarded),output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,flush=True)
    finally:machine.cleanup()
    assert sha(SOURCE)==source_hash,'referee source changed during test'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,source_sha256=source_hash,dependency_manifest_sha256=sha(manifest_path),fixture=fixture,cases=reports),indent=2)+'\n')

if __name__=='__main__':main()
