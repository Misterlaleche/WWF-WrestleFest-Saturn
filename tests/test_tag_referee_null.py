#!/usr/bin/env python3
"""TAG referee countdown expiry from a natural pre-call snapshot, against intact ROM.

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

FIXTURE=ROOT/'tests/fixtures/tag_referee_null'
SOURCE=ROOT/'core/src/wf_r17_referee.c'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/tag-policy-search/state5-fixed-native')
    parser.add_argument('--source',type=Path,default=SOURCE)
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--output',type=Path,default=ROOT/'build/tag-referee-null/differential')
    args=parser.parse_args();source=args.source.resolve();build=args.build_dir.resolve();output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    manifest_path=build/'runtime-build.json';manifest=json.loads(manifest_path.read_text());assert not manifest['sources_changed_during_build']
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name!='wf_r17_referee.o':objects.append(str(build/name))
    binary=output/'referee-null';source_hash=sha(source)
    sanitizer=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if args.sanitize else []
    subprocess.run([os.environ.get('CC','cc'),*manifest['configuration']['flags'],*sanitizer,str(ROOT/'tests/test_tag_referee_null.c'),str(source),str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();regs=json.loads((FIXTURE/'state.json').read_text())['d']
    assert raw[0x1682:0x1688]==bytes.fromhex('000100000000')
    assert rom[6:8]==bytes.fromhex('0654') and rom[10:12]==b'\xff\xff'
    assert rom[0x21]==0xff and rom[0xfe:0x100]==b'\xff\xff'
    cases=[('observed',raw,regs)]
    for pointer in (0x80000000,0xff000000,0x1c05b0,0x801c05b0):
        data=bytearray(raw);data[0x1684:0x1688]=pointer.to_bytes(4,'big')
        cases.append((f'pointer-{pointer:08x}',bytes(data),regs))
    for x in (0x100,0x280,0x604,0x680):
        data=bytearray(raw);data[0x11fa:0x11fc]=x.to_bytes(2,'big')
        cases.append((f'x-{x:04x}',bytes(data),regs))
    data=bytearray(raw);data[0x1222]=0x7f
    cases.append(('facing-lower-bits',bytes(data),regs))
    high_regs=[value|0xbeef0000 if i==1 else value for i,value in enumerate(regs)]
    cases.append(('word-register-upper-bits',raw,high_regs))
    # Synthetic direct action boundaries supplement the unmodified real input.
    # Primary9 is inert; the public action latch drives each approach variant.
    for action in (1,2,4):
        for pointer in (0,0x1c05b0):
            data=bytearray(raw)
            data[0x1214:0x1216]=(9).to_bytes(2,'big')
            data[0x1210:0x1212]=action.to_bytes(2,'big')
            data[0x124a:0x124e]=pointer.to_bytes(4,'big')
            cases.append((f'approach-{action}-pointer-{pointer:08x}',bytes(data),high_regs))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    discarded=[]
    def rom_write(at,value):discarded.append((at,value))
    mem.set_special_range_write_funcs(0,8,w8=rom_write,w16=rom_write,w32=rom_write)
    reports=[]
    try:
        for name,data,case_regs in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data);native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),str(ROOT/'source_assets/ROM68K.BIN'),*map(str,case_regs)],text=True,capture_output=True)
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);mem.w_block(0xc0000,bytes(0x4000));cpu.w_sr(0x2700)
            for i,r in enumerate(case_regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1f914)
            trace=[];sounds=[];discarded.clear()
            while cpu.r_pc()!=0x1f934:
                pc=cpu.r_pc();trace.append(pc);assert len(trace)<100000,(name,'ROM runaway',hex(pc))
                if pc==0x206c:sounds.append(cpu.r_reg(0)&0xffff)
                cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))+mem.r_block(0xc0000,0x4000)+b''.join(s.to_bytes(2,'big') for s in sounds)
            (output/(name+'-oracle.bin')).write_bytes(expected);(output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            (output/(name+'-native.log')).write_text(result.stdout+result.stderr)
            if result.returncode:
                (output/'failure.json').write_text(json.dumps(dict(name=name,returncode=result.returncode,stdout=result.stdout,stderr=result.stderr,source_sha256=source_hash,oracle_instructions=len(trace)),indent=2)+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes();assert actual==expected,(name,'state mismatch',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:16],len(actual),len(expected))
            assert mem.r_block(0,len(rom))==rom
            if name in ('observed','observed-next1','observed-next2'):
                number=1 if name=='observed' else int(name[-1])+1
                cases.append((f'observed-next{number}',expected[:0x4000],[int.from_bytes(expected[0x4000+i*4:0x4004+i*4],'big') for i in range(8)]))
            reports.append(dict(name=name,instructions=len(trace),discarded_rom_writes=list(discarded),output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,flush=True)
    finally:machine.cleanup()
    guards=[]
    for pointer in (1,0x1c0000,0x1c0500,0x80000001):
        data=bytearray(raw);data[0x1684:0x1688]=pointer.to_bytes(4,'big')
        name=f'guard-{pointer:08x}';input_path=output/(name+'-input.bin');input_path.write_bytes(data)
        native_path=output/(name+'-native.bin')
        result=subprocess.run([str(binary),str(input_path),str(native_path),str(ROOT/'source_assets/ROM68K.BIN'),*map(str,regs)],text=True,capture_output=True)
        assert result.returncode==2 and 'dependency=1c1684' in result.stdout,(name,result.returncode,result.stdout,result.stderr)
        expected=bytearray(data);expected[0x1682:0x1684]=bytes(2);expected[0x1214:0x1216]=bytes.fromhex('8002')
        actual=native_path.read_bytes()
        assert actual[:0x4000]==expected,(name,'unsupported pointer mutated beyond existing guard')
        assert actual[0x4000:0x4020]==bytes(4)+b''.join(r.to_bytes(4,'big') for r in regs[1:])
        assert actual[0x4020:]==bytes(0x4000)
        guards.append(dict(pointer=hex(pointer),dependency='0x1c1684',unchanged_link=True))
    assert sha(source)==source_hash,'referee source changed during test'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,source_sha256=source_hash,sanitized_referee_and_harness=args.sanitize,dependency_manifest_sha256=sha(manifest_path),fixture_manifest_sha256=sha(FIXTURE/'manifest.json'),cases=reports,unsupported_pointer_guards=guards),indent=2)+'\n')

if __name__=='__main__':main()
