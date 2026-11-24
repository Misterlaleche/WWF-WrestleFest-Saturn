#!/usr/bin/env python3
"""Derive compact ring and streaming pose assets from verified user ROM.

No complete program or graphic ROM is included in resident runtime data.
World coordinates follow $26CA8: page X=(metatileX/10), Y=(64-metatileY)/8.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from wf_r18_rom_graphics import Rom, be16, rgb444_to_saturn, pack4

def build(rompath, out, mode=0, palette_mode=0):
    r=Rom(rompath); out.mkdir(parents=True,exist_ok=True)
    # Same bounded 10-byte pieces consumed by wf_graphics_emit_composition.
    data=bytearray();index=bytearray(b'R18P'+struct.pack('>HH',1,82)+bytes(82*8));maxpieces=0
    def pieces(s,p,partner):
        nonlocal maxpieces
        pp=r.expanded_pieces(s,p,partner);off=len(data);maxpieces=max(maxpieces,len(pp))
        for q in pp:
            data.extend(struct.pack('>HBBhhBB',q['tile_code'],q['palette_resource'],q['flip_x']|(q['flip_y']<<1),q['rel_x'],q['rel_y'],q['chain_index'],q['chain']))
        return struct.pack('>IHH',off,len(pp),1)
    for s,_,_,count in r.active_sets():
        off=len(index);struct.pack_into('>IHH',index,8+s*8,off,count,0)
        for p in range(count):
            entry=r.pose_entry(s,p)
            if entry in (None,0xfffe,0xffff):index.extend(bytes(8));continue
            kind=be16(r.p,entry)>>14
            if kind==2:
                variants=b''.join(pieces(s,p,v) for v in range(12));off=len(data);data.extend(variants)
                index.extend(struct.pack('>IHH',off,0,12))
            else:index.extend(pieces(s,p,0))
    (out/'R18POSE.IDX').write_bytes(index);(out/'R18POSE.BIN').write_bytes(data)
    # Flatten pages into world row-major cells: attr then dense tile index.
    layers=[];codes=set()
    for layer in ('fg','bg'):
        cells=[None]*(160*128)
        for page,ptr in enumerate(r.world_page_ptrs(layer,mode)):
            for x,y,code,pal,fx,fy,_,_ in r.world_page_entries(layer,ptr):
                assert code<4096
                cells[(page//8*16+y)*160+page%8*20+x]=(code,pal|(fx<<6)|(fy<<7));codes.add(code)
        layers.append(cells)
    # Animated 4x4 patches copied by $2946E/$2951E also require pixels.
    animids=[set(),set()]
    animroot=int.from_bytes(r.p[0x28c66+mode*4:0x28c6a+mode*4],'big')
    for obj in range((32,32,44,0,12,36,44)[mode]):
        desc=int.from_bytes(r.p[animroot+obj*4:animroot+obj*4+4],'big')
        layer=bool(r.p[desc+1]&128)
        for frame in range(r.p[desc]+1):animids[layer].add(r.p[desc+2+frame*2])
    for layer in range(2):
        root=(0x295a4 if layer==0 else 0x295dc)+mode*8
        source=int.from_bytes(r.p[root+4:root+8],'big')
        for frame in animids[layer]:
            for cell in range(16):
                off=source+frame*(64 if layer==0 else 32)+cell*(4 if layer==0 else 2)
                code=be16(r.p,off+2)&8191 if layer==0 else be16(r.p,off)&4095
                assert code<4096;codes.add(code)
    codes=sorted(codes);mapping={code:i for i,code in enumerate(codes)}
    body=bytearray(128*4); pages={}
    for li,cells in enumerate(layers):
        for page in range(64):
            payload=b''.join(struct.pack('>HH',cells[(page//8*16+y)*160+page%8*20+x][1],mapping[cells[(page//8*16+y)*160+page%8*20+x][0]]) for y in range(16) for x in range(20))
            if payload not in pages:
                pages[payload]=32+len(body);body.extend(payload)
            struct.pack_into('>I',body,(li*64+page)*4,pages[payload])
    tileoff=32+len(body)
    for code in codes:body.extend(pack4(r.bg_tile(code),16,16))
    for code in codes:body.extend(struct.pack('>H',code))
    paloff=32+len(body)
    for layer in ('fg','bg'):
        for c in r.bg_palette(layer,palette_mode):body.extend(struct.pack('>H',c))
    header=struct.pack('>4sHHHHIIIII',b'R18R',2,mode,palette_mode,len(codes),32,tileoff,paloff,160,128)
    (out/'R18RING.BIN').write_bytes(header+body)
    manifest={'world_mode':mode,'palette_mode':palette_mode,'world_tiles':[160,128],'source_world_pixels':[2560,2048], 'ring_tiles':len(codes),'max_pose_pieces':maxpieces,'pose_piece_bytes':10,'pose_index_set_count':82,'scroll_y_source':'2048 - shadow_y + screen_y','assets':{}}
    for name in ('R18POSE.IDX','R18POSE.BIN','R18RING.BIN'):
        b=(out/name).read_bytes();manifest['assets'][name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
    (out/'R18GRAPHICS.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps(manifest,indent=2))
if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--rom',required=True,type=Path);a.add_argument('--out',required=True,type=Path);a.add_argument('--mode',type=int,default=0);a.add_argument('--palette-mode',type=int,default=0);v=a.parse_args();build(v.rom,v.out,v.mode,v.palette_mode)
