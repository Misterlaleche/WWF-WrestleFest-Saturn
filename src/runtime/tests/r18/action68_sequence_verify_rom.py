"""Isolated natural ring action, interleaving resolver/action/physics."""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,re,shutil,struct,subprocess,tempfile
import machine68k as mus

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
RAM=0x1c0000
parser=argparse.ArgumentParser();parser.add_argument('--route',choices=('68','6a','6b','7a'),default='68')
parser.add_argument('--start-frame',type=int)
parser.add_argument('--start-slot',type=int,default=4)
parser.add_argument('--character',type=int,choices=range(12))
parser.add_argument('--mode',choices=('tag','royal'),default='tag')
parser.add_argument('--rescue',action='store_true')
parser.add_argument('--through-rescue',action='store_true')
parser.add_argument('--scene-handoff',action='store_true')
parser.add_argument('--through-69',action='store_true')
parser.add_argument('--palette-allocation',action='store_true')
parser.add_argument('--emit-terminal-state',action='store_true')
args=parser.parse_args()
if args.start_frame is None:args.start_frame={'68':960,'6a':803,'6b':1343,'7a':2276}[args.route]
assert not (args.route=='6b' and args.through_69)
assert args.mode=='tag' or (args.route=='68' and args.start_frame==803 and not args.through_69)
assert not args.rescue or (args.route=='68' and args.mode=='tag' and args.start_frame==803 and not args.through_69)
assert not args.through_rescue or args.rescue
assert not args.scene_handoff or args.through_rescue
assert not args.palette_allocation or args.route=='7a'
assert not args.emit_terminal_state or (args.route=='68' and args.rescue and
                                        not args.through_rescue and args.character is not None)
assert args.route!='7a' or (args.start_frame==2276 and
                            not args.through_69 and not args.rescue and not args.through_rescue)
assert args.route!='6a' or (args.start_frame==803 and not args.through_69 and
                            not args.rescue and not args.through_rescue)
rom=(ROOT/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
if args.route=='6a':
 transition_report_path=ROOT.parent.parent/'validation/local-20261026/ringside-transition-current-rom.json'
 transition_report=json.loads(transition_report_path.read_bytes())
 meta={'rom_sha1':transition_report['rom_sha1']}
 item={'sha256':transition_report['postscene_state_sha256']}
 capture_name='action68-rescue-postscene-character02.state'
 capture=(ROOT.parent.parent/'validation/local-20261026'/capture_name).read_bytes()
 assert transition_report['passed']==transition_report['count']==30
 transition_case=next(case for case in transition_report['results'] if case['label']=='derived-action68-rescue-to-real-scene')
 assert not transition_case['native_status'] and not transition_case['memory_differences'] and not transition_case['register_differences']
 assert all(not part['count'] for part in transition_case['broad_diff'].values())
 assert capture[:4]==b'R68S' and len(capture)==4+32+0x4000*2
else:
 manifest_name='action7a_sequence_manifest.json' if args.route=='7a' else f'action{args.route}_manifest.json'
 meta=json.loads((HERE/manifest_name).read_bytes())
 item=next(item for item in meta['captures'] if item['frame']==args.start_frame
           and (args.route!='6b' or item['slot']==args.start_slot))
 capture_name=(f'action68_snapshot_noinput_{args.start_frame}.bin' if args.route=='68'
               else (f'action6b_snapshot_noinput_{args.start_frame}-slot{args.start_slot}.bin'
                     if args.route=='6b' else 'action7a_snapshot_full_flow_2276.bin'))
 capture=(HERE/capture_name).read_bytes()
assert hashlib.sha1(rom).hexdigest()==meta['rom_sha1']
assert hashlib.sha256(capture).hexdigest()==item['sha256']
if args.route=='7a':assert list(struct.unpack_from('=6I',capture))==item['header']
outside_capture_sha256=None
if args.scene_handoff:
 outside_meta=json.loads((HERE/'action6b_manifest.json').read_bytes())
 outside_item=next(item for item in outside_meta['captures'] if item['frame']==1343 and item['slot']==1)
 outside_capture=(HERE/'action6b_snapshot_noinput_1343-slot1.bin').read_bytes()
 assert hashlib.sha256(outside_capture).hexdigest()==outside_item['sha256']
 outside_capture_sha256=outside_item['sha256']
 assert capture[56+0x7e:56+0x80]==b'\x00\x00' and capture[56+0x161]==0
 assert outside_capture[56+0x7e:56+0x80]==b'\x00\x02' and outside_capture[56+0x161]==2
slot=4 if args.route in ('68','6a') else (args.start_slot if args.route=='6b' else 0)
actor=RAM+0x5b0+slot*0x10c
initial=bytearray(0x200000);initial[:len(rom)]=rom
if args.route=='6a':
 initial[RAM:RAM+0x4000]=capture[36:36+0x4000]
 initial[0xc0000:0xc4000]=capture[36+0x4000:]
else:
 initial[RAM:RAM+0x2000]=capture[56:8248]
 initial[0xc0000:0xc2000]=capture[8248:16440]
if args.mode=='royal':initial[RAM+0x161]|=1
if args.rescue:
 initial[actor+0x33]|=1
 initial[actor+0x37]|=0x10
source_character=int.from_bytes(initial[actor+2:actor+4],'big')
if args.character is not None:initial[actor+2:actor+4]=args.character.to_bytes(2,'big')
if args.palette_allocation:
 palette_character=args.character if args.character is not None else source_character
 initial[RAM+0x1610+palette_character]=0x83
 initial[RAM+0x1603]=0xff
regs=list(struct.unpack_from('=8I',capture,4 if args.route=='6a' else 24))
core=re.search(r'^CORE=(.*)$',(ROOT/'Makefile').read_text(),re.M)[1].split()
events=[];steps=0;phase_history=[];selector_history=[];scene_handoff_at=None
visual_poses={int.from_bytes(initial[actor+4:actor+6],'big')&0x7fff}
terminal_state_sha256=None;terminal_state_name=None

with tempfile.TemporaryDirectory(prefix='wf-ring-sequence-',ignore_cleanup_errors=os.name=='nt') as temp:
 frozen=Path(temp)
 for part in ('src','include','generated'):shutil.copytree(ROOT/part,frozen/part)
 bridge='tests/r18/action_reversal_family_oracle_bridge.c';(frozen/'tests/r18').mkdir(parents=True)
 shutil.copy2(HERE/'action_reversal_family_oracle_bridge.c',frozen/bridge)
 lib=frozen/'action.so'
 subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,
                 'src/wf_r18_runtime_handoff.c',bridge,'-o',str(lib)],cwd=frozen,check=True)
 native=ctypes.CDLL(str(lib))
 native.action_reversal_family_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
 native.action_reversal_resolver_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 native.action_reversal_physics_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint]
 machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
 mem.w_block(0,bytes(initial));buffer=ctypes.create_string_buffer(bytes(initial))
 d=(ctypes.c_uint32*8)(*regs)
 for i,value in enumerate(regs):cpu.w_reg(i,value)

 def run_rom(pc,limit=20000):
  cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(pc)
  for i in range(15):cpu.w_reg(i,d[i] if i<8 else 0)
  cpu.w_reg(8,actor);descriptor=0
  for _ in range(limit):
   at=cpu.r_pc()
   if at==0x1c0c8:descriptor=cpu.r_reg(9)
   if at==0x70000:break
   cpu.execute(1)
  assert cpu.r_pc()==0x70000,(steps,hex(pc),'ROM runaway',hex(cpu.r_pc()))
  return descriptor

 def equal(stage,descriptor=None,native_descriptor=None):
  for at,n in ((RAM,0x4000),(0xc0000,0x4000)):
   expected=bytes(mem.r_block(at,n));got=buffer.raw[at:at+n]
   assert got==expected,(steps,stage,'RAM/FG',[(hex(at+i),x,y) for i,(x,y) in enumerate(zip(got,expected)) if x!=y][:12])
  assert list(d)==[cpu.r_reg(i) for i in range(8)],(steps,stage,'D0-D7')
  if descriptor is not None:assert descriptor==native_descriptor,(steps,stage,'descriptor',hex(descriptor),hex(native_descriptor))

 for tick in range(1500):
  state=mem.r16(actor+0x20)
  if not state&0x8000:
   mem.w16(actor+0x1e,mem.r16(actor+0x1c));mem.w16(actor+0x1c,state);mem.w16(actor+0x20,state|0x8000)
  mem.w16(actor+0x36,0)
  if buffer.raw[actor+1]:
   run_rom(0x280dc,5000)
   if mem.r8(actor+0x37):
    for off,corr in ((6,0x38),(10,0x3a),(14,0x3c)):
     mem.w16(actor+off,(mem.r16(actor+off)+mem.r16(actor+corr))&0xffff)
  status=native.action_reversal_resolver_oracle(buffer,d,slot)
  assert status==0,(tick,'native resolver',status)
  equal('resolver')

  descriptor=run_rom(0x1c03e)
  nd=ctypes.c_uint32()
  status=native.action_reversal_family_oracle(buffer,d,slot,ctypes.byref(nd))
  assert status==0,(tick,'native action',status)
  equal('action',descriptor,nd.value)

  run_rom(0x2208,5000)
  status=native.action_reversal_physics_oracle(buffer,d,slot)
  assert status==0,(tick,'native physics',status)
  equal('physics')

  phase=mem.r16(actor+0x44)&(0xffff if args.route=='7a' else 3)
  selector=mem.r16(actor+0x60)&255
  state=mem.r16(actor+0x20)&255
  visual_poses.add(mem.r16(actor+4)&0x7fff)
  if not phase_history or phase_history[-1][1]!=phase:phase_history.append((tick,phase))
  if not selector_history or selector_history[-1][1]!=selector:selector_history.append((tick,selector))
  events.append({'tick':tick,'phase':phase,'selector':selector,'state':state,
                 'x':mem.r16(actor+6),'y':mem.r16(actor+10),'descriptor':descriptor})
  if args.scene_handoff and scene_handoff_at is None and selector==0x6b:
   # Replay the outside-ring scene mode observed in the independent HOST
   # capture at frame 1343; the scene manager is outside this actor harness.
   mem.w16(RAM+0x7e,2);mem.w8(RAM+0x161,2)
   ctypes.memmove(ctypes.addressof(buffer)+RAM+0x7e,b'\x00\x02',2)
   ctypes.memmove(ctypes.addressof(buffer)+RAM+0x161,b'\x02',1)
   scene_handoff_at=tick
  steps+=1
  if (args.route in ('6a','6b','7a') and state==0) or (args.route=='68' and
      (((args.through_69 or args.through_rescue) and state==0) or
       (not (args.through_69 or args.through_rescue) and (selector!=0x68 or state!=5)))):break
 if args.route=='68':
  if args.rescue:
   assert steps<1500,(steps,events[-1])
   if args.through_rescue:
    assert events[-1]['state']==0 and {0x68,0x6a,0x6b}<={event['selector'] for event in events},events[-1]
    assert [selector for _,selector in selector_history]==[0x68,0x6a,0x6b],selector_history
    if args.scene_handoff:assert scene_handoff_at is not None
   else:assert events[-1]['selector']==0x6a and events[-1]['state']==5,events[-1]
  elif args.mode=='royal':
   assert [phase for _,phase in phase_history]==[0] and steps<1500,(steps,phase_history)
   assert events[-1]['selector']==0x7a and events[-1]['state']==5,events[-1]
  else:
   assert [phase for _,phase in phase_history]==([1,2,3] if args.start_frame==960 else [0,1,2,3]),phase_history
   if args.character is None or args.character==source_character:
    expected_phases=([(0,1),(35,2),(175,3)] if args.start_frame==960
                     else [(0,0),(97,1),(192,2),(332,3)])
    assert phase_history==expected_phases,(steps,phase_history)
    expected_steps=({(803,False):352,(803,True):445,(960,False):195,(960,True):288}
                    [(args.start_frame,args.through_69)])
    assert steps==expected_steps,(steps,events[-1])
   else:assert steps<1500,(steps,events[-1])
   if args.through_69:assert events[-1]['state']==0,events[-1]
   else:assert events[-1]['selector']==0x69 and events[-1]['state']==5,events[-1]
 elif args.route=='6a':
  assert steps<1500 and events[-1]['state']==0 and events[-1]['selector']==0x6b,(steps,events[-1])
  assert [selector for _,selector in selector_history]==[0x6a,0x6b],selector_history
  if args.character is None or args.character==source_character:assert steps==132,(steps,events[-1])
 elif args.route=='6b':
  expected_steps={(1343,1):36,(1343,4):36,(1436,0):40}
  assert steps==expected_steps[(args.start_frame,slot)] and events[-1]['state']==0,(steps,events[-1])
  assert events[-1]['selector']==0x6b,events[-1]
 else:
  assert steps<1500 and events[-1]['state']==0 and events[-1]['selector']==0,(steps,events[-1])
  assert [selector for _,selector in selector_history]==[0x7a,0],selector_history
  assert [phase for _,phase in phase_history]==[0,1,2,3,4,5,0],phase_history
  graphics_report=json.loads((ROOT.parent.parent/'validation/local-20261026/royal-exit-poses-direct-rom.json').read_bytes())
  assert graphics_report['status']=='PASS' and graphics_report['rom_sha1']==meta['rom_sha1']
  assert graphics_report['verifier_sha256']==hashlib.sha256((ROOT/'tools/verify_r18_metasprite_rom.py').read_bytes()).hexdigest()
  assert graphics_report['metasprite_source_sha256']==hashlib.sha256((ROOT/'src/wf_r18_metasprite.c').read_bytes()).hexdigest()
  assert visual_poses=={int(value,16) for value in graphics_report['royal_exit_poses']},visual_poses
  if args.character is None or args.character==source_character:
   assert steps==233,(steps,events[-1])
  if args.palette_allocation:
   palette_character=args.character if args.character is not None else source_character
   assert mem.r8(RAM+0x1610+palette_character)==0 and mem.r8(RAM+0x1603)==0
 transfer_count=mem.r16(RAM+0x1828)
 transfer_commands=list(mem.r_block(RAM+0x1808,2))
 if args.through_69:assert transfer_count==2 and transfer_commands==[1,2]
 assert bytes(mem.r_block(0,len(rom)))==rom and buffer.raw[:len(rom)]==rom
 if args.emit_terminal_state:
  terminal_state_name=f'action68-rescue-terminal-character{args.character:02d}.state'
  terminal_state=(b'R68T'+struct.pack('=8I',*d)+
                  bytes(mem.r_block(RAM,0x4000))+bytes(mem.r_block(0xc0000,0x4000)))
  terminal_state_sha256=hashlib.sha256(terminal_state).hexdigest()
  (ROOT.parent.parent/'validation/local-20261026'/terminal_state_name).write_bytes(terminal_state)
 machine.cleanup()
 if os.name=='nt':assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))

report={'status':'PASS','route':args.route,'steps':steps,'start_frame':args.start_frame,
        'start_slot':slot,'mode':'royal-capture' if args.route=='7a' else args.mode,'rescue':args.rescue,
        'through_rescue':args.through_rescue,'scene_handoff_at':scene_handoff_at,
        'through_69':args.through_69,
        'source_character':source_character,'character':args.character if args.character is not None else source_character,
        'start_snapshot_sha256':item['sha256'],
        'source_transition_report_sha256':hashlib.sha256(transition_report_path.read_bytes()).hexdigest() if args.route=='6a' else None,
        'terminal_state_name':terminal_state_name,'terminal_state_sha256':terminal_state_sha256,
        'outside_scene_snapshot_sha256':outside_capture_sha256,
        'rom_sha1':meta['rom_sha1'],'phase_history':phase_history,
        'selector_history':selector_history,'visual_poses':[f'{pose:04X}' for pose in sorted(visual_poses)],
        'last':events[-1],
        'controlled_inputs':{
            'character_replaced_in_snapshot':args.character is not None and args.character!=source_character,
            'royal_mode_bit_set':args.mode=='royal',
            'tag_rescue_flags_set':args.rescue,
            'outside_scene_mode_replayed_from_independent_capture':args.scene_handoff,
            'palette_allocation_seeded':args.palette_allocation},
        'transfer_queue_count':transfer_count,
        'transfer_commands':transfer_commands,
        'source_sha256':hashlib.sha256((ROOT/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest(),
        'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'bridge_sha256':hashlib.sha256((HERE/'action_reversal_family_oracle_bridge.c').read_bytes()).hexdigest(),
        'compared':['RAM16KB','FG16KB','D0-D7','descriptor after each action tick'],
        'limits':['Isolated projection from a HOST capture or a verified native/ROM scene output: other actors do not run.',
                  'Character, Royal, rescue, scene-mode and palette-allocation variants are controlled mutations, not independently captured natural scenes.',
                  ('Postscene state contains 16 KiB of work RAM and foreground RAM.' if args.route=='6a'
                   else 'Only 8 KiB of captured arcade work RAM was supplied; unrepresented bytes are zero.'),
                  'No physical Saturn, audio, VDP1 or frame-rate equivalence claim.']}
suffix='-through69' if args.through_69 else ''
mode_suffix='-royal' if args.mode=='royal' else ''
rescue_suffix='-rescue' if args.rescue else ''
rescue_end_suffix='-through6b' if args.through_rescue else ''
scene_suffix='-outside-scene' if args.scene_handoff else ''
character_suffix=f'-character{args.character:02d}' if args.character is not None else ''
report_name=(f'action68-interleaved-natural-{args.start_frame}{suffix}{mode_suffix}{rescue_suffix}{rescue_end_suffix}{scene_suffix}.json' if args.route=='68'
             else (f'action6a-interleaved-postscene-{args.start_frame}-slot{slot}.json' if args.route=='6a'
                   else (f'action6b-interleaved-natural-{args.start_frame}-slot{slot}.json' if args.route=='6b'
                         else f'action7a-interleaved-natural-{args.start_frame}-slot{slot}.json')))
if character_suffix:report_name=report_name[:-5]+character_suffix+'.json'
if args.palette_allocation:report_name=report_name[:-5]+'-palalloc.json'
out=ROOT.parent.parent/'validation/local-20261026'/report_name
out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS',steps,'resolver/action/physics steps; phases',phase_history,'last',events[-1])
