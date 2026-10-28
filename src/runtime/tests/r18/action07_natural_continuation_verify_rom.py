#!/usr/bin/env python3
"""Continue a natural selector-$07 capture and audit its character graphics."""
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
import sys
import tempfile
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
WORKSPACE = ROOT.parents[1]
HISTORIC = WORKSPACE/'validation/local-20261024/tag-current-basic-afterraw79'
CAPTURE = HISTORIC/'snapshot-00408.bin'
TRACE = HISTORIC/'trace.csv'
GRAPHICS = WORKSPACE/'validation/local-20261027/action-pose-metasprite-after-wordcache.json'
ROM_ARCHIVE = WORKSPACE.parent/'local-rom/wwfwfest.zip'
ROM_SHA1 = 'a726c46f350723ea73158bc78055d17d418b14e2'
RAM, FG, SLOT = 0x1c0000, 0xc0000, 1
ACTOR = RAM+0x5b0+SLOT*0x10c
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
capture=CAPTURE.read_bytes()
assert hashlib.sha1(rom).hexdigest()==ROM_SHA1
assert hashlib.sha256(capture).hexdigest()=='9ceb6a04c00dd5c1a74d9270edebd49f2b5d29c71bb5bee22c5fd7a8ff1e633f'
assert len(capture)==16472 and struct.unpack_from('=6I',capture)==(1200,SLOT,1,0x1c03e,408,0)
source_character=int.from_bytes(capture[56+0x5b0+SLOT*0x10c+2:56+0x5b0+SLOT*0x10c+4],'big')
assert source_character==7
with TRACE.open(newline='') as stream:
    observed=[row for row in csv.DictReader(stream) if row['kind']=='dispatch'
              and int(row['slot'])==SLOT and 1200<=int(row['frame'])<=1241]
assert [int(row['frame']) for row in observed]==list(range(1200,1242))
assert all(int(row['state'],16)&255==5 and int(row['selector'],16)&255==7
           and row['status']=='0' for row in observed)
assert int(observed[-1]['after_state'],16)&255==0
observed_poses=[int(row['after_pose'],16) for row in observed]

core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
bridge='tests/r18/action_reversal_family_oracle_bridge.c'
cases=[]
with tempfile.TemporaryDirectory(prefix='wf-action07-natural-',ignore_cleanup_errors=os.name=='nt') as temp:
    frozen=Path(temp)
    for folder in ('src','include','generated'):shutil.copytree(ROOT/folder,frozen/folder)
    (frozen/'tests/r18').mkdir(parents=True)
    shutil.copy2(ROOT/bridge,frozen/bridge)
    sources={str(path.relative_to(frozen)).replace('\\','/'):hashlib.sha256(path.read_bytes()).hexdigest()
             for path in sorted(frozen.rglob('*')) if path.is_file()}
    library=frozen/'action07.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                    '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',bridge,'-o',str(library)],cwd=frozen,check=True)
    native=ctypes.CDLL(str(library))
    native.action_reversal_family_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),
                                                    ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    for name in ('action_reversal_resolver_oracle','action_reversal_physics_oracle'):
        getattr(native,name).argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    for character in range(12):
        initial=bytearray(0x200000);initial[:len(rom)]=rom
        initial[RAM:RAM+0x2000]=capture[56:8248];initial[FG:FG+0x2000]=capture[8248:16440]
        initial[ACTOR+2:ACTOR+4]=character.to_bytes(2,'big')
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
        mem.w_block(0,bytes(initial));buffer=ctypes.create_string_buffer(bytes(initial))
        d=(ctypes.c_uint32*8)(*struct.unpack_from('=8I',capture,24))
        poses=[];duration=42 if character==7 else 38
        for tick in range(duration):
            def execute(pc):
                cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
                for i in range(15):cpu.w_reg(i,d[i] if i<8 else 0)
                cpu.w_reg(8,ACTOR);descriptor=None
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
            if tick:
                state=mem.r16(ACTOR+0x20)
                if not state&0x8000:
                    mem.w16(ACTOR+0x1e,mem.r16(ACTOR+0x1c))
                    mem.w16(ACTOR+0x1c,state);mem.w16(ACTOR+0x20,state|0x8000)
                mem.w16(ACTOR+0x36,0)
                if buffer.raw[ACTOR+1]:
                    execute(0x280dc)
                    if mem.r8(ACTOR+0x37):
                        for off,corr in ((6,0x38),(10,0x3a),(14,0x3c)):
                            mem.w16(ACTOR+off,(mem.r16(ACTOR+off)+mem.r16(ACTOR+corr))&0xffff)
                assert native.action_reversal_resolver_oracle(buffer,d,SLOT)==0
                equal('resolver')
            descriptor=execute(0x1c03e);actual=ctypes.c_uint32()
            assert native.action_reversal_family_oracle(buffer,d,SLOT,ctypes.byref(actual))==0
            assert descriptor==actual.value==(0x130e6 if character==7 else 0x130fe),(
                character,tick,'descriptor',descriptor,actual.value)
            equal('action');pose=mem.r16(ACTOR+4);poses.append(pose)
            if character==source_character:
                assert pose==observed_poses[tick],(character,tick,'historical pose',hex(pose),hex(observed_poses[tick]))
            execute(0x2208);assert native.action_reversal_physics_oracle(buffer,d,SLOT)==0
            equal('physics')
            state=mem.r16(ACTOR+0x20)&255
            assert state==(0 if tick==duration-1 else 5),(character,tick,'terminal',state)
        assert bytes(mem.r_block(0,len(rom)))==rom
        changes=[]
        for tick,pose in enumerate(poses,1):
            if not changes or changes[-1][1]!=pose:changes.append([tick,pose])
        cases.append({'character':character,'steps':duration,'terminal_state':0,
                      'pose_changes':changes,'poses':[f'{x:04X}' for x in sorted({p&0x7fff for p in poses})]})
        machine.cleanup()
    if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))

# The normal human/CPU choice tables and actual graphics availability are
# independent evidence for which character variants are meaningful.
matrix=(ROOT/'generated/wf_action_matrix.c').read_text()
rows=[tuple(int(value,16) for value in triple) for triple in
      re.findall(r'\{(0x[0-9A-F]+),(0x[0-9A-F]+),(0x[0-9A-F]+)\}',matrix)]
assert len(rows)==12*21
human=[]
for character in range(12):
    start=0xe52e+64*character
    generated=bytes(value for row in rows[character*21:(character+1)*21] for value in row)
    assert rom[start:start+63]==generated and rom[start+63]==0xff
    human.extend([{'character':character,'category':i//3,'button':i%3+1}
                  for i,value in enumerate(generated) if value==7])
def pointer(at):
    value=int.from_bytes(rom[at:at+4],'big');assert 0<=value<len(rom);return value
cpu_hits=[];cpu_tables=0
for base,lanes in ((0x219b4,9),(0x2307c,3)):
    for character in range(12):
        directory=pointer(base+4*character)
        for lane in range(lanes):
            at=pointer(directory+4*lane);count=rom[at]
            assert count<=6 and at+1+count<=len(rom)
            if 7 in rom[at+1:at+1+count]:cpu_hits.append({'table':f'{base:06X}','character':character,'lane':lane})
            cpu_tables+=1
assert len(human)==4 and {hit['character'] for hit in human}=={7,8}
assert cpu_tables==144 and len(cpu_hits)==6 and {hit['character'] for hit in cpu_hits}=={7,8}
sys.path.insert(0,str(ROOT/'tools'))
from wf_r18_rom_graphics import Rom
graphics_rom=Rom(ROM_ARCHIVE)
available={character:[graphics_rom.pose_entry(character,pose) not in (None,65534,65535)
                      for pose in (0xbf,0xc0,0xc1)] for character in range(12)}
assert [character for character,valid in available.items() if all(valid)]==[7,8]
assert all(not any(valid) for character,valid in available.items() if character not in (7,8))
graphics=json.loads(GRAPHICS.read_text())
assert graphics['status']=='PASS' and graphics['mode']=='action-pose-links'
assert graphics['rom_sha1']==ROM_SHA1 and graphics['direct_rom_cases']==2356 and not graphics['blocked_cases']
assert graphics['metasprite_source_sha256']==hashlib.sha256((ROOT/'src/wf_r18_metasprite.c').read_bytes()).hexdigest()
assert graphics['verifier_sha256']==hashlib.sha256((ROOT/'tools/verify_r18_metasprite_rom.py').read_bytes()).hexdigest()
assert graphics['pose_link_audit_sha256']==hashlib.sha256(
    (WORKSPACE/'validation/local-20261026/action-pose-link-audit.json').read_bytes()).hexdigest()
inventory=json.loads((WORKSPACE/'validation/local-20261026/action-selector-inventory.json').read_text())
assert inventory['selectors'][7]['characters_in_generated_choice_matrix']==[7,8]
tables=json.loads((WORKSPACE/'validation/local-20261026/action-animation-table-audit.json').read_text())
assert tables['status']=='PASS' and tables['rom_sha1']==ROM_SHA1
assert {int(value,16)&0x7fff for value in tables['action_selectors'][7]['poses_raw']}=={0xbf,0xc0,0xc1}
assert all(case['poses']==['00BF','00C0','00C1'] for case in cases)
report={'status':'PASS','rom_sha1':ROM_SHA1,'snapshot_sha256':hashlib.sha256(capture).hexdigest(),
        'historical_trace_sha256':hashlib.sha256(TRACE.read_bytes()).hexdigest(),
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'graphics_report_sha256':hashlib.sha256(GRAPHICS.read_bytes()).hexdigest(),
        'source_character':source_character,'sources_sha256':sources,'cases':cases,
        'total_steps':sum(case['steps'] for case in cases),'human_choice_hits':human,
        'cpu_choice_tables_checked':cpu_tables,'cpu_choice_hits':cpu_hits,
        'characters_with_rom_poses':[7,8],'empty_pose_entries_for_controlled_ids':[0,1,2,3,4,5,6,9,10,11],
        'limits':['The historical natural capture is character 7; other IDs are controlled substitutions.',
                  'Ten substitutions lack these three sprite poses in the original ROM; their action tests do not establish natural reachability.',
                  'Only normal human and two CPU choice families were audited; indirect assignments may exist.',
                  'The historical HOST match blocked later at frame 5026; no complete current match is claimed.',
                  'The snapshot projects 8 KiB RAM and foreground, with remaining compared bytes initialized to zero.',
                  'Other actors and complete frame scheduling do not run between calls; no VDP1 or Saturn timing claim.']}
args.report.parent.mkdir(parents=True,exist_ok=True)
args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS $07:',len(cases),'character continuations,',report['total_steps'],'steps; graphics/normal choices:',[7,8])
