#!/usr/bin/env python3
"""Export native Saturn scene assets from the supplied ROM and MAME RAM oracle.

Every sprite source cell is validated against WP25's independent tile manifests.
The background and foreground are the actual paid TAG map snapshot; no historical
WIP camera, palette reduction, guessed art, or entrance interpolation is used.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "workpack_25/tools"))
from wf25_graphics import Rom, be16, be32, pack4

ROM_PATH = ROOT.parents[1] / "project_sources/01-wwfwfest.zip"
SNAPS = ROOT.parent / "verification/mame/snaps"
FULL = ROOT.parent / "verification/mame/full/frames.bin"
OUT = ROOT / "generated"
RECORD_SIZE = 40972


def rgb888(v):
    # Saturn mode2 longword / Yaul rgb888_t stores big-endian [CC,B,G,R].
    return (((v >> 8) & 15) * 17 << 16) | (((v >> 4) & 15) * 17 << 8) | ((v & 15) * 17)


def snapshot(index):
    with FULL.open('rb') as f:
        f.seek(index * RECORD_SIZE)
        raw = f.read(RECORD_SIZE)
    assert len(raw) == RECORD_SIZE
    pos = 4
    result = {'frame': be32(raw, 0)}
    for name, size in [('ram',16384),('spr',8192),('text',8192),('fg',4096),('bg',4096),('scroll',8)]:
        result[name] = raw[pos:pos+size]
        pos += size
    return result


class Exporter:
    def __init__(self):
        self.rom = Rom(ROM_PATH)
        self.tiles = []
        self.tile_bytes = []
        self.tile_lookup = {}
        self.wordmark_lookup = {}
        self.banks = []
        self.bank_lookup = {}
        self.poses = []
        self.frames = []
        self.pieces = []
        self.name_frames = []
        self.palette_ram = (SNAPS / 'pal3300.bin').read_bytes()

    def palette(self, values, label):
        values = tuple(int(v) & 4095 for v in values)
        assert len(values) == 16
        if values not in self.bank_lookup:
            assert len(self.banks) < 48, 'Sprite banks overlap the ring bitmap palette'
            base = len(self.banks) * 16
            self.bank_lookup[values] = base
            self.banks.append({'base':base,'rgb444':values,'label':label})
        return self.bank_lookup[values]

    def hardware_palette(self, index):
        # A5/A6 absent: CPU address inverse of palette's connected address bits.
        out = []
        for i in range(index, index+16):
            cpu_byte = ((i & ~15) << 3) | ((i & 15) << 1)
            out.append(be16(self.palette_ram, cpu_byte))
        return out

    def tile(self, kind, code, pixels=None):
        key = (kind,code)
        if key not in self.tile_lookup:
            if pixels is None:
                if kind == 'sprite': pixels = self.rom.sprite_tile(code)
                elif kind == 'background': pixels = self.rom.bg_tile(code & 4095)
                elif kind == 'text':
                    a = np.zeros((16,16),dtype=np.uint8)
                    a[:8,:8] = np.frombuffer(self.rom.text_tile(code),dtype=np.uint8).reshape(8,8)
                    pixels = bytes(a.ravel())
                else: raise ValueError(kind)
            packed = pack4(pixels,16,16)
            idx = len(self.tile_bytes)
            self.tile_lookup[key] = idx
            self.tiles.append({'atlas':idx,'kind':kind,'source':code,'sha256':hashlib.sha256(packed).hexdigest()})
            self.tile_bytes.append(packed)
        return self.tile_lookup[key]

    def add_frame(self, label, pieces):
        frame = {'label':label,'first':len(self.pieces),'count':len(pieces)}
        self.frames.append(frame)
        self.pieces.extend(pieces)
        return len(self.frames)-1

    def piece(self, kind, code, x, y, palette, flip=0, flags=0):
        return {'tile':self.tile(kind,code),'x':x,'y':y,'palette':palette,'flip':flip,'flags':flags,'width':16,'height':16,'source':code}

    def pixels(self, piece):
        width,height=piece['width'],piece['height']
        size=width*height//2
        start=piece['tile']
        data=b''.join(self.tile_bytes[start:start+(size+127)//128])[:size]
        packed=np.frombuffer(data,dtype=np.uint8).reshape(height,width//2)
        out=np.empty((height,width),dtype=np.uint8)
        out[:,::2]=packed>>4;out[:,1::2]=packed&15
        if piece['flip']&1:out=out[:,::-1]
        if piece['flip']&2:out=out[::-1,:]
        return out

    def flatten_wordmark(self, frame_index):
        """Precompose unchanged original text indices into one native texture.

        These text frames are static, single-palette and transparent. Cropping
        screen-invisible/zero pixels is therefore lossless, including the
        original ROM name placement and spaces between glyphs.
        """
        frame=self.frames[frame_index]
        original=self.pieces[frame['first']:frame['first']+frame['count']]
        assert original and len({p['palette']for p in original})==1,frame['label']
        canvas=np.zeros((240,320),dtype=np.uint8)
        blank=offscreen=0
        for p in original:
            assert p['flags']==0
            px=self.pixels(p)
            if not np.any(px):blank+=1;continue
            x,y=p['x'],p['y'];w,h=p['width'],p['height']
            x0,y0=max(0,x),max(0,y);x1,y1=min(320,x+w),min(240,y+h)
            if x0>=x1 or y0>=y1:offscreen+=1;continue
            cut=px[y0-y:y1-y,x0-x:x1-x]
            canvas[y0:y1,x0:x1][cut!=0]=cut[cut!=0]
        ys,xs=np.nonzero(canvas)
        assert len(xs)>0,frame['label']
        x,y=int(xs.min()),int(ys.min())
        w=((int(xs.max())-x+1)+7)&~7
        h=int(ys.max())-y+1
        assert 0<w<=504 and 0<h<=255
        cropped=np.zeros((h,w),dtype=np.uint8)
        end=min(x+w,320)
        cropped[:,:end-x]=canvas[y:y+h,x:end]
        packed=pack4(cropped.tobytes(),w,h)
        digest=hashlib.sha256(packed).hexdigest()
        key=(w,h,digest)
        if key not in self.wordmark_lookup:
            tile=len(self.tile_bytes)
            self.wordmark_lookup[key]=tile
            self.tiles.append({'atlas':tile,'kind':'wordmark','source':frame['label'],
                               'width':w,'height':h,'bytes':len(packed),'sha256':digest})
            padded=packed+b'\0'*((-len(packed))%128)
            self.tile_bytes.extend(padded[i:i+128]for i in range(0,len(padded),128))
        piece={'tile':self.wordmark_lookup[key],'x':x,'y':y,'palette':original[0]['palette'],
               'flip':0,'flags':0,'width':w,'height':h,'source':frame['label']}
        frame.update({'first':len(self.pieces),'count':1,'unflattened_count':len(original),
                      'blank_cells':blank,'offscreen_cells':offscreen})
        self.pieces.append(piece)

    def actors(self):
        for s in list(range(13))+[31]:
            selected = [0,12,89,90,91,92,93] if s < 12 else range(32)
            for pose in selected:
                entry = self.rom.pose_entry(s,pose)
                if entry is None or entry in (0xfffe,0xffff): continue
                pieces = []
                for q in self.rom.expanded_pieces(s,pose):
                    pal = self.palette(self.rom.resource_palette(q['palette_resource'],True),f'resource_{q["palette_resource"]:02x}')
                    pieces.append(self.piece('sprite',q['tile_code'],q['rel_x'],q['rel_y'],pal,q['flip_x']|q['flip_y']<<1))
                frame = self.add_frame(f'pose_{s:02x}_{pose:04x}', pieces)
                self.poses.append({'set':s,'pose':pose,'frame':frame,'pieces':pieces})

    def backgrounds(self, snap):
        sx,sy,_,_ = struct.unpack('>4H',snap['scroll'])
        # Source mode7C: FG_WORLD opaque, actors, BG_WORLD transparent.
        back = np.zeros((240,320),dtype=np.uint16)
        overlay = []
        for layer in ['fg','bg']:
            data = snap[layer]
            for vy in range(-((sy+8)&15),240,16):
                for vx in range(-(sx&15),320,16):
                    tx=((vx+sx)&511)//16
                    ty=((vy+sy+8)&511)//16
                    if layer == 'fg':
                        attr,code = struct.unpack_from('>HH',data,(ty*32+tx)*4)
                        bank=attr&15;flip=(attr>>6)&3;palette_index=0x1000+bank*16
                    else:
                        attr=be16(data,(ty*32+tx)*2);code=attr&4095
                        bank=(attr>>12)&15;flip=0;palette_index=0xc00+bank*16
                    tile=np.frombuffer(self.rom.bg_tile(code&4095),dtype=np.uint8).reshape(16,16)
                    if flip&1: tile=tile[:,::-1]
                    if flip&2: tile=tile[::-1,:]
                    pal=self.hardware_palette(palette_index)
                    if layer == 'fg':
                        xx0,yy0=max(vx,0),max(vy,0);xx1,yy1=min(vx+16,320),min(vy+16,240)
                        back[yy0:yy1,xx0:xx1]=np.array(pal,dtype=np.uint16)[tile[yy0-vy:yy1-vy,xx0-vx:xx1-vx]]
                    elif np.any(tile):
                        base=self.palette(pal,f'bg_world_bank_{bank}')
                        overlay.append(self.piece('background',code&4095,vx,vy,base,flip))
        colors,index=np.unique(back,return_inverse=True)
        assert len(colors)<=256
        ring=np.zeros((256,512),dtype=np.uint8)
        ring[:240,:320]=index.reshape(240,320).astype(np.uint8)
        self.ring_colors=[int(v) for v in colors]
        self.ring=ring
        self.ring_rgb444=back
        self.foreground_frame=self.add_frame('foreground',overlay)

    def text(self, data, label):
        pieces=[]
        for cell in range(64*32):
            a,b=struct.unpack_from('>HH',data,cell*4)
            code=(a&255)|((b&15)<<8)
            if code==0:continue
            x=(cell%64)*8;y=(cell//64)*8-8
            if x>=320 or y>=240 or y+8<=0:continue
            bank=(b>>4)&15
            palette=self.palette(self.hardware_palette(bank*16),f'captured_text_bank_{bank}')
            pieces.append(self.piece('text',code,x,y,palette))
        return self.add_frame(label,pieces)

    def name_cells(self, identity, slot):
        """ROM98BA/993A/9ACA/99D0: raw numeric name geometry, no scaling."""
        p=self.rom.p
        out=[]
        if identity >= 10:
            # Large Demolition wordmark spans both columns of its team row.
            start=0x504 if slot<2 else 0x1504
            for row in range(8):
                for col in range(38):
                    code=0x900|p[0x9d36+row*38+col]
                    if code:out.append((start+row*0x100+col*4,code))
            return out
        start=be16(p,0x9980+identity*8+slot*2)
        a=be32(p,0x9b26+identity*4)
        code=be16(p,a);count=be16(p,a+2)+1
        pos=4
        while True:
            for j in range(count):out.append((start+j*4,code+j))
            code+=count
            delta=be16(p,a+pos)
            if delta==0:break
            count=be16(p,a+pos+2)+1
            start+=delta;pos+=4
        return out

    def names(self):
        p=self.rom.p
        for identity in range(12):
            palette_ptr=be32(p,0x9e66+identity*4)
            values=[be16(p,palette_ptr+i*2)for i in range(16)]
            palette=self.palette(values,f'name_identity_{identity}')
            for slot in range(4):
                pcs=[]
                for off,code in self.name_cells(identity,slot):
                    x=((off//4)%64)*8;y=((off//4)//64)*8-8
                    if x<320 and -8<y<240 and any(self.rom.text_tile(code)):
                        pcs.append(self.piece('text',code,x,y,palette))
                frame=self.add_frame(f'name_{identity:02x}_{slot}',pcs)
                self.name_frames.append({'identity':identity,'slot':slot,'frame':frame})

    def sprites(self, snap, label):
        pcs=[]
        for i in range(0,len(snap['spr']),16):
            w=struct.unpack_from('>8H',snap['spr'],i);f=w[1]&255
            if not f&1:continue
            x=(w[5]&255)|((f&4)<<6)
            if x>496:x-=512
            y=((256-((w[0]&255)|((f&2)<<7)))&511)-24
            chain=(f>>5)&7;fx=bool(f&16);fy=bool(f&8)
            code=(w[2]&255)|((w[3]&255)<<8)
            pal=self.palette(self.hardware_palette(0x400+(w[4]&15)*16),f'snapshot_sprite_slot_{w[4]&15}')
            for c in range(chain+1):
                yy=y-chain*16+c*16 if fy else y-c*16
                if x<320 and x+16>0 and yy<240 and yy+16>0:
                    pcs.append(self.piece('sprite',code+c,x,yy,pal,int(fx)|int(fy)<<1))
        return self.add_frame(label,pcs)

    def render(self, frame_ids):
        rgb=np.zeros((240,320,3),dtype=np.uint8)
        rgb[:,:,0]=(self.ring_rgb444&15)*17
        rgb[:,:,1]=((self.ring_rgb444>>4)&15)*17
        rgb[:,:,2]=((self.ring_rgb444>>8)&15)*17
        for fi in frame_ids:
            fr=self.frames[fi]
            for q in self.pieces[fr['first']:fr['first']+fr['count']]:
                px=self.pixels(q)
                x,y=q['x'],q['y'];x0,y0=max(0,x),max(0,y);x1,y1=min(320,x+q['width']),min(240,y+q['height'])
                if x0>=x1 or y0>=y1:continue
                cut=px[y0-y:y1-y,x0-x:x1-x]
                values=self.banks[q['palette']//16]['rgb444']
                pal=np.array([[(v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17]for v in values],dtype=np.uint8)
                use=np.ones(cut.shape,dtype=bool)if q['flags']&1 else cut!=0
                rgb[y0:y1,x0:x1][use]=pal[cut[use]]
        return Image.fromarray(rgb)

    def emit(self):
        OUT.mkdir(exist_ok=True)
        # Discard former glyph command lists after their exact composition.
        compact=[]
        for frame in self.frames:
            referenced=self.pieces[frame['first']:frame['first']+frame['count']]
            frame['first']=len(compact)
            compact.extend(referenced)
        self.pieces=compact
        atlas=b''.join(self.tile_bytes)
        assert len(atlas)+1024*32+32<=524288
        (OUT/'TEX.BIN').write_bytes(atlas)
        (OUT/'RING.BIN').write_bytes(self.ring.tobytes())
        cram=[0]*1024
        for bank in self.banks:
            for i,c in enumerate(bank['rgb444']):cram[bank['base']+i]=rgb888(c)
        for i,c in enumerate(self.ring_colors):cram[768+i]=rgb888(c)
        (OUT/'CRAM.BIN').write_bytes(struct.pack('>1024I',*cram))
        hdr='''#ifndef WP02_ASSETS_H\n#define WP02_ASSETS_H\n#include <stdint.h>\ntypedef struct { uint16_t tile; int16_t x,y; uint16_t palette; uint8_t flip,flags; uint16_t width,height; } wp02_piece_t;\ntypedef struct { uint16_t first,count; } wp02_frame_t;\nextern const wp02_piece_t wp02_pieces[];\nextern const wp02_frame_t wp02_frames[];\nconst wp02_frame_t *wp02_pose_find(unsigned raw_gfx_set,unsigned raw_selector);\nconst wp02_frame_t *wp02_name_find(unsigned raw_identity,unsigned logical_slot);\n/* Pose X mirror = -piece.x, flip XOR1. Pose Y = raster anchor +piece.y. */\n/* Textures4bpp, row-major native width/height; address=atlas base+tile*128. */\n/* All actors16x16; exact static text composed into variable-size textures. */\n/* Width is a multiple of8; height1..255. No scaling. */\n/* palette is the absolute CRAM entry base; flags bit0 means opaque. */\n'''
        hdr+=f'#define WP02_TEXTURE_BYTES {len(atlas)}U\n#define WP02_RING_BYTES 131072U\n#define WP02_RING_PALETTE_BASE 768U\n#define WP02_FOREGROUND_FRAME {self.foreground_frame}U\n#define WP02_CREDIT_FRAME {self.credit_frame}U\n#define WP02_CAPTURE_NAMES_FRAME {self.capture_names_frame}U\n#define WP02_VS_FRAME {self.vs_frame}U\n#define WP02_PIECE_COUNT {len(self.pieces)}U\n#define WP02_FRAME_COUNT {len(self.frames)}U\n#endif\n'
        (OUT/'wp02_assets.h').write_text(hdr)
        c=['#include "wp02_assets.h"','const wp02_piece_t wp02_pieces[]={']
        c += [' {%d,%d,%d,%d,%d,%d,%d,%d},'%(q['tile'],q['x'],q['y'],q['palette'],q['flip'],q['flags'],q['width'],q['height'])for q in self.pieces]
        c += ['};','const wp02_frame_t wp02_frames[]={']
        c += [' {%d,%d}, /* %s */'%(f['first'],f['count'],f['label'])for f in self.frames]
        c += ['};','typedef struct { uint16_t set,pose,frame; } pose_ix_t;','static const pose_ix_t pose_index[]={']
        c += [' {%d,%d,%d},'%(p['set'],p['pose'],p['frame'])for p in self.poses]
        c += ['};','static const uint16_t name_index[12][4]={']
        c += [' {'+','.join(str(q['frame'])for q in self.name_frames if q['identity']==i)+'},'for i in range(12)]
        c += ['};','const wp02_frame_t *wp02_pose_find(unsigned s,unsigned p){p&=0x7fff;for(unsigned i=0;i<sizeof(pose_index)/sizeof(pose_index[0]);i++)if(pose_index[i].set==s&&pose_index[i].pose==p)return &wp02_frames[pose_index[i].frame];return 0;}','const wp02_frame_t *wp02_name_find(unsigned i,unsigned s){return i<12&&s<4?&wp02_frames[name_index[i][s]]:0;}']
        (OUT/'wp02_assets.c').write_text('\n'.join(c)+'\n')
        manifest={'rom_sha1':hashlib.sha1(self.rom.p).hexdigest(),'source':'Actual paid TAG MAME0.264 frame3249 maps; verified supplied ROM pixel decode','tiles':self.tiles,'palette_banks':self.banks,'poses':self.poses,'frames':self.frames,'pieces':self.pieces,'name_frames':self.name_frames,'ring_palette':self.ring_colors,'texture_bytes':len(atlas),'ring_palette_base':768,'foreground_frame':self.foreground_frame,'credit_frame':self.credit_frame,'capture_names_frame':self.capture_names_frame,'vs_frame':self.vs_frame}
        (OUT/'assets.json').write_text(json.dumps(manifest,separators=(',',':'))+'\n')
        (OUT/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256((OUT/n).read_bytes()).hexdigest()+'  '+n+'\n'for n in ['TEX.BIN','RING.BIN','CRAM.BIN','wp02_assets.h','wp02_assets.c']))
        return manifest


def main():
    e=Exporter();e.actors();snap=snapshot(0);e.backgrounds(snap)
    e.credit_frame=e.text(snapshot(128)['text'],'credit')
    e.capture_names_frame=e.text(snap['text'],'captured_names')
    e.names()
    # Common VS text is exactly the remaining captured non-name cells.
    all_name_cells=set()
    for identity,slot in zip([0,6,9,1],range(4)):
        all_name_cells.update(off//4 for off,code in e.name_cells(identity,slot))
    vs=bytearray(snap['text'])
    for cell in all_name_cells:vs[cell*4:cell*4+4]=b'\0'*4
    e.vs_frame=e.text(vs,'versus')
    original=e.sprites(snap,'captured_actor_stream')
    for frame_index in [q['frame']for q in e.name_frames]+[e.vs_frame,e.credit_frame]:
        e.flatten_wordmark(frame_index)
    m=e.emit()
    e.render([original,e.foreground_frame,e.capture_names_frame]).save(OUT/'oracle_reconstruction.png')
    live_names=[next(q['frame'] for q in e.name_frames if q['identity']==identity and q['slot']==slot) for slot,identity in enumerate([0,6,9,1])]
    native=e.render([original,e.foreground_frame,*live_names,e.vs_frame,e.credit_frame])
    native.save(OUT/'native_composition_reference.png')
    ref=Image.open(ROOT.parent/'verification/mame/full/sn0030.png').convert('RGB')
    diff=np.any(np.asarray(native)!=np.asarray(ref),axis=2)
    (OUT/'pixel_comparison.json').write_text(json.dumps({'comparison':'native ROM name builder + exact sprite snapshot + exact ring maps vs MAME snapshot30','differing_pixels':int(diff.sum()),'pixels':320*240,'reference':'verification/mame/full/sn0030.png'},indent=2)+'\n')
    e.render([e.foreground_frame]).save(OUT/'ring_preview.png')
    (OUT/'sprite_preview.png').write_bytes((OUT/'oracle_reconstruction.png').read_bytes())
    print(json.dumps({'texture_bytes':m['texture_bytes'],'tiles':len(m['tiles']),'palette_banks':len(m['palette_banks']),'poses':len(m['poses']),'foreground_pieces':e.frames[e.foreground_frame]['count'],'frames':len(e.frames),'pieces':len(e.pieces),'ring_colors':len(e.ring_colors),'output':str(OUT)},indent=2))


if __name__=='__main__':main()
