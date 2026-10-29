#!/usr/bin/env python3
"""Compare actual native C state+assets+native painter order with MAME PNGs."""
from pathlib import Path
import csv
import functools
import hashlib
import json
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parent
G=ROOT/'generated'
MAME=ROOT.parent/'verification/mame/full'
m=json.loads((G/'assets.json').read_text())
atlas=(G/'TEX.BIN').read_bytes()
palettes={x['base']:np.array([[(v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17]for v in x['rgb444']],dtype=np.uint8)for x in m['palette_banks']}
ringidx=np.frombuffer((G/'RING.BIN').read_bytes(),dtype=np.uint8).reshape(256,512)[:240,:320]
ringpal=np.array([[(v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17]for v in m['ring_palette']],dtype=np.uint8)
ring=ringpal[ringidx]
pose_frames={(q['set'],q['pose']):q['frame']for q in m['poses']}
name_frames={(q['identity'],q['slot']):q['frame']for q in m['name_frames']}
states={}
with (ROOT/'native_scene.csv').open() as f:
    for row in csv.DictReader(f):
        q={k:int(v)for k,v in row.items()}
        states.setdefault(q['index'],[]).append(q)


@functools.lru_cache(maxsize=None)
def texture(tile,width,height):
    packed=np.frombuffer(atlas,dtype=np.uint8,count=width*height//2,offset=tile*128).reshape(height,width//2)
    pixels=np.empty((height,width),dtype=np.uint8)
    pixels[:,::2]=packed>>4;pixels[:,1::2]=packed&15
    return pixels


def draw_frame(rgb,frame,ax=0,ay=0,mirror=False):
    f=m['frames'][frame]
    for p in m['pieces'][f['first']:f['first']+f['count']]:
        x=ax+(-p['x']if mirror else p['x']);y=ay+p['y']
        x0,y0=max(x,0),max(y,0);x1,y1=min(x+p['width'],320),min(y+p['height'],240)
        if x0>=x1 or y0>=y1:continue
        flip=p['flip']^(1 if mirror else 0)
        tile=texture(p['tile'],p['width'],p['height'])
        if flip&1:tile=tile[:,::-1]
        if flip&2:tile=tile[::-1,:]
        cut=tile[y0-y:y1-y,x0-x:x1-x]
        use=np.ones(cut.shape,dtype=bool)if p['flags']&1 else cut!=0
        rgb[y0:y1,x0:x1][use]=palettes[p['palette']][cut[use]]


@functools.lru_cache(maxsize=10)
def render(index):
    actors=states[index]
    rgb=ring.copy()
    for a in actors:
        if not a['active']&0x8000:continue
        key=(a['type'],a['selector']&0x7fff)
        if key not in pose_frames:raise RuntimeError(f'Missing native pose {key}')
        ax=(a['x']>>16)-480
        ay=232-((a['y']>>16)+(a['z']>>16)-560)
        draw_frame(rgb,pose_frames[key],ax,ay,bool(a['selector']&0x8000))
    draw_frame(rgb,m['foreground_frame'])
    draw_frame(rgb,m['credit_frame'])
    if actors[0]['names']:
        # Actual root order: logical slots0,1,2,3; the cast is the MAME fixture.
        for slot,identity in enumerate([0,6,9,1]):draw_frame(rgb,name_frames[identity,slot])
        draw_frame(rgb,m['vs_frame'])
    return rgb


rows=[]
for p in sorted(MAME.glob('sn*.png')):
    index=int(p.stem[2:])
    if index==0 or index>=1569:continue
    expected=np.asarray(Image.open(p).convert('RGB'))
    attempts=[]
    for offset in [-1,0,1]:
        j=index+offset
        if j not in states:continue
        d=np.any(render(j)!=expected,axis=2)
        attempts.append((int(d.sum()),offset))
    best=min(attempts)
    rows.append({'mame_frame':index,'different_pixels':best[0],'native_index_offset':best[1], 'attempts':attempts})
    if best[0] and len([r for r in rows if r['different_pixels']])<=4:
        Image.fromarray(render(index+best[1])).save(G/f'native_mismatch_{index:04}.png')
summary={'source':'Native wp02_scene.c executed as host C; generated atlas; exact root renderer order/mirror/anchor equations','checked_images':len(rows),'exact_images':sum(r['different_pixels']==0 for r in rows),'maximum_different_pixels':max(r['different_pixels']for r in rows),'total_different_pixels':sum(r['different_pixels']for r in rows),'offset_counts':{str(o):sum(r['native_index_offset']==o for r in rows)for o in [-1,0,1]},'rows':rows}
(G/'native_composition_audit.json').write_text(json.dumps(summary,indent=2)+'\n')
for k,v in summary.items():
    if k!='rows':print(k,v)
print('first mismatches',[r for r in rows if r['different_pixels']][:8])

# Hash the exact native update images without writing 1570 separate PNGs.
# Keys correspond to host wp02_tick calls minus1 (CSV index), not MAME PNG
# counters: MAME screenshots display the prior native update because its
# sprite buffer is latched at VBlank. Retain every index even if images repeat.
hashes={str(i):hashlib.sha256(render(i).tobytes()).hexdigest()for i in sorted(states)if i<=1569}
hash_report={'width':320,'height':240,'format':'RGB888 row-major, no padding',
             'seed':'0xCEB7C58E','raw_identities':[0,6,9,1],
             'index_semantics':'native wp02_tick call count minus1; MAME PNG n matches native n-1',
             'first_index':min(map(int,hashes)),'last_index':max(map(int,hashes)),
             'terminal_clear_index':1569,'sha256_by_native_update':hashes}
(G/'reference_frame_hashes.json').write_text(json.dumps(hash_report,indent=2)+'\n')
print('hashed_native_frames',len(hashes),'unique_images',len(set(hashes.values())))
