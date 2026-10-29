#!/usr/bin/env python3
"""Read-only proof from a normal-controller Yabause capture and actor samples.

This program consumes capture JSON. It does not load an emulator or access
emulated memory. A failed/incomplete movement check is reported, never patched.
"""
from pathlib import Path
import argparse
import ast
import collections
import hashlib
import json
import re
import sys

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parents[1]
CAPTURE=PROJECT/'tooling/capture_yabause.py'
ENTRY=PROJECT/'native/src/runtime/src/wp1979_entry.c'
INPUT=PROJECT/'native/src/runtime/src/wp1979_input.c'
BUTTONS={'up':4,'down':5,'left':6,'right':7,'a':0,'b':8,'c':11,
         'x':1,'y':9,'z':10,'l':12,'r':13,'start':3}
ACTIVE={'right':1,'left':2,'up':8,'down':4,'a':16,'b':32,'c':64,'start':128}
ACTIONS={'a':1,'b':2,'c':4,'start':256}
WALK={'left':(-138240,0),'right':(138240,0),'up':(0,-110592),'down':(0,110592)}

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def actor(sample):
 b=bytes.fromhex(sample['actor0'])
 if len(b)<0xaa:raise ValueError('actor0 watch must include bytes through +A9')
 w=lambda off:int.from_bytes(b[off:off+2],'big')
 q=lambda off:int.from_bytes(b[off:off+4],'big',signed=True)
 return dict(character=w(2),active=bool(b[0]&128),legal=bool(b[0x33]&1),
             state=w(0x20)&0x7fff,x=q(6),y=q(10),direction=w(0xa8)&15,
             held=w(0xa6),fresh=w(0xa2))

def source_route_audit():
 source=CAPTURE.read_text();tree=ast.parse(source)
 funcs={n.name:n for n in tree.body if isinstance(n,(ast.FunctionDef,ast.AsyncFunctionDef))}
 mapping=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
              and any(isinstance(t,ast.Name) and t.id=='buttons' for t in n.targets))
 assert mapping==BUTTONS,'Capture controller labels changed; review mapping'
 input_text=ast.get_source_segment(source,funcs['input_state'])
 reader_text=ast.get_source_segment(source,funcs['read_memory'])
 assert 'button in pad[port]' in input_text and 'device == 1' in input_text
 assert 'C.c_ubyte.from_address' in reader_text and '.value' in reader_text
 for n in ast.walk(funcs['read_memory']):
  if isinstance(n,(ast.Assign,ast.AugAssign,ast.AnnAssign)):
   targets=n.targets if isinstance(n,ast.Assign) else [n.target]
   assert all(isinstance(t,ast.Name) for t in targets),'Memory reader contains a nonlocal write'
 forbidden={'memmove','memset','retro_unserialize','retro_cheat_set','M68KSetRegisters','SH2SetRegisters'}
 for n in ast.walk(tree):
  if isinstance(n,ast.Call):
   name=n.func.attr if isinstance(n.func,ast.Attribute) else n.func.id if isinstance(n.func,ast.Name) else ''
   assert name not in forbidden,f'Unexpected mutation API: {name}'
 controls=dict((name,int(mask,16)) for name,mask in re.findall(
  r'sample->pressed\.button\.(\w+)\)\s+bits\s*&=\s*\(uint8_t\)~0x([0-9a-fA-F]+)u',ENTRY.read_text()))
 assert controls==ACTIVE,'Native controls mapping changed; review expected levels'
 return dict(status='pass',controller_method='normal retro_set_input_state callback; scheduled pad levels',
             observation_method='read_memory reads bytes; analyzer reads JSON only',
             mutation_apis_detected=[],button_mapping=mapping,
             sha256={str(p.relative_to(PROJECT)):sha(p) for p in (CAPTURE,ENTRY,INPUT)})

def analyze(directory,schedule_path,allow_synthetic=False):
 directory=Path(directory);schedule_path=Path(schedule_path)
 report=json.loads((directory/'report.json').read_text())
 schedule=json.loads(schedule_path.read_text())
 identity=json.loads((directory/'identity.json').read_text()) if (directory/'identity.json').exists() else {}
 command=json.loads((directory/'command.json').read_text()) if (directory/'command.json').exists() else []
 failures=[]
 def check(ok,reason):
  if not ok:failures.append(reason)
 check(not report.get('synthetic') or allow_synthetic,'Synthetic fixture is not an actual capture')
 check(report.get('status')=='complete','Capture did not complete')
 check(identity.get('a_bin_matches') is True,'Capture lacks verified disc A.BIN/linked-binary identity')
 try:
  audit=source_route_audit()
 except (AssertionError,StopIteration,KeyError) as exc:
  audit=dict(status='fail',reason=str(exc));failures.append('Controller/memory observation source audit failed')
 check(str(CAPTURE) in command,'Recorded capture command does not select the audited capture script')
 check('--schedule' in command,'Capture command lacks --schedule')
 if '--schedule' in command:
  actual=Path(command[command.index('--schedule')+1]).resolve()
  check(actual==schedule_path.resolve(),'Capture command used a different schedule path')
 expected_events=[dict(e,applied_at_frame=e['frame']) for e in schedule]
 check(report.get('events')==expected_events,'Reported applied controller events differ from the requested schedule')
 check(all(e.get('port',0)==0 for e in schedule),'This proof expects only player-one controller events')
 first_press=next(e['frame'] for e in schedule if e.get('buttons'))
 samples=sorted(report.get('samples',[]),key=lambda s:s['emulator_frame'])
 running=[s for s in samples if s.get('wp1979_started')]
 check(bool(running),'No started gameplay samples')
 check(not any(s.get('fatal_owner') or s.get('fatal_dependency') for s in samples),'Runtime reported a fatal owner/dependency')
 prior=[s for s in running if s['emulator_frame']<first_press]
 after=[s for s in running if s['emulator_frame']>=first_press]
 check(bool(prior) and prior[-1].get('wp1979_demo_active')==1,'Replay was not active immediately before the first physical event')
 check(all(s.get('wp1979_demo_active')==1 for s in prior),'Replay changed state before a physical event')
 transition=next((s for s in after if s.get('wp1979_demo_active')==0),None)
 check(transition is not None,'No observed demo_active 1→0 transition')
 if transition:
  latency=transition['emulator_frame']-first_press
  check(0<=latency<=8,'Takeover was not observed within eight emulated frames of the controller event')
  check(all(s.get('wp1979_demo_active')==0 for s in after if s['emulator_frame']>=transition['emulator_frame']),
        'Replay reactivated after physical takeover')
 else:latency=None
 check(bool(after) and after[-1].get('game_frame',0)>after[0].get('game_frame',0),'Native gameplay did not advance after takeover')
 check(all('actor0' in s for s in after),'Missing actor0 samples; run capture with --actors')
 decoded={}
 for s in after:
  if 'actor0' in s:
   try:decoded[s['emulator_frame']]=actor(s)
   except (ValueError,KeyError) as exc:failures.append(str(exc));break
 check(bool(decoded) and all(v['character']==0 for v in decoded.values()),'Observed player actor is not consistently Hogan (character ID 0)')
 phases=[];walking_directions=[]
 for index,event in enumerate(schedule):
  buttons=event.get('buttons',[])
  if not buttons:continue
  start=event['frame'];end=schedule[index+1]['frame'] if index+1<len(schedule) else report['frames_run']
  active=sum(ACTIVE.get(b,0) for b in set(buttons));direction=active&15
  held=sum(ACTIONS.get(b,0) for b in set(buttons))
  observations=[(s,decoded[s['emulator_frame']]) for s in after
                if start+3<=s['emulator_frame']<end and s['emulator_frame'] in decoded]
  matches=[(s,v) for s,v in observations if v['direction']==direction and v['held']==held]
  phase=dict(buttons=buttons,start_emulator_frame=start,end_emulator_frame=end,
             expected_hardware_low=f'0x{255^active:02x}',expected_direction=direction,
             expected_held=held,decoded_matching_samples=len(matches),observed_samples=len(observations),
             main_states=dict(collections.Counter(str(v['state']) for _,v in observations)))
  check(bool(matches),f'No matching native decoded input for {buttons} during frames {start}..{end-1}')
  if len(buttons)==1 and buttons[0] in WALK:
   target=WALK[buttons[0]];steps=[];opposite=[]
   for (sa,va),(sb,vb) in zip(observations,observations[1:]):
    if sb.get('game_frame',0)-sa.get('game_frame',0)!=1:continue
    if not all(v['state']==1 and v['active'] and v['legal'] and v['direction']==direction for v in (va,vb)):continue
    delta=(vb['x']-va['x'],vb['y']-va['y'])
    if delta==target:steps.append(sb['emulator_frame'])
    axis=0 if target[0] else 1
    if delta[axis]*target[axis]<0:opposite.append(dict(emulator_frame=sb['emulator_frame'],delta_fixed=delta))
   phase['exact_hogan_walk_steps']=len(steps)
   phase['first_exact_walk_frame']=steps[0] if steps else None
   phase['expected_step_fixed']=list(target)
   phase['opposite_walk_state_steps']=opposite
   check(not (len(opposite)>=3 and not steps),f'{buttons[0]} produced repeated walking motion in the opposite physical direction')
   if matches:
    phase['observed_world_start']=[matches[0][1]['x']/65536,matches[0][1]['y']/65536]
    phase['observed_world_end']=[matches[-1][1]['x']/65536,matches[-1][1]['y']/65536]
   if steps:walking_directions.append(buttons[0])
  phases.append(phase)
 exact_steps=sum(p.get('exact_hogan_walk_steps',0) for p in phases)
 check(len(walking_directions)>=2 and exact_steps>=12,
       'Need at least twelve exact Hogan walking steps across two direction intervals; combat/ropes may have blocked this attempt')
 last_release=max(e['frame'] for e in schedule if not e.get('buttons'))
 neutral=[s for s in after if s['emulator_frame']>=last_release+8 and s['emulator_frame'] in decoded
          and decoded[s['emulator_frame']]['direction']==0 and decoded[s['emulator_frame']]['held']==0]
 check(len(neutral)>=16,'Not enough neutral native samples after the final physical release')
 return dict(status='pass' if not failures else 'fail',failures=failures,
             kind='synthetic-analyzer-test' if report.get('synthetic') else 'emulator-normal-controller-takeover',
             hardware_test=False,capture_directory=str(directory.resolve()),schedule_sha256=sha(schedule_path),
             capture_core_sha256=report.get('core_sha256'),capture_disc_sha256=identity.get('sha256'),
             source_route_audit=audit,
             transition=dict(first_physical_emulator_frame=first_press,
                 demo_off_emulator_frame=transition['emulator_frame'] if transition else None,
                 demo_off_game_frame=transition.get('game_frame') if transition else None,
                 callback_to_observed_takeover_frames=latency),
             phases=phases,walking_directions=walking_directions,exact_hogan_walk_steps=exact_steps,
             neutral_samples_after_final_release=len(neutral),
             limitations=['This is an emulator controller-callback test, not physical Saturn hardware.',
                          'Exact walking-step evidence is selected only while Hogan is active, legal, and in walk state 1; combat-driven movement is reported separately.'])

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('capture_directory',type=Path)
 p.add_argument('--schedule',type=Path,default=HERE/'controller-schedule.json')
 p.add_argument('--out',type=Path)
 args=p.parse_args()
 result=analyze(args.capture_directory,args.schedule)
 out=args.out or args.capture_directory/'takeover-analysis.json'
 out.parent.mkdir(parents=True,exist_ok=True)
 out.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result,indent=2))
 return 0 if result['status']=='pass' else 1

if __name__=='__main__':sys.exit(main())
