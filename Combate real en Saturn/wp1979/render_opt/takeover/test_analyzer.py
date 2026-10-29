#!/usr/bin/env python3
"""Synthetic parser checks only; no capture, emulator, or runtime mutation."""
from pathlib import Path
import copy
import json
import analyze_takeover as module

here=Path(__file__).resolve().parent
directory=here/'build/analyzer-fixture';directory.mkdir(parents=True,exist_ok=True)
schedule_path=here/'controller-schedule.json';schedule=json.loads(schedule_path.read_text())
module.ENTRY=here/'wp1979_entry-proposed.c' # audit the corrected mapper fixture
samples=[];x=592<<16;y=336<<16
for frame in range(500,961):
 buttons=[]
 for event in schedule:
  if event['frame']<=frame-2:buttons=event['buttons']
 active=sum(module.ACTIVE[b] for b in buttons);held=sum(module.ACTIONS.get(b,0) for b in buttons)
 direction=active&15
 if len(buttons)==1 and buttons[0] in module.WALK:
  dx,dy=module.WALK[buttons[0]];x+=dx;y+=dy
 actor=bytearray(176);actor[0]=128;actor[0x33]=1
 actor[0x20:0x22]=(0x8001 if direction else 0x8000).to_bytes(2,'big')
 actor[6:10]=x.to_bytes(4,'big');actor[10:14]=y.to_bytes(4,'big')
 actor[0xa6:0xa8]=held.to_bytes(2,'big');actor[0xa8:0xaa]=direction.to_bytes(2,'big')
 samples.append(dict(emulator_frame=frame,wp1979_started=1,game_frame=frame-474,
                     wp1979_demo_active=int(frame<542),fatal_owner=0,fatal_dependency=0,actor0=actor.hex()))
base=dict(status='complete',frames_run=961,synthetic=True,hardware_test=False,
          events=[dict(e,applied_at_frame=e['frame']) for e in schedule],samples=samples)
(directory/'identity.json').write_text(json.dumps({'a_bin_matches':True,'sha256':{'fixture':'SYNTHETIC'}}))
(directory/'command.json').write_text(json.dumps(['python',str(module.CAPTURE),'--schedule',str(schedule_path)]))
def run(report):
 (directory/'report.json').write_text(json.dumps(report))
 return module.analyze(directory,schedule_path,allow_synthetic=True)
positive=run(base);assert positive['status']=='pass',positive['failures']
bad=copy.deepcopy(base)
for sample in bad['samples']:sample['wp1979_demo_active']=1
no_transition=run(bad);assert no_transition['status']=='fail' and any('transition' in f for f in no_transition['failures'])
bad=copy.deepcopy(base)
for sample in bad['samples']:
 actor=bytearray.fromhex(sample['actor0']);old=int.from_bytes(actor[10:14],'big')
 actor[10:14]=((672<<16)-old).to_bytes(4,'big');sample['actor0']=actor.hex()
reversed_vertical=run(bad)
assert reversed_vertical['status']=='fail' and any('opposite physical direction' in f for f in reversed_vertical['failures'])
# Leave this fixture explicitly synthetic, with the valid case for inspection.
run(base)
result=dict(kind='synthetic-analyzer-regression',positive=positive['status'],
            rejects_missing_takeover=no_transition['status']=='fail',
            rejects_reversed_vertical=reversed_vertical['status']=='fail',
            actual_capture_performed=False)
(here/'analyzer-test-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
