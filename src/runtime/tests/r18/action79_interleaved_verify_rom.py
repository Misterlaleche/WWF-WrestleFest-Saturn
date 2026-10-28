"""Natural $79/$1E/$1F/$2D/$2E/$2F continuations with resolver/action/physics interleaved."""
import argparse
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
HERE = Path(__file__).resolve().parent
RAM = 0x1c0000
FG = 0xc0000
parser = argparse.ArgumentParser()
parser.add_argument('--route', choices=('79','1e','1f','2d','2e','2f'), default='79')
parser.add_argument('--character', type=int, choices=range(12))
parser.add_argument('--report', type=Path)
parser.add_argument('--skip-graphics', action='store_true',
                    help='Record ROM action/physics pose discovery before direct graphics verification')
args = parser.parse_args()
rom = (ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
if args.route=='79':
    manifest=json.loads((HERE/'action79_manifest.json').read_bytes())
    capture_name='action79_snapshot_afterflag34.bin'
    snapshot_sha256=manifest['sha256']
    header=(5677,1,1,0x1c03e,1529,1)
    slot=1
    action_bridge='action79_oracle_bridge.c'
    action_symbol='wf_action79_oracle'
elif args.route in ('1e','1f'):
    manifest=json.loads((HERE/'action1f_manifest.json').read_bytes())
    capture_name=('action1f_snapshot_tag_pressure_3747.bin' if args.route=='1f'
                  else 'action1f_snapshot_tag_pressure_4189_raw1e.bin')
    snapshot_sha256=(manifest['snapshot_sha256'] if args.route=='1f'
                     else manifest['alias1e']['snapshot_sha256'])
    header=((3747,5,1,0x1c03e,1114,1) if args.route=='1f'
            else (4189,1,1,0x1c03e,1268,1))
    slot=header[1]
    action_bridge='action1f_oracle_bridge.c'
    action_symbol='action1f_oracle'
elif args.route=='2f':
    manifest=json.loads((HERE/'action2f_manifest.json').read_bytes())
    capture_name='action2f_snapshot_f88acad.bin'
    snapshot_sha256=manifest['sha256']
    header=(3016,1,1,0x1c03e,952,1)
    slot=1
    action_bridge='action2f_oracle_bridge.c'
    action_symbol='wf_action2f_oracle_with_descriptor'
elif args.route=='2e':
    manifest=json.loads((HERE/'action2e_manifest.json').read_bytes())
    capture_name='action2e_snapshot_main.bin'
    snapshot_sha256=manifest['sha256']
    header=tuple(manifest['header'])
    slot=1
    action_bridge='action2e_oracle_bridge.c'
    action_symbol='wf_action2e_oracle_with_descriptor'
else:
    manifest=json.loads((HERE/'action2d_manifest.json').read_bytes())
    capture_name='action2d_snapshot_full_flow_3614.bin'
    snapshot_sha256=manifest['sha256']
    header=tuple(manifest['header'])
    slot=4
    action_bridge='action2d_oracle_bridge.c'
    action_symbol='action2d_oracle'
capture=(HERE/capture_name).read_bytes()
assert hashlib.sha1(rom).hexdigest()==manifest['rom_sha1']
assert hashlib.sha256(capture).hexdigest()==snapshot_sha256
assert struct.unpack_from('=6I',capture)==header
actor = RAM+0x5b0+slot*0x10c
source_character = int.from_bytes(capture[56+0x5b0+slot*0x10c+2:56+0x5b0+slot*0x10c+4], 'big')
core = re.search(r'^CORE=(.*)$', (ROOT/'Makefile').read_text(), re.M).group(1).split()
cases = []
with tempfile.TemporaryDirectory(prefix=f'wf-action{args.route}-interleaved-', ignore_cleanup_errors=os.name=='nt') as temp:
    frozen = Path(temp)
    for folder in ('src','include','generated'):
        shutil.copytree(ROOT/folder, frozen/folder)
    (frozen/'tests/r18').mkdir(parents=True)
    bridges = (action_bridge,'action_reversal_family_oracle_bridge.c')
    for bridge in bridges:
        shutil.copy2(HERE/bridge, frozen/'tests/r18'/bridge)
    lib = frozen/'action.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',
                    '-Iinclude',*core,'src/wf_r18_runtime_handoff.c',
                    *(f'tests/r18/{name}' for name in bridges),'-o',str(lib)],
                   cwd=frozen, check=True)
    native = ctypes.CDLL(str(lib))
    action_oracle=getattr(native,action_symbol)
    action_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
    for name in ('action_reversal_resolver_oracle','action_reversal_physics_oracle'):
        getattr(native,name).argtypes = [ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
    for character in ([args.character] if args.character is not None else range(12)):
        initial = bytearray(0x200000)
        initial[:len(rom)] = rom
        initial[RAM:RAM+0x2000] = capture[56:8248]
        initial[FG:FG+0x2000] = capture[8248:16440]
        initial[actor+2:actor+4] = character.to_bytes(2,'big')
        regs = list(struct.unpack_from('=8I',capture,24))
        machine = mus.Machine(mus.CPUType.M68000,2048)
        cpu,mem = machine.cpu,machine.mem
        mem.w_block(0,bytes(initial))
        buffer = ctypes.create_string_buffer(bytes(initial))
        d = (ctypes.c_uint32*8)(*regs)
        for i,value in enumerate(regs):cpu.w_reg(i,value)
        phase_history = []
        selector_history = []
        visual_poses = {int.from_bytes(initial[actor+4:actor+6],'big')&0x7fff}

        def run_rom(pc, limit=20000):
            cpu.w_sp(0x1d0000)
            mem.w32(0x1d0000,0x70000)
            cpu.w_pc(pc)
            for i in range(15):cpu.w_reg(i,d[i] if i<8 else 0)
            cpu.w_reg(8,actor)
            descriptor = 0
            for _ in range(limit):
                at = cpu.r_pc()
                if at==0x1c0c8:descriptor=cpu.r_reg(9)
                if at==0x70000:break
                if args.route in ('79','2e','2f') and at==0x2052:
                    cpu.w_pc(mem.r32(cpu.r_sp()))
                    cpu.w_sp(cpu.r_sp()+4)
                    continue
                cpu.execute(1)
            assert cpu.r_pc()==0x70000,(character,pc,'ROM runaway',hex(cpu.r_pc()))
            return descriptor

        def equal(stage,tick,descriptor=None,native_descriptor=None):
            for at,n in ((RAM,0x4000),(FG,0x4000)):
                expected=bytes(mem.r_block(at,n))
                got=buffer.raw[at:at+n]
                assert got==expected,(character,tick,stage,'RAM/FG',
                                      [(hex(at+i),x,y) for i,(x,y) in enumerate(zip(got,expected)) if x!=y][:12])
            assert list(d)==[cpu.r_reg(i) for i in range(8)],(character,tick,stage,'D0-D7')
            if descriptor is not None:
                assert descriptor==native_descriptor,(character,tick,stage,'descriptor',hex(descriptor),hex(native_descriptor))

        for tick in range(256):
            state=mem.r16(actor+0x20)
            if not state&0x8000:
                mem.w16(actor+0x1e,mem.r16(actor+0x1c))
                mem.w16(actor+0x1c,state)
                mem.w16(actor+0x20,state|0x8000)
            mem.w16(actor+0x36,0)
            if buffer.raw[actor+1]:
                run_rom(0x280dc,5000)
                if mem.r8(actor+0x37):
                    for off,corr in ((6,0x38),(10,0x3a),(14,0x3c)):
                        mem.w16(actor+off,(mem.r16(actor+off)+mem.r16(actor+corr))&0xffff)
            status=native.action_reversal_resolver_oracle(buffer,d,slot)
            assert status==0,(character,tick,'resolver',status)
            equal('resolver',tick)

            descriptor=run_rom(0x1c03e)
            nd=ctypes.c_uint32()
            status=action_oracle(buffer,d,slot,ctypes.byref(nd))
            assert status==0,(character,tick,'action',status)
            equal('action',tick,descriptor,nd.value)

            run_rom(0x2208,5000)
            status=native.action_reversal_physics_oracle(buffer,d,slot)
            assert status==0,(character,tick,'physics',status)
            equal('physics',tick)
            visual_poses.add(mem.r16(actor+4)&0x7fff)
            phase=mem.r16(actor+0x44)
            if not phase_history or phase_history[-1][1]!=phase:phase_history.append((tick,phase))
            selector=mem.r16(actor+0x60)&0xff
            if not selector_history or selector_history[-1][1]!=selector:selector_history.append((tick,selector))
            if not (mem.r16(actor+0x20)&0xff):break
            if args.route=='2f' and selector not in (0x2f,0x81):break
            if args.route=='2e' and selector!=0x2e:break
            if args.route=='2d' and selector!=0x2d:break
        else:raise AssertionError((character,'no release after 256 ticks'))
        if args.route not in ('2d','2e','2f'):
            assert (mem.r16(actor+0x60)&0xff)==int(args.route,16),(character,'unexpected selector')
        assert bytes(mem.r_block(0,len(rom)))==rom
        cases.append({'character':character,'steps':tick+1,'phase_history':phase_history,
                      'selector_history':selector_history,
                      'visual_poses':[f'{pose:04X}' for pose in sorted(visual_poses)],
                      'terminal_state':mem.r16(actor+0x20)&0xff})
        machine.cleanup()
    if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))

validation = ROOT.parent.parent/'validation/local-20261026'
actual={(case['character'],int(pose,16)) for case in cases for pose in case['visual_poses']}
graphic_covered=None
graphic_action_sha=None
graphic_royal_sha=None
if actual and not args.skip_graphics:
    tables=json.loads((validation/'action-animation-table-audit.json').read_bytes())
    inventory=json.loads((validation/'action-selector-inventory.json').read_bytes())
    pose_audit=json.loads((validation/'action-pose-link-audit.json').read_bytes())
    graphics=json.loads((validation/'action-pose-metasprite-verify.json').read_bytes())
    royal_graphics=json.loads((validation/'royal-exit-poses-direct-rom.json').read_bytes())
    assert all(item['status']=='PASS' and item['rom_sha1']==manifest['rom_sha1']
               for item in (pose_audit,graphics,royal_graphics))
    assert graphics['pose_link_audit_sha256']==hashlib.sha256((validation/'action-pose-link-audit.json').read_bytes()).hexdigest()
    assert graphics['verifier_sha256']==royal_graphics['verifier_sha256']==hashlib.sha256((ROOT/'tools/verify_r18_metasprite_rom.py').read_bytes()).hexdigest()
    assert graphics['metasprite_source_sha256']==royal_graphics['metasprite_source_sha256']==hashlib.sha256((ROOT/'src/wf_r18_metasprite.c').read_bytes()).hexdigest()
    linked={(character,int(pose,16)&0x7fff)
            for selector,item in enumerate(tables['action_selectors'])
            for character in inventory['selectors'][selector]['characters_in_generated_choice_matrix']
            for pose in item['poses_raw']
            if (character,int(pose,16)&0x7fff)!=(5,0x20f)}
    for category in ('additional_pose_pairs','action79_new_pose_pairs',
                     'action1e_1f_new_pose_pairs','action1e_1f_capture_new_pose_pairs',
                     'action2f_capture_new_pose_pairs','action2d_capture_new_pose_pairs'):
        linked.update((item['character'],int(item['pose'],16)) for item in pose_audit[category])
    linked.update((character,int(pose,16)) for character in range(12)
                  for pose in royal_graphics['royal_exit_poses'])
    graphic_targets=actual
    if args.route=='2f':
        matrix_characters=set(inventory['selectors'][0x2f]['characters_in_generated_choice_matrix'])
        assert matrix_characters=={1,5}
        graphic_targets={(character,pose) for character,pose in actual if character in matrix_characters}
        assert len(graphic_targets)==14
        assert pose_audit['action2f_controlled_nonmatrix_missing_pose_pairs']==26
    if args.route=='2e':
        matrix_characters=set(inventory['selectors'][0x2e]['characters_in_generated_choice_matrix'])
        assert matrix_characters=={1,5,9}
        graphic_targets={(character,pose) for character,pose in actual if character in matrix_characters}
        assert len(graphic_targets)==6
        assert pose_audit['action2e_controlled_nonmatrix_missing_pose_pairs']==9
    if args.route=='2d':
        matrix_characters=set(inventory['selectors'][0x2d]['characters_in_generated_choice_matrix'])
        assert matrix_characters=={0,5,6,10,11}
        graphic_targets={(character,pose) for character,pose in actual if character in matrix_characters}
        assert len(graphic_targets)==10
        assert pose_audit['action2d_controlled_nonmatrix_missing_pose_pairs']==7
    assert graphic_targets<=linked,('unverified visual poses',sorted(graphic_targets-linked))
    graphic_covered=len(graphic_targets&linked)
    graphic_action_sha=hashlib.sha256((validation/'action-pose-metasprite-verify.json').read_bytes()).hexdigest()
    graphic_royal_sha=hashlib.sha256((validation/'royal-exit-poses-direct-rom.json').read_bytes()).hexdigest()

report={'status':'PASS','route':args.route,'rom_sha1':manifest['rom_sha1'],'snapshot_sha256':snapshot_sha256,
        'source_character':source_character,'cases':cases,'total_steps':sum(case['steps'] for case in cases),
        'visual_character_pose_pairs':len(actual),'graphic_covered_visual_pairs':graphic_covered,
        'graphic_target_visual_pairs':(len(graphic_targets) if not args.skip_graphics else None),
        'visual_character_pose_pair_list':[{'character':character,'pose':f'{pose:04X}'}
                                           for character,pose in sorted(actual)],
        'graphic_action_report_sha256':graphic_action_sha,
        'graphic_royal_report_sha256':graphic_royal_sha,
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'action_source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),
        'resolver_source_sha256':hashlib.sha256((ROOT/'src/wf_r17_resolver_280dc.c').read_bytes()).hexdigest(),
        'bridges_sha256':{name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in bridges},
        'compared':['RAM16KB','FG16KB','D0-D7','action descriptor after each action tick'],
        'limits':['Character IDs other than the captured ID are controlled variants of one natural HOST snapshot.',
                  'Other actors and the full frame scheduler do not run.',
                  ('Only $2052 audio output is intercepted in the ROM.' if args.route in ('79','2e','2f')
                   else 'The ROM routine executes without intercepts.'),
                  'No Saturn timing or audible sound claim.']}
if args.skip_graphics:
    report['limits'].append('Visual pose pairs are discovered from the ROM continuation, not graphics-verified in this run.')
if args.route=='2f':
    report['limits'].append('Only characters 1 and 5 appear for $2F in the generated choice matrix; 26 pose pairs of artificial non-matrix variants are absent in the ROM. Dynamic reachability remains to be checked.')
if args.route=='2e':
    report['limits'].append('Only characters 1, 5 and 9 appear for $2E in the generated choice matrix; nine pose pairs of artificial non-matrix variants are absent in the ROM. Dynamic reachability remains to be checked.')
if args.route=='2d':
    report['limits'].append('Only characters 0, 5, 6, 10 and 11 appear for $2D in the generated choice matrix; seven pose pairs of artificial non-matrix variants are absent in the ROM. Dynamic reachability remains to be checked.')
if args.report:
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',len(cases),'character continuations;',report['total_steps'],'resolver/action/physics ticks')
