#!/usr/bin/env python3
"""Make VDP2 cells from ROM bitplanes and an input-only arcade state.

Only the immutable corridor maps/palettes are retained. Fighter animation,
names, floor light and the ROM's shutter glyphs are rendered by native code.
No screenshot or per-frame video is an input texture.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import numpy as np
from arcade_graphics import Graphics, State, pack_vdp2_pattern

def state_at(path, frame):
    with Path(path).open('rb') as f:
        f.seek((frame-1200)*65600)
        raw=f.read(65600)
    assert len(raw)==65600 and struct.unpack_from('>I',raw)[0]==frame
    vals=struct.unpack_from('>14H',raw,20)
    return State(raw[64:4160],raw[4160:8256],raw[8256:16448],raw[16448:24640],
                 np.frombuffer(raw[24640:41024],'>u2'),vals[:4],vals[7],vals[5],frame)

def make(rom, capture, out):
    gfx=Graphics(rom); st=state_at(capture,1302)
    out.mkdir(parents=True,exist_ok=True)
    maps=[];counts=[]
    for kind,raw,base,pal in [(0,st.bg1,0,256),(1,st.bg0,0x20000,512)]:
        w=np.frombuffer(raw,'>u2')
        codes=w[:1024]&4095 if kind==0 else w.reshape(-1,2)[:,1]&4095
        variants=[np.frombuffer(state_at(capture,f).bg1,'>u2')[:1024] for f in (1302,1310,1355)] if kind==0 else [w]
        used=sorted(set(map(int,np.concatenate([v&4095 for v in variants])))) if kind==0 else sorted(set(map(int,codes)))
        look={v:i for i,v in enumerate(used)}
        tex=b''.join(pack_vdp2_pattern(gfx.tile('background',c)) for c in used)
        native=np.array([(base>>5)+look[int(c)]*4 for c in codes],np.uint32)
        if kind==0:bank=w[:1024]>>12
        else:
            attr=w.reshape(-1,2)[:,0].astype(np.uint32);bank=attr&15
            native|=((attr>>6)&1)<<30;native|=((attr>>7)&1)<<31
        native|=(bank.astype(np.uint32)+pal//16)<<16
        (out/f'BG{kind}.TEX').write_bytes(tex)
        (out/f'MAP{kind}.BIN').write_bytes(native.astype('>u4').tobytes())
        if kind==0:
            for variant,v in enumerate(variants):
                nm=np.array([(base>>5)+look[int(c&4095)]*4 for c in v],np.uint32)
                nm|=((v.astype(np.uint32)>>12)+pal//16)<<16
                (out/f'CROWD{variant}.BIN').write_bytes(nm.astype('>u4').tobytes())
        counts.append(len(used));maps.append(used)
    # Keeping all ROM font glyphs makes every original identity label usable.
    font=b''.join(pack_vdp2_pattern(gfx.tile('text',c)) for c in range(4096))
    (out/'FONT.TEX').write_bytes(font)
    (out/'REFERENCE_TEXT.BIN').write_bytes(st.text)
    pal=np.concatenate((st.palette[:256],st.palette[0xc00:0xd00],st.palette[0x1000:0x1100]))
    rgb=(((pal.astype(np.uint32)>>8)&15)*17<<16)|(((pal.astype(np.uint32)>>4)&15)*17<<8)|((pal.astype(np.uint32)&15)*17)
    (out/'ENVPAL.C32').write_bytes(rgb.astype('>u4').tobytes())
    (out/'ENVPAL.C44').write_bytes(pal.astype('>u2').tobytes())
    record={'source_frame':1302,'source':'original ROM bitplanes and original arcade device state',
            'scroll':list(st.scroll),'priority':st.priority,'background_tile_codes':maps,
            'tile_counts':counts,'font_tiles':4096,'native_width':320,'native_height':240,
            'assets':{p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in out.iterdir() if p.is_file()}}
    (out/'environment.json').write_text(json.dumps(record,indent=2)+'\n')
    sizes={n:(out/n).stat().st_size for n in ['BG0.TEX','BG1.TEX','FONT.TEX']}
    (out.parent/'environment_assets.h').write_text('#ifndef WP01_ENVIRONMENT_ASSETS_H\n#define WP01_ENVIRONMENT_ASSETS_H\n'+''.join(f'#define WP01_{n.split(".")[0]}_BYTES {v}U\n' for n,v in sizes.items())+'#endif\n')
    print(json.dumps({'sizes':sizes,'tiles':counts,'scroll':record['scroll'],'priority':st.priority}))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('capture',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    make(a.rom,a.capture,a.output)
