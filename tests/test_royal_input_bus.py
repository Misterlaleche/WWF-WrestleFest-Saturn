#!/usr/bin/env python3
"""Original $514E input phase at a naturally won Royal celebration.

Runs the real port/main.c input adapter and compares all 16KiB RAM plus
eight D registers with unmodified 68000 instructions. Cabinet ports and
read-only original ROM are separate bus regions, including 24-bit aliases.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import machine68k as mus
from run_tag_campaign_main import ROOT,sha,input_driver_sources

FIXTURE=ROOT/'tests/fixtures/royal_input_bus'
MAIN=ROOT/'port/main.c'


def flags():
    return ['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE',
            '-DWF_JO_ASSETS_HOST','-DWF_JO_FRONTEND_TEST','-DWF_JO_AUTORUN',
            '-DWF_JO_AUTORUN_MODE=2','-DWF_JO_FRONTEND','-DZ_SOLO','-DNO_GZIP',
            *['-I'+str(ROOT/p) for p in ('tests/native/jo_host','tests','core/include','port','port/zlib','tests/native')]]


def link(command,output):
    with (output/'link.log').open('w') as log:
        result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:print((output/'link.log').read_text())
    result.check_returncode()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/tag-policy-search/state5-fixed-native')
    parser.add_argument('--output',type=Path,default=ROOT/'build/royal-input-bus')
    parser.add_argument('--main-source',type=Path,default=MAIN)
    parser.add_argument('--capture',action='store_true')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((build/'runtime-build.json').read_text());assert not manifest['sources_changed_during_build']
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name);objects.append(str(build/name))
    wraps=['-Wl,--wrap=wf_jo_frontend_run','-Wl,--wrap=wf_jo_frontend_after_combat_at_level']
    sources=[str(ROOT/'tests/native/assets_fixture.c'),str(ROOT/'port/wf_jo_frontend.c'),str(ROOT/'port/wf_jo_clock.c')]
    if args.capture:
        FIXTURE.mkdir(parents=True,exist_ok=False)
        policy=ROOT/'tests/policies/royal_closer.h';binary=output/'royal-input-capture'
        source_files=[MAIN,ROOT/'tests/test_tag_campaign_main.c',ROOT/'tests/test_royal_input_bus.c',*input_driver_sources(policy)]
        hashes={str(p):sha(p) for p in source_files}
        link([os.environ.get('CC','cc'),*flags(),'-DWF_CAPTURE_ROYAL_INPUT',
              '-DWF_CAMPAIGN_INPUT_DRIVER="'+str(policy)+'"',*wraps,
              '-Wl,--wrap=wf_r18_ring_bind','-Wl,--wrap=wf_r18_post_tag_step',
              '-Wl,--wrap=wf_gameplay_bindings_build',str(ROOT/'tests/test_tag_campaign_main.c'),
              str(ROOT/'tests/test_royal_input_bus.c'),*sources,*objects,'-o',str(binary)],output)
        env=dict(os.environ,WF_CAPTURE_ROYAL_INPUT=str(FIXTURE),WF_POLICY_SWITCH_AT='12000',
                 WF_POLICY_PERIOD='4',WF_POLICY_SPACING='16',WF_POLICY_VARIANT='22',WF_POLICY_PIN_HEALTH='24',
                 WF_JO_TEST_CACHE_BYTES='565248')
        with (output/'capture.log').open('w') as stream:
            result=subprocess.run([str(binary),'1','60000',str(output),'0','0','0x1b4e'],cwd=ROOT/'cd',env=env,stdout=stream,stderr=subprocess.STDOUT)
        assert result.returncode==2,('expected original input blocker',result.returncode)
        assert all(sha(p)==hashes[str(p)] for p in source_files),'capture sources changed'
        fixture=dict(scope='Observer before real $514E at naturally won Royal tick13716; no runtime writes.',
                binary_sha256=sha(binary),sources=hashes,runtime_manifest_sha256=sha(build/'runtime-build.json'),
                environment={k:v for k,v in env.items() if k.startswith('WF_POLICY_')},
                files={name:sha(FIXTURE/name) for name in ('arcade-ram.bin','state.json')})
        (FIXTURE/'manifest.json').write_text(json.dumps(fixture,indent=2)+'\n')
        print('CAPTURED',FIXTURE);return
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    binary=output/'royal-input-bus';main_hash=sha(args.main_source)
    link([os.environ.get('CC','cc'),*flags(),'-DWF_JO_PROBE_MAIN="'+str(args.main_source.resolve())+'"',
          *wraps,str(ROOT/'tests/test_royal_input_bus.c'),*sources,*objects,'-o',str(binary)],output)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();meta=json.loads((FIXTURE/'state.json').read_text())
    regs,hardware=meta['d'],meta['hardware_low'];actor=0x5b0+4*0x10c
    assert raw[actor+0x8a:actor+0x8e]==bytes(4) and raw[0x16c4:0x16c6]==b'\0\4'
    cases=[('observed',raw,hardware)]
    for address in (0x80000000,0xff000000,0x100,0x6db0,0x7fffe,0x80006db0,0xff07fffe,
                    0x140020,0x140022,0x140024,0x140026,0x80140020):
        for pipeline in (0,1):
            data=bytearray(raw);data[actor+0x8a:actor+0x8e]=address.to_bytes(4,'big')
            # Original frame-counter low byte $0083 selects the A/B pipeline.
            data[0x83]=(data[0x83]&~1)|pipeline
            cases.append((f'bus-{address:08x}-pipe{pipeline}',bytes(data),[0x5a,0xa5,0x00,0xff]))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem;reports=[]
    try:
        for name,data,inputs in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data);native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),*map(str,regs),*map(str,inputs)],cwd=ROOT/'cd',text=True,capture_output=True)
            (output/(name+'-native-result.json')).write_text(json.dumps(dict(returncode=result.returncode,stdout=result.stdout,stderr=result.stderr),indent=2)+'\n')
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            for i,value in enumerate(inputs):mem.w16(0x140020+i*2,0xff00|value)
            cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x514e);trace=[]
            while cpu.r_pc()!=0x70000:
                trace.append(cpu.r_pc());assert len(trace)<10000,(name,'ROM runaway',hex(cpu.r_pc()));cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))
            (output/(name+'-oracle.bin')).write_bytes(expected)
            (output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes()
            assert actual==expected,(name,'RAM/register mismatch',[(hex(i+0x1c0000),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:16])
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,flush=True)
    finally:machine.cleanup()
    negative=[]
    for address in (1,0x80000,0x140021,0x140028,0x1c0000):
        data=bytearray(raw);data[actor+0x8a:actor+0x8e]=address.to_bytes(4,'big');path=output/(f'unsupported-{address:x}.bin');path.write_bytes(data)
        result=subprocess.run([str(binary),str(path),str(output/(f'unsupported-{address:x}-native.bin')),*map(str,regs),*map(str,hardware)],cwd=ROOT/'cd',text=True,capture_output=True)
        assert result.returncode==2 and 'dependency=517c' in result.stdout,(address,result)
        negative.append(hex(address))
    assert sha(args.main_source)==main_hash,'main source changed during test'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,fixture=fixture['files'],
          main_sha256=main_hash,binary_sha256=sha(binary),runtime_manifest_sha256=sha(build/'runtime-build.json'),
          rom_sha256=hashlib.sha256(rom).hexdigest(),cases=reports,unsupported_bus_controls=negative),indent=2)+'\n')


if __name__=='__main__':main()
