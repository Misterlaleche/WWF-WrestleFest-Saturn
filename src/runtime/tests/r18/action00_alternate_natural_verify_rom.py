#!/usr/bin/env python3
"""Continue a natural alternate-$00 HOST capture against the arcade ROM."""
import argparse
import csv
import ctypes
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
WORKSPACE = ROOT.parents[1]
HERE = Path(__file__).resolve().parent
HISTORIC = WORKSPACE/'validation/local-20261024/tag-winner-pressure-after1f'
CAPTURE = HISTORIC/'snapshot-01216.bin'
TRACE = HISTORIC/'trace.csv'
GRAPHICS = WORKSPACE/'validation/local-20261027/action-pose-metasprite-after-wordcache.json'
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
RAM, FG, SLOT = 0x1c0000, 0xc0000, 5
ACTOR = RAM+0x5b0+SLOT*0x10c
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
rom = (ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
capture = CAPTURE.read_bytes()
assert hashlib.sha1(rom).hexdigest() == ROM_SHA1
assert hashlib.sha256(capture).hexdigest() == '4c1c2720dafa048d968d312a00f3c2100ee87301dd6ed4fc15b60e2bdc317553'
assert len(capture) == 16472 and struct.unpack_from('=6I', capture) == (4002, SLOT, 1, 0x1c03e, 1216, 0)
source_character = int.from_bytes(capture[56+0x5b0+SLOT*0x10c+2:56+0x5b0+SLOT*0x10c+4], 'big')
assert source_character == 3
with TRACE.open(newline='') as stream:
    observed = [row for row in csv.DictReader(stream)
                if row['kind'] == 'dispatch' and int(row['slot']) == SLOT
                and 4002 <= int(row['frame']) <= 4034]
assert [int(row['frame']) for row in observed] == list(range(4002,4035))
assert all(int(row['state'],16)&255 == 5 and int(row['selector'],16)&255 == 0
           and int(row['flags33'],16)&0x20 and row['status'] == '0' for row in observed)
assert int(observed[-1]['after_state'],16)&255 == 0
observed_poses = [int(row['after_pose'],16) for row in observed]

core = re.search(r'^CORE=(.*)$', (ROOT/'Makefile').read_text(), re.M)[1].split()
bridge = 'tests/r18/action_reversal_family_oracle_bridge.c'
cases = []
with tempfile.TemporaryDirectory(prefix='wf-action00-natural-alt-', ignore_cleanup_errors=os.name == 'nt') as temp:
    frozen = Path(temp)
    for folder in ('src','include','generated'):
        shutil.copytree(ROOT/folder, frozen/folder)
    (frozen/'tests/r18').mkdir(parents=True)
    shutil.copy2(ROOT/bridge, frozen/bridge)
    sources = {str(path.relative_to(frozen)).replace('\\','/'):
               hashlib.sha256(path.read_bytes()).hexdigest()
               for path in sorted(frozen.rglob('*')) if path.is_file()}
    library = frozen/'action00.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                    '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',bridge,'-o',str(library)],
                   cwd=frozen,check=True)
    native = ctypes.CDLL(str(library))
    native.action_reversal_family_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32),
                                                     ctypes.c_uint, ctypes.POINTER(ctypes.c_uint32)]
    for name in ('action_reversal_resolver_oracle','action_reversal_physics_oracle'):
        getattr(native,name).argtypes = [ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    for character in range(12):
        initial=bytearray(0x200000)
        initial[:len(rom)]=rom
        initial[RAM:RAM+0x2000]=capture[56:8248]
        initial[FG:FG+0x2000]=capture[8248:16440]
        initial[ACTOR+2:ACTOR+4]=character.to_bytes(2,'big')
        assert initial[ACTOR+0x56]&0x80 and initial[ACTOR+0x33]&0x20
        machine=mus.Machine(mus.CPUType.M68000,2048)
        cpu,mem=machine.cpu,machine.mem
        mem.w_block(0,bytes(initial))
        buffer=ctypes.create_string_buffer(bytes(initial))
        d=(ctypes.c_uint32*8)(*struct.unpack_from('=8I',capture,24))
        poses=[]
        for tick in range(33):
            def execute(pc):
                cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
                for i in range(15):cpu.w_reg(i,d[i] if i<8 else 0)
                cpu.w_reg(8,ACTOR)
                descriptor=None
                for _ in range(20000):
                    at=cpu.r_pc()
                    if at==0x1c0c8:descriptor=cpu.r_reg(9)
                    if at==0x70000:break
                    cpu.execute(1)
                assert cpu.r_pc()==0x70000,(character,tick,hex(pc),'ROM runaway')
                return descriptor
            def equal(stage):
                for at,n in ((RAM,0x4000),(FG,0x4000)):
                    expected=bytes(mem.r_block(at,n));got=buffer.raw[at:at+n]
                    assert got==expected,(character,tick,stage,hex(at),
                        [(hex(at+i),x,y) for i,(x,y) in enumerate(zip(got,expected)) if x!=y][:12])
                assert list(d)==[cpu.r_reg(i) for i in range(8)],(character,tick,stage,'D0-D7')
            # The captured first call is already positioned immediately before
            # dispatch. From the next tick, run the state and arena handoff.
            if tick:
                state=mem.r16(ACTOR+0x20)
                if not state&0x8000:
                    mem.w16(ACTOR+0x1e,mem.r16(ACTOR+0x1c))
                    mem.w16(ACTOR+0x1c,state);mem.w16(ACTOR+0x20,state|0x8000)
                mem.w16(ACTOR+0x36,0)
                if buffer.raw[ACTOR+1]:
                    execute(0x280dc)
                    if mem.r8(ACTOR+0x37):
                        for offset,correction in ((6,0x38),(10,0x3a),(14,0x3c)):
                            mem.w16(ACTOR+offset,(mem.r16(ACTOR+offset)+mem.r16(ACTOR+correction))&0xffff)
                assert native.action_reversal_resolver_oracle(buffer,d,SLOT)==0
                equal('resolver')
            descriptor=execute(0x1c03e);actual=ctypes.c_uint32()
            assert native.action_reversal_family_oracle(buffer,d,SLOT,ctypes.byref(actual))==0
            assert descriptor==actual.value==0x1286c,(character,tick,'descriptor',descriptor,actual.value)
            equal('action')
            pose=mem.r16(ACTOR+4);poses.append(pose)
            assert pose==observed_poses[tick],(character,tick,'natural pose timing',hex(pose),hex(observed_poses[tick]))
            execute(0x2208)
            assert native.action_reversal_physics_oracle(buffer,d,SLOT)==0
            equal('physics')
            state=mem.r16(ACTOR+0x20)&255
            assert state==(0 if tick==32 else 5),(character,tick,'unexpected state',state)
        assert bytes(mem.r_block(0,len(rom)))==rom
        cases.append({'character':character,'steps':len(poses),'terminal_state':0,
                      'poses':[f'{pose:04X}' for pose in sorted({x&0x7fff for x in poses})]})
        machine.cleanup()
    if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))

graphics=json.loads(GRAPHICS.read_text())
assert graphics['status']=='PASS' and graphics['rom_sha1']==ROM_SHA1 and graphics['blocked_cases']==[]
assert graphics['mode']=='action-pose-links' and graphics['direct_rom_cases']==2356
assert graphics['action_character_pose_pairs']==1178
assert graphics['pose_link_audit_sha256']==hashlib.sha256(
    (WORKSPACE/'validation/local-20261026/action-pose-link-audit.json').read_bytes()).hexdigest()
assert graphics['metasprite_source_sha256']==hashlib.sha256((ROOT/'src/wf_r18_metasprite.c').read_bytes()).hexdigest()
assert graphics['verifier_sha256']==hashlib.sha256((ROOT/'tools/verify_r18_metasprite_rom.py').read_bytes()).hexdigest()
inventory=json.loads((WORKSPACE/'validation/local-20261026/action-selector-inventory.json').read_text())
assert inventory['selectors'][0]['characters_in_generated_choice_matrix']==list(range(12))
tables=json.loads((WORKSPACE/'validation/local-20261026/action-animation-table-audit.json').read_text())
assert tables['status']=='PASS' and tables['rom_sha1']==ROM_SHA1
assert {int(value,16)&0x7fff for value in tables['action_selectors'][0]['poses_raw']}=={0,0x92,0x93,0x94}
assert all(case['poses']==['0000','0092','0093','0094'] for case in cases)
report={'status':'PASS','rom_sha1':ROM_SHA1,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),
        'historical_trace_sha256':hashlib.sha256(TRACE.read_bytes()).hexdigest(),
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'graphics_report_sha256':hashlib.sha256(GRAPHICS.read_bytes()).hexdigest(),
        'sources_sha256':sources,'source_character':source_character,'cases':cases,
        'total_steps':sum(case['steps'] for case in cases),'graphic_character_pose_pairs':48,
        'limits':['The natural captured character is 3; eleven other characters are controlled variants of this snapshot.',
                  'The HOST snapshot projects 8 KiB of game RAM and 8 KiB of foreground; the remaining compared bytes start at zero.',
                  'The first call begins immediately before dispatch; later calls interleave the actor resolver and physics.',
                  'Other actors, input, referee and complete frame scheduling do not run between calls.',
                  'The historical HOST match blocked later at frame 4189; this report covers frames 4002 through 4034 only.',
                  'No Saturn hardware timing or final VDP1 pixel equivalence is claimed.']}
args.report.parent.mkdir(parents=True,exist_ok=True)
args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS natural alternate $00 and 12 character continuations:',report['total_steps'],'action/physics steps')
