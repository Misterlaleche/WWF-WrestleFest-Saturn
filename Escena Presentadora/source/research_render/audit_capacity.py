#!/usr/bin/env python3
"""Count native VDP1 commands for every allowed roster and the full fixture."""
import csv
import itertools
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parent
OUT=ROOT/'generated'
m=json.loads((OUT/'assets.json').read_text())
frames=m['frames'];pieces=m['pieces']
poses={(q['set'],q['pose']):q['frame']for q in m['poses']}
names={(q['identity'],q['slot']):q['frame']for q in m['name_frames']}


def count(frame,ax=0,ay=0,mirror=False):
    f=frames[frame]
    return sum(ax+(-p['x']if mirror else p['x'])<320
               and ax+(-p['x']if mirror else p['x'])+p['width']>0
               and ay+p['y']<240 and ay+p['y']+p['height']>0
               for p in pieces[f['first']:f['first']+f['count']])


def text_count(ids,legacy=False,deduplicate=True):
    out=frames[m['vs_frame']]['unflattened_count']if legacy else count(m['vs_frame'])
    for slot,identity in enumerate(ids):
        if deduplicate and slot%2 and identity==11 and ids[slot-1]==10:continue
        f=names[identity,slot]
        out+=frames[f]['unflattened_count']if legacy else count(f)
    return out


singles=[0,1,2,3,6,7,8,9]
teams=list(itertools.permutations(singles,2))+[(10,11)]
rosters=[a+b for a in teams for b in teams if len(set(a+b))==4]
assert len(rosters)==1792
anchors=[(96,136,True),(64,120,True),(224,136,False),(256,120,False)]
fixed=4+count(m['foreground_frame'])+count(m['credit_frame'])+count(poses[12,0],288,64)
initial=[];bounds=[]
for ids in rosters:
    initial.append((fixed+count(poses[31,2],160,96)+text_count(ids)
                    +sum(count(poses[i,0],*anchors[slot])for slot,i in enumerate(ids)),ids))
    # Conservative whole-scene bound: all actors may use their largest resident
    # pose simultaneously, all names visible, and no actor clipping is credited.
    actor_max=sum(max(frames[q['frame']]['count']for q in m['poses']if q['set']==i)for i in ids)
    presenter_max=max(frames[q['frame']]['count']for q in m['poses']if q['set']==31)
    bounds.append((fixed+actor_max+presenter_max+text_count(ids),ids))

states={}
with (ROOT/'native_scene.csv').open()as f:
    for row in csv.DictReader(f):
        p={k:int(v)for k,v in row.items()}
        states.setdefault(p['index'],[]).append(p)
fixture=[]
for index,actors in sorted(states.items()):
    native=4+count(m['foreground_frame'])+count(m['credit_frame'])
    legacy=4+count(m['foreground_frame'])+frames[m['credit_frame']]['unflattened_count']
    for actor in actors:
        if not actor['active']&0x8000:continue
        f=poses[actor['type'],actor['selector']&0x7fff]
        ax=(actor['x']>>16)-480
        ay=232-((actor['y']>>16)+(actor['z']>>16)-560)
        n=count(f,ax,ay,bool(actor['selector']&0x8000))
        native+=n;legacy+=n
    if actors[0]['names']:
        native+=text_count((0,6,9,1))
        legacy+=text_count((0,6,9,1),legacy=True)
    fixture.append({'native_index':index,'commands_before':legacy,'commands_after':native})

worst=max(initial);bound=max(bounds)
summary={'canonical_rosters':len(rosters),'maximum_initial_commands':worst[0],
         'maximum_initial_roster':worst[1],
         'conservative_all_poses_commands':bound[0],
         'conservative_all_poses_roster':bound[1],
         'command_capacity':256,'texture_bytes':m['texture_bytes'],
         'vdp1_bytes_with_commands_and_alignment':m['texture_bytes']+256*32+32,
         'vdp1_capacity':524288,
         'fixture_initial_commands_before':fixture[0]['commands_before'],
         'fixture_initial_commands_after':fixture[0]['commands_after'],
         'fixture_peak_commands_before':max(r['commands_before']for r in fixture),
         'fixture_peak_commands_after':max(r['commands_after']for r in fixture),
         'fixture_frames':len(fixture),
         'static_text_frames':[{'label':f['label'],'before':f['unflattened_count'],'after':f['count'],
                               'transparent_cells_removed':f['blank_cells'],'offscreen_cells_removed':f['offscreen_cells']}
                              for f in frames if 'unflattened_count'in f]}
(OUT/'capacity_audit.json').write_text(json.dumps(summary,indent=2)+'\n')
(OUT/'command_timeline.json').write_text(json.dumps(fixture,separators=(',',':'))+'\n')
print(json.dumps({k:v for k,v in summary.items()if k!='static_text_frames'},indent=2))
assert worst[0]<256 and bound[0]<256
assert summary['vdp1_bytes_with_commands_and_alignment']<=524288
