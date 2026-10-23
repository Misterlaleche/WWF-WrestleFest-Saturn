#!/usr/bin/env python3
"""Gate a completed, deterministic SH-2 profile; emulator VBlanks, not host FPS."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('profile',type=Path);p.add_argument('--start',type=int,default=30);p.add_argument('--stop',type=int,default=120);p.add_argument('--max-vblanks-per-frame',type=float,default=2.0);a=p.parse_args();d=json.loads(a.profile.read_text())
assert d['frame']==a.stop and d['profile_frames']==a.stop-a.start
assert not d['fatal_owner'] and not d['fatal_dependency']
assert d['profile_calls'][7:9]==[0,0], 'This comparison requires the precached sequence'
ratio=d['profile_vblanks']/d['profile_frames']
print(f"frames{a.start}–{a.stop}: {d['profile_vblanks']} VBlanks/{d['profile_frames']} frames = {ratio:.3f} VBlanks/frame")
assert ratio<=a.max_vblanks_per_frame, f'Frame budget exceeded: {ratio:.3f} > {a.max_vblanks_per_frame}'
print('PASS measured frame budget; no CD reads or forced/skipped gameplay')
