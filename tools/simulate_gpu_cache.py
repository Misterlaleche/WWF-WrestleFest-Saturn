#!/usr/bin/env python3
"""Model a fenced immutable VDP1 texture cache from actual native tile requests.

The trace is emitted only by the host fixture. No frame, tile or decision from
this offline audit is used by the console executable. Simulation counts are
not Saturn speed measurements.
"""
import argparse,collections,json,struct
from pathlib import Path

def load(path):
    data=path.read_bytes();at=0;frames=[]
    while at<len(data):
        frame,count=struct.unpack_from('>IH',data,at);at+=6
        ids=struct.unpack_from('>'+str(count)+'H',data,at);at+=count*2
        assert len(set(ids))==count<=256
        frames.append((frame,ids))
    assert at==len(data)
    return frames

def simulate(frames,capacity,block_slots,lag,policy,block_shift=6):
    source=[None]*capacity;last=[-lag-1]*capacity;lookup={};hand=0
    blocks=collections.OrderedDict();pixels=decodes=blocked=0
    max_upload=max_decode=peak_protected=0
    for sequence,(frame,ids) in enumerate(frames):
        # Mark every resident reference before any replacement, including
        # references appearing late in arcade draw order.
        for tile in ids:
            if tile in lookup:last[lookup[tile]]=sequence
        misses=decoded=0;shown=sequence-lag
        for tile in ids:
            if tile in lookup:continue
            if policy=='lru':
                slot=min(range(capacity),key=lambda s:last[s])
                if last[slot]>=shown:
                    return {'capacity':capacity,'policy':policy,'blocked_at_frame':frame,
                        'reason':'all slots belong to a displayed/pending/building frame'}
            else:
                slot=hand
                while last[slot]>=shown:
                    slot=(slot+1)%capacity
                    if slot==hand:
                        return {'capacity':capacity,'policy':policy,'blocked_at_frame':frame,
                            'reason':'all slots belong to a displayed/pending/building frame'}
                hand=(slot+1)%capacity
            if source[slot] is not None:del lookup[source[slot]]
            source[slot]=tile;lookup[tile]=slot;last[slot]=sequence;misses+=1
            block=tile>>block_shift
            if block not in blocks:
                decoded+=1
                if len(blocks)==block_slots:blocks.popitem(last=False)
            blocks[block]=None;blocks.move_to_end(block)
        pixels+=misses;decodes+=decoded;max_upload=max(max_upload,misses)
        max_decode=max(max_decode,decoded)
        peak_protected=max(peak_protected,sum(tag>=shown for tag in last))
    return {'capacity':capacity,'policy':policy,'frames':len(frames),'texture_uploads':pixels,
        'texture_bytes':pixels*128,'block_decodes':decodes,'max_uploads_in_frame':max_upload,
        'max_decodes_in_frame':max_decode,'peak_protected_slots':peak_protected,
        'blocked_at_frame':None}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory',type=Path)
    p.add_argument('--block-slots',type=int,default=71)
    p.add_argument('--pending-lag',type=int,default=3)
    p.add_argument('--block-shift',type=int,choices=(5,6),default=6)
    a=p.parse_args();result={'description':'Offline native-request model, not measured Saturn FPS',
        'block_slots':a.block_slots,'block_shift':a.block_shift,'pending_lag':a.pending_lag,'modes':{}}
    for mode in ('tag','royal'):
        frames=load(a.directory/(mode+'-tiles.bin'))
        row={'distinct_tiles':len({tile for _,ids in frames for tile in ids}),
            'max_visible_tiles':max(map(lambda f:len(f[1]),frames)), 'models':[]}
        for policy in ('clock','lru'):
            for capacity in (256,512,768,1024,1536,2048,3072):
                v=simulate(frames,capacity,a.block_slots,a.pending_lag,policy,a.block_shift)
                row['models'].append(v);print(mode,v)
        result['modes'][mode]=row
    target=a.directory/('simulation-'+str(a.block_slots)+'.json')
    target.write_text(json.dumps(result,indent=2)+'\n');print(target)

if __name__=='__main__':main()
