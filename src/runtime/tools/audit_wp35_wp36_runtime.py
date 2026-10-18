#!/usr/bin/env python3
import csv, re, struct, hashlib, json
from collections import defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CSV=ROOT/'authority/wp25-metasprite-expanded-pieces.csv'
IDX=ROOT/'assets/combat_pose_index_be.bin'
PCS=ROOT/'assets/combat_pose_pieces_be.bin'
OUT=ROOT/'audit/wp35-wp36'
OUT.mkdir(parents=True,exist_ok=True)
POSES=565
SETS=12

def row_tuple(x):
    return (int(x['tile_code'],16),int(x['palette_resource'],16),int(x['flip_x'])|(int(x['flip_y'])<<1),int(x['rel_x']),int(x['rel_y']),int(x['chain_index']),int(x['chain']))

groups=defaultdict(list); partner_counts=defaultdict(int)
with CSV.open(newline='') as f:
    for x in csv.DictReader(f):
        s=int(x['set'],16);p=int(x['pose']);ps=x['partner_set']
        if ps:
            partner_counts[(s,p,int(ps,16))]+=1
        else:
            groups[(s,p)].append(row_tuple(x))
idx=IDX.read_bytes();pcs=PCS.read_bytes()
assert len(idx)==SETS*POSES*6

def runtime_pose(s,p):
    o=(s*POSES+p)*6
    st,c=struct.unpack_from('>IH',idx,o)
    got=[]
    for i in range(c):got.append(struct.unpack_from('>HBBhhBB',pcs,(st+i)*10))
    return st,c,got

matrix=[]; exact=0; empty=0; mism=[]
for s in range(SETS):
    for p in range(POSES):
        exp=groups.get((s,p),[]);st,c,got=runtime_pose(s,p)
        if exp:
            status='EXACT' if got==exp else 'MISMATCH'
            if status=='EXACT':exact+=1
            else:mism.append((s,p,len(exp),c))
        else:
            status='EMPTY' if c==0 else 'EXTRA_RUNTIME_COMPOSITION'
            if status=='EMPTY':empty+=1
            else:mism.append((s,p,0,c))
        matrix.append((f'0x{s:02X}',f'0x{p:04X}',len(exp),c,status,st))
with (OUT/'unpaired-rom-vs-runtime.csv').open('w',newline='') as f:
    w=csv.writer(f);w.writerow(['gfx_set','pose','authority_unpaired_count','runtime_count','status','runtime_start']);w.writerows(matrix)
assert not mism, f'unpaired mismatches: {mism[:10]}'

# WP36 pinned regressions from accepted handoff.
def check_count(s,p,n):
    _,c,_=runtime_pose(s,p);assert c==n,(s,p,c,n)
check_count(1,0x62,23);check_count(1,0x63,23);check_count(10,0x5b,20);check_count(4,0x68,23)
# Warrior bbox and palette.
for p in (0x62,0x63):
    _,c,g=runtime_pose(1,p);assert c==23
    xs=[x[3] for x in g];ys=[x[4] for x in g]
    assert min(xs)==-34 and min(ys)==-98
    assert max(x+16 for x in xs)-min(xs)==64
    assert max(y+16 for y in ys)-min(ys)==112
    assert {x[1] for x in g}=={1}
# Bossman partner case exactly 86 in authority, absent from unpaired runtime pack.
assert partner_counts[(3,0x178,1)]==86
_,c,_=runtime_pose(3,0x178);assert c==0
# Maximum partner-expanded count, used as capacity sentinel.
max_pair=max(partner_counts.items(),key=lambda kv:kv[1]);assert max_pair==((3,0x178,1),86),max_pair

# Runtime animation-table pose superset. This is deliberately not claimed exact per-character reachability.
text=(ROOT/'generated/wf_animation_tables.c').read_text()
poses=sorted({int(x,16)&0x7fff for x in re.findall(r'\{0x([0-9A-Fa-f]{4})u,0x[0-9A-Fa-f]{4}u\}',text)})
reach=[]; counts=defaultdict(int)
partner_by_sp=defaultdict(set)
for (s,p,ps),n in partner_counts.items():partner_by_sp[(s,p)].add(ps)
for s in range(SETS):
    for p in poses:
        if p>=POSES:
            status='POSE_OUTSIDE_RUNTIME_PACK'
            rc=0; pc=0
        else:
            _,rc,_=runtime_pose(s,p);pc=max([partner_counts[(s,p,ps)] for ps in partner_by_sp.get((s,p),set())] or [0])
            if rc: status='UNPAIRED_EMITTABLE'
            elif pc: status='PARTNER_SET_REQUIRED'
            else: status='NO_COMPOSITION_FOR_SET_IN_SUPERSET'
        counts[status]+=1;reach.append((f'0x{s:02X}',f'0x{p:04X}',rc,pc,status))
with (OUT/'runtime-animation-pose-superset-matrix.csv').open('w',newline='') as f:
    w=csv.writer(f);w.writerow(['gfx_set','pose','runtime_unpaired_count','max_partner_count','classification']);w.writerows(reach)

# Cross-layer matrix.  It deliberately separates source-composition parity from
# dynamic reachability/global placement/hardware validation.  WP25 is a
# secondary embedded byte-level authority here; the accepted WP36 handoff pins
# the ownership/classification but the full WP36 inventory ZIP is not embedded.
cross=[]
for s in range(SETS):
    for p in range(POSES):
        exp=groups.get((s,p),[])
        _,rc,_=runtime_pose(s,p)
        variants=sorted(partner_by_sp.get((s,p),set()))
        maxpc=max([partner_counts[(s,p,ps)] for ps in variants] or [0])
        if exp and rc==len(exp):
            runtime='EXACT_UNPAIRED'
            final='STRUCTURAL_EXACT_PRE_CULL'
        elif not exp and rc==0 and variants:
            runtime='PARTNER_SELECTION_REQUIRED'
            final='OPEN_PARTNER_SELECTION'
        elif not exp and rc==0:
            runtime='NO_UNPAIRED_COMPOSITION'
            final='OPEN_REACHABILITY'
        else:
            runtime='MISMATCH'
            final='MISMATCH'
        cap=max(rc,maxpc)
        wp26='CAPACITY_OK' if cap<=512 else 'CAPACITY_EXCEEDED'
        cross.append((f'0x{s:02X}',f'0x{p:04X}',len(exp),len(variants),maxpc,rc,runtime,'WP35_CONTRACT_CONSUMED','WP26_DECODE_CONSUMED',wp26,final))
with (OUT/'ROM-RUNTIME-WP26-MATRIX.csv').open('w',newline='') as f:
    w=csv.writer(f);w.writerow(['gfx_set','pose','embedded_authority_unpaired_count','partner_variant_count','max_partner_count','runtime_unpaired_count','runtime_source_status','wp35_position_contract','wp26_coordinate_contract','wp26_capacity','classification']);w.writerows(cross)

discrepancies=[
 ('ACTIVE_RASTER_PIPELINE','CLOSED_IN_THIS_CHECKPOINT','Gameplay/WP35/WP26','Previous runtime treated +$14/+16 as final draw coordinates; active combat render now preserves $247C intermediate anchors and applies WP26 buffered-sprite decode.'),
 ('UNPAIRED_RUNTIME_COMPOSITIONS','EXACT','Runtime/WP36','All 3,632 in-range non-empty wrestler unpaired compositions are byte/field exact against the embedded WP25 expanded-piece authority; 3,148 remaining in-range slots are exactly empty.'),
 ('PARTNER_EXPANDED_SELECTION','OPEN','Gameplay/WP02/WP36','Runtime active actor emitter has no proved partner-set selector. Partner-required frames remain fail-closed instead of guessing.'),
 ('BIG_BOSSMAN_0178_PARTNER01','CAPACITY_PASS_SELECTION_OPEN','Gameplay/WP36/WP26','86-cell authority fixture survives composition emission and VDP1 planning with 86 commands, but active partner selection is not yet wired.'),
 ('FULL_WP36_INVENTORY_CROSSCHECK','OPEN','Chat master/WP36 transport','Accepted handoff is present, but the full WP36 inventory ZIP/CSV is not embedded in this checkpoint; no claim of exhaustive WP36-vs-runtime equality is made.'),
 ('ALL_REACHABLE_FRAME_MATRIX','OPEN','Gameplay/WP02/WP36','Generated animation pose set is an over-approximation; exact per-character runtime reachability is not proved by this checkpoint.'),
 ('F4C2_NORMAL_FRAME_OWNER','BLOCKED','Gameplay','$F4C2 remains the next scheduler blocker; exact $280DC and $2208 dependencies plus broader $1C03E coverage are not replaced with no-ops.'),
 ('HARDWARE_VALIDATION','REQUIRED','WP26/Golden QA/Saturn hardware','No physical Saturn validation is claimed for these source changes.')
]
with (OUT/'WP35-WP36-DISCREPANCIES.csv').open('w',newline='') as f:
    w=csv.writer(f);w.writerow(['item','status','owner','detail']);w.writerows(discrepancies)

summary={
 'authority_source':'current package authority/wp25-metasprite-expanded-pieces.csv; WP35+WP36 handoff pins accepted WP35/WP36 ownership but full WP36 inventory ZIP is not embedded',
 'unpaired_compositions_exact':exact,
 'empty_unpaired_slots_exact':empty,
 'unpaired_slot_total':SETS*POSES,
 'runtime_piece_pack_sha256':hashlib.sha256(pcs).hexdigest(),
 'runtime_index_sha256':hashlib.sha256(idx).hexdigest(),
 'wp36_pins':{
  'warrior_0062':23,'warrior_0063':23,'demolition_005b':20,'hawk_0068':23,
  'bossman_0178_partner01':86
 },
 'max_partner_expanded':{'gfx_set':3,'pose':'0x0178','partner_set':1,'pieces':86},
 'animation_pose_superset_unique_poses':len(poses),
 'animation_pose_superset_classifications':dict(counts),
 'classification':'PARTIAL: unpaired source composition is byte/field exact against embedded authority; partner selection/integration and exact all-reachable-frame WP36 crosscheck remain OPEN'
}
(OUT/'wp35-wp36-runtime-crosscheck-summary.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')
print('PASS WP35/WP36 runtime crosscheck')
print(json.dumps(summary,indent=2,sort_keys=True))
