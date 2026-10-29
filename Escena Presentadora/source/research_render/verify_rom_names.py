#!/usr/bin/env python3
"""Execute original ROM 98BA and compare its text output with native assets."""
from pathlib import Path
import json
import sys
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parent/'research_rom'))
from rom_oracle import Oracle,load_rom
sys.path.insert(0,str(ROOT/'workpack_25/tools'))
from wf25_graphics import Rom
rompath=ROOT.parents[1]/'project_sources/01-wwfwfest.zip'
rom=Rom(rompath);program=load_rom(rompath)
m=json.loads((ROOT/'generated/assets.json').read_text())
atlas=(ROOT/'generated/TEX.BIN').read_bytes()
pal={b['base']:np.array([[(v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17]for v in b['rgb444']],dtype=np.uint8)for b in m['palette_banks']}
names={(x['identity'],x['slot']):m['frames'][x['frame']]for x in m['name_frames']}
cases=[(0,6,9,1),(7,9,10,11),(10,11,7,9),(2,3,0,8),(1,2,6,7)]
result=[]
for ids in cases:
    o=Oracle(program,ids)
    o.u.mem_write(0x180000,(ROOT.parent/'verification/mame/snaps/pal3300.bin').read_bytes())
    o.u.emu_start(0x98ba,0x1ff000,count=1000000)
    text=bytes(o.u.mem_read(0xc0000,8192))
    actual=np.zeros((240,320,3),dtype=np.uint8)
    for c in range(2048):
        # 99D0 writes the high byte; 9ACA writes the low byte. Real 8-bit
        # text RAM aliases either byte to the same low-byte read value.
        a=text[c*4]|text[c*4+1];b=text[c*4+2]|text[c*4+3]
        code=a|((b&15)<<8);bank=b>>4
        if not code:continue
        x=(c%64)*8;y=(c//64)*8-8
        if x>=320 or y<0 or y>=240:continue
        tile=np.frombuffer(rom.text_tile(code),dtype=np.uint8).reshape(8,8)
        vals=[int.from_bytes(o.u.mem_read(0x180000+bank*0x80+i*2,2),'big')for i in range(16)]
        palette=np.array([[(v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17]for v in vals],dtype=np.uint8)
        cut=actual[y:y+8,x:x+8];cut[tile!=0]=palette[tile[tile!=0]]
    native=np.zeros_like(actual)
    frames=[names[i,s]for s,i in enumerate(ids)if not(s%2 and i==11 and ids[s-1]==10)]
    frames.append(m['frames'][m['vs_frame']])
    for f in frames:
        for q in m['pieces'][f['first']:f['first']+f['count']]:
            x,y=q['x'],q['y'];w,h=q['width'],q['height']
            packed=np.frombuffer(atlas,dtype=np.uint8,count=w*h//2,offset=q['tile']*128).reshape(h,w//2)
            tile=np.empty((h,w),dtype=np.uint8);tile[:,::2]=packed>>4;tile[:,1::2]=packed&15
            x0,y0=max(x,0),max(y,0);x1,y1=min(x+w,320),min(y+h,240)
            if x0>=x1 or y0>=y1:continue
            px=tile[y0-y:y1-y,x0-x:x1-x];sub=native[y0:y1,x0:x1]
            sub[px!=0]=pal[q['palette']][px[px!=0]]
    diff=int(np.any(actual!=native,axis=2).sum())
    result.append({'raw_identities':ids,'different_pixels':diff})
    if 10 in ids:
        Image.fromarray(actual).save(ROOT/'generated'/('rom_names_'+''.join(f'{i:02}'for i in ids)+'.png'))
    assert diff==0,(ids,diff)
out={'status':'PASS','method':'Original 68000 helper98BA in Unicorn, byte-smear-normalized exact text RAM and original palette writes; native generated atlas, deduplicated Demolition wordmark','cases':result}
(ROOT/'generated/rom_name_audit.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
