#!/usr/bin/env python3
import csv, hashlib, struct, sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1])

# Manifest integrity for every distributed golden and preview artifact.
for man,subdir in [('golden-manifest.csv','golden'),('preview-manifest.csv','preview')]:
    with (root/'manifests'/man).open(newline='') as f:
        rows=list(csv.DictReader(f))
    assert rows, man
    listed=set()
    for r in rows:
        p=root/subdir/r['file']; listed.add(r['file'])
        assert p.is_file(),p
        assert hashlib.sha256(p.read_bytes()).hexdigest()==r['sha256'],p
        assert p.stat().st_size==int(r['bytes']),p
    actual={p.name for p in (root/subdir).iterdir() if p.is_file() and p.name!='README.md'}
    assert actual==listed,(man,sorted(actual-listed),sorted(listed-actual))

# All static PNG golden refs are exact 320x240 RGBA/RGB images.
for p in (root/'golden').glob('*.png'):
    b=p.read_bytes(); assert b[:8]==b'\x89PNG\r\n\x1a\n'
    w,h=struct.unpack('>II',b[16:24]); assert (w,h)==(320,240),(p,w,h)

# Primary animated previews are lossless APNG, not GIF/color-quantized authority.
assert not list((root/'preview').glob('*.gif')), 'GIF must not be distributed as pixel-perfect authority'
for p in sorted((root/'preview').glob('*.apng')):
    im=Image.open(p)
    assert im.format=='PNG' and getattr(im,'is_animated',False),p
    assert im.size==(320,240),p
    assert im.n_frames>1,p

# Exact timing/event trace: 402 represented quanta per presentation.
for p in sorted((root/'preview').glob('*-TRACE.csv')):
    rows=list(csv.DictReader(p.open()))
    segments=sorted({int(r['segment']) for r in rows})
    assert segments==list(range(len(segments))),p
    total=sum(int(r['represented_rom_quanta']) for r in rows)
    assert total==402*len(segments),(p,total,len(segments))
    for seg in segments:
        rr=[r for r in rows if int(r['segment'])==seg]
        assert len(rr)==338,(p,seg,len(rr))
        reveal=[r for r in rr if r['phase']=='reveal']; walk=[r for r in rr if r['phase']=='walk']
        close=[r for r in rr if r['phase']=='close']; hold=[r for r in rr if r['phase']=='close_hold']
        post=[r for r in rr if r['phase']=='post_close_outer_sync']
        assert len(reveal)==8 and all(int(r['represented_rom_quanta'])==3 for r in reveal)
        assert [int(r['mask_solid_rows']) for r in reveal]==list(range(8,0,-1))
        assert len(walk)==320
        assert [int(r['actor_update_event']) for r in walk]==list(range(1,321))
        assert all(int(r['represented_rom_quanta'])==1 for r in walk)
        assert int(walk[0]['actor_y_fixed_16_16'],16)==0x012F8000
        assert int(walk[-1]['actor_y_fixed_16_16'],16)==0x00900000
        assert len(close)==8 and [int(r['mask_solid_rows']) for r in close]==list(range(1,9))
        assert close[0]['actor_update_event']=='321' and all(not r['actor_update_event'] for r in close[1:])
        assert int(close[0]['actor_y_fixed_16_16'],16)==0x008F8000
        assert all(int(r['represented_rom_quanta'])==3 for r in close)
        assert len(hold)==1 and int(hold[0]['represented_rom_quanta'])==33
        assert len(post)==1 and int(post[0]['represented_rom_quanta'])==1

    # APNG playback duration must preserve the trace's total represented quanta;
    # adjacent identical pixel states may be coalesced by the PNG writer.
    apng=p.with_name(p.name.replace('-TRACE.csv','.apng'))
    im=Image.open(apng); ms=0.0
    for i in range(im.n_frames):
        im.seek(i); ms += float(im.info.get('duration',0))
    assert abs(ms-total*15.0)<0.01,(apng,ms,total*15)

# Pair-normalization and persistent follower phase are visible in the trace itself.
tag=root/'preview/WP12-R03-TAG-FIRST-MATCH-ROM-EXACT-TRACE.csv'
tr=list(csv.DictReader(tag.open()))
assert 'normalized ids 1/0' in tr[0]['label']
seg0=[r for r in tr if r['segment']=='0']; seg1=[r for r in tr if r['segment']=='1']
assert seg0[-1]['phase']=='post_close_outer_sync' and seg0[-1]['follower_counter_after']=='1' and seg0[-1]['follower_phase_after']=='0'
first_walk1=next(r for r in seg1 if r['phase']=='walk')
assert first_walk1['follower_counter_after']=='2' and first_walk1['follower_phase_after']=='0'

print('WP12 R03 golden/APNG/trace artifact audit: PASS')
