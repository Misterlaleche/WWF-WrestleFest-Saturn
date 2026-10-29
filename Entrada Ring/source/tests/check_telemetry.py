#!/usr/bin/env python3
"""Run only against actual emulator telemetry, not synthetic frame counters."""
import argparse,csv,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('telemetry',type=Path);a=p.parse_args()
rows=list(csv.DictReader(a.telemetry.open()))
live=[{k:int(v) for k,v in r.items()} for r in rows if int(r['boot_stage'])==3]
assert len(live)>=450,'Need an entire scene plus an automatic replay'
assert not any(r['error'] for r in live),'Runtime hardware/asset error'
assert max(r['loops'] for r in live)>=2,'Automatic replay was not exercised'
assert max(r['missed_vblanks'] for r in live)==0,'Missed Saturn VBlank during scene/restart'
assert max(r['sprite_count'] for r in live)<=84,'Sprite command budget exceeded'
assert all(r['left_id']!=r['right_id'] for r in live),'Random pair repeated one identity'
print(json.dumps({'pass':True,'frames':len(live),'loops':max(r['loops'] for r in live),
 'missed_vblanks':0,'max_work_ticks':max(r['max_work_ticks'] for r in live),
 'max_sprite_cells':max(r['sprite_count'] for r in live)}))
