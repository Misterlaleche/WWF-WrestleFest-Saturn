#!/usr/bin/env python3
"""Differential Royal raw49 release against the intact 68000 dispatcher.

The fixture is observed before actor5's real callback at Royal tick9803,
reached only by physical pad inputs. ROM is mapped read-only in the oracle:
attempted writes are recorded, as on the arcade bus, without changing bytes.
All 16KiB game RAM and eight live D registers must agree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import machine68k as mus
from run_tag_campaign_main import ROOT,sha

FIXTURE=ROOT/'tests/fixtures/royal_pin_release'
SOURCE=ROOT/'core/src/wf_r17_actions_r11.c'


def capture(build,output,manifest,objects):
    FIXTURE.mkdir(parents=True,exist_ok=True)
    assert not any(FIXTURE.iterdir()), 'Never overwrite a preserved fixture'
    policy=ROOT/'tests/policies/royal_pressure.h'
    binary=output/'royal-pin-capture'
    flags=['-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-DWF_JO_ENGINE',
           '-DWF_JO_ASSETS_HOST','-DWF_JO_FRONTEND_TEST','-DWF_JO_AUTORUN',
           '-DWF_JO_AUTORUN_MODE=2','-DWF_JO_FRONTEND','-DZ_SOLO','-DNO_GZIP',
           '-DWF_CAPTURE_ROYAL_PIN','-DWF_CAMPAIGN_INPUT_DRIVER="'+str(policy)+'"']
    flags+=['-I'+str(ROOT/p) for p in ('tests/native/jo_host','tests','core/include','port','port/zlib','tests/native')]
    command=[os.environ.get('CC','cc'),*flags,
             '-Wl,--wrap=wf_jo_frontend_run','-Wl,--wrap=wf_jo_frontend_after_combat_at_level',
             '-Wl,--wrap=wf_r18_ring_bind','-Wl,--wrap=wf_r18_post_tag_step','-Wl,--wrap=wf_r17_dispatch_1c03e_r11',
             str(ROOT/'tests/test_tag_campaign_main.c'),str(ROOT/'tests/test_royal_pin_release.c'),
             str(ROOT/'tests/native/assets_fixture.c'),str(ROOT/'port/wf_jo_frontend.c'),
             str(ROOT/'port/wf_jo_clock.c'),*objects,str(build/'wf_r17_actions_r11.o'),'-o',str(binary)]
    subprocess.run(command,cwd=ROOT,check=True)
    env=dict(os.environ,WF_CAPTURE_ROYAL_PIN=str(FIXTURE),WF_POLICY_PERIOD='4',
             WF_POLICY_SPACING='16',WF_POLICY_VARIANT='3',WF_POLICY_PIN_HEALTH='255',
             WF_JO_TEST_CACHE_BYTES='565248')
    with (output/'capture.log').open('w') as stream:
        result=subprocess.run([str(binary),'1','60000',str(output),'0','0','0x1b4e'],cwd=ROOT/'cd',env=env,stdout=stream,stderr=subprocess.STDOUT)
    assert result.returncode==2,('expected original blocked route',result.returncode)
    fixture=dict(scope='Pre-callback observer, Royal actor5 tick9803, physical controller; no runtime writes.',
                 binary_sha256=sha(binary),runtime_manifest=manifest,policy_sha256=sha(policy),
                 environment={key:value for key,value in env.items() if key.startswith('WF_POLICY_')},
                 files={name:sha(FIXTURE/name) for name in ('arcade-ram.bin','state.json')})
    (FIXTURE/'manifest.json').write_text(json.dumps(fixture,indent=2)+'\n')
    print('CAPTURED',FIXTURE)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build/tag-policy-search/cpu-collision-native')
    parser.add_argument('--output',type=Path,default=ROOT/'build/royal-pin-release')
    parser.add_argument('--source',type=Path,default=SOURCE)
    parser.add_argument('--capture',action='store_true')
    args=parser.parse_args();build=args.build_dir.resolve();output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((build/'runtime-build.json').read_text());assert not manifest['sources_changed_during_build']
    objects=[]
    for name,digest in sorted(manifest['objects'].items()):
        assert sha(build/name)==digest,('stale object',name)
        if name!='wf_r17_actions_r11.o':objects.append(str(build/name))
    if args.capture:
        capture(build,output,manifest,objects)
        return
    fixture=json.loads((FIXTURE/'manifest.json').read_text())
    for name,digest in fixture['files'].items():assert sha(FIXTURE/name)==digest
    binary=output/'royal-pin-release';source_hash=sha(args.source)
    subprocess.run([os.environ.get('CC','cc'),*manifest['configuration']['flags'],'-I'+str(ROOT/'core/src'),
                    str(ROOT/'tests/test_royal_pin_release.c'),str(args.source.resolve()),
                    str(ROOT/'tests/native/assets_fixture.c'),*objects,'-o',str(binary)],cwd=ROOT,check=True)
    rom=(ROOT/'source_assets/ROM68K.BIN').read_bytes()
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    raw=(FIXTURE/'arcade-ram.bin').read_bytes();metadata=json.loads((FIXTURE/'state.json').read_text());regs=metadata['d']
    actor=0x5b0+5*0x10c;target=0x5b0+6*0x10c
    assert int.from_bytes(raw[actor+0x26:actor+0x2a],'big')==0x1c0000+target
    assert int.from_bytes(raw[target+0x26:target+0x2a],'big')==0
    cases=[('observed',raw)]
    # Tagged-null variants take the frame/contact release branch, since the
    # original early selector branch only releases on literal LONG zero.
    for link in (0x80000000,0xff000000,0x1c05b0,0x801c05b0):
        data=bytearray(raw);data[target+0x26:target+0x2a]=link.to_bytes(4,'big')
        data[actor+0x60]&=0x7f;data[actor+0x37]|=0x10
        data[actor+0x24:actor+0x26]=b'\x00\x01'
        data[target+0x9c:target+0xa0]=bytes(4)
        data[0x5b0+0x9c:0x5b0+0xa0]=b'\x12\x34\x56\x78'
        cases.append((f'link-{link:08x}',bytes(data)))
    machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
    reports=[];rom_writes=[]
    def write_rom(address,value):rom_writes.append((address,value))
    mem.set_special_range_write_funcs(0,num_pages=8,w8=write_rom,w16=write_rom,w32=write_rom)
    try:
        for name,data in cases:
            input_path=output/(name+'-input.bin');input_path.write_bytes(data)
            native_path=output/(name+'-native.bin')
            result=subprocess.run([str(binary),str(input_path),str(native_path),'5',*map(str,regs)],text=True,capture_output=True)
            (output/(name+'-native-result.json')).write_text(json.dumps(dict(returncode=result.returncode,stdout=result.stdout,stderr=result.stderr),indent=2)+'\n')
            mem.w_block(0,rom);mem.w_block(0x1c0000,data);rom_writes.clear();cpu.w_sr(0x2700)
            for i,r in enumerate(regs):cpu.w_reg(i,r)
            for i in range(8,15):cpu.w_reg(i,0)
            cpu.w_reg(8,0x1c0000+actor);cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(0x1c03e)
            trace=[]
            while cpu.r_pc()!=0x70000:
                trace.append(cpu.r_pc());assert len(trace)<10000,(name,'ROM runaway',hex(cpu.r_pc()))
                cpu.execute(1)
            expected=mem.r_block(0x1c0000,0x4000)+b''.join(cpu.r_reg(i).to_bytes(4,'big') for i in range(8))
            (output/(name+'-oracle.bin')).write_bytes(expected)
            (output/(name+'-trace.json')).write_text(json.dumps([hex(pc) for pc in trace])+'\n')
            assert result.returncode==0,(name,'native blocked',result.stdout,result.stderr)
            actual=native_path.read_bytes()
            assert actual==expected,(name,'RAM/register mismatch',[(hex(i+0x1c0000),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:16])
            assert mem.r_block(0,len(rom))==rom
            reports.append(dict(name=name,instructions=len(trace),rom_writes=list(rom_writes),output_sha256=hashlib.sha256(actual).hexdigest()))
            print('PASS',name,result.stdout.strip(),flush=True)
    finally:machine.cleanup()
    # An unsupported RAM address must still stop at the exact dependency;
    # the ROM-null exception must not turn every missing actor into success.
    invalid=bytearray(cases[-1][1]);invalid[target+0x26:target+0x2a]=(0x1c4000).to_bytes(4,'big')
    invalid_path=output/'invalid-ram-input.bin';invalid_path.write_bytes(invalid)
    invalid_result=subprocess.run([str(binary),str(invalid_path),str(output/'invalid-ram-native.bin'),'5',*map(str,regs)],text=True,capture_output=True)
    assert invalid_result.returncode==2 and 'dependency=135b0' in invalid_result.stdout,invalid_result
    assert sha(args.source)==source_hash,'source changed during run'
    (output/'summary.json').write_text(json.dumps(dict(status='PASS',scope=__doc__,fixture=fixture['files'],
            source_sha256=source_hash,binary_sha256=sha(binary),runtime_manifest_sha256=sha(build/'runtime-build.json'),cases=reports,
            test_sources={name:sha(ROOT/name) for name in ('tests/test_royal_pin_release.py','tests/test_royal_pin_release.c')},
            unsupported_ram_negative_control=invalid_result.stdout.strip()),indent=2)+'\n')


if __name__=='__main__':main()
