#!/usr/bin/env python3
"""Reject stalled music, torn FRT duration reads, and graphical regressions."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('capture',type=Path);a=p.parse_args()
video=[r for r in csv.DictReader((a.capture/'telemetry.csv').open()) if int(r['boot_stage'])==3]
audio=[r for r in csv.DictReader((a.capture/'audio-telemetry.csv').open()) if int(r['playing'])]
assert len(video)>400 and len(audio)>400,'Not enough running scene/audio frames'
assert not any(int(r['error']) for r in video),'Runtime error'
assert max(int(r['missed_vblanks']) for r in video)==0,'Lost graphical presentation'
max_handler=max(int(r['max_handler_ticks']) for r in audio)
assert max_handler<14037,f'Audio handler exceeds one frame or FRT read tears: {max_handler}'
assert max(int(r['event_index']) for r in audio)>2000,'Music sequence did not complete'
for prev,cur in zip(audio,audio[1:]):
    if int(cur['interrupts'])<int(prev['interrupts']):continue
    delta=int(cur['elapsed_ticks'])-int(prev['elapsed_ticks'])
    assert 0<=delta<40000,f'Unexpected audio-clock discontinuity: {delta}'
print(json.dumps({'status':'PASS','running_frames':len(video),'loops':int(video[-1]['loops']),
    'max_audio_handler_ticks':max_handler,
    'max_dispatch_lateness_ticks':max(int(r['max_lateness_ticks']) for r in audio),
    'max_work_ticks':max(int(r['max_work_ticks']) for r in video),'missed_vblanks':0},indent=2))
