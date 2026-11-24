#!/usr/bin/env python3
"""WWF WrestleFest (Technos, 1991) exact graphics/metadata extractor.

Input: user-supplied wwfwfest.zip. Output: derived manifests/hashes and optional
packed 4-bpp Saturn-friendly assets. Never emits complete ROM chips or program.bin.
All offsets are main 68000 byte addresses after even/odd reconstruction.
"""
from __future__ import annotations
import argparse,csv,hashlib,json,struct,zipfile,zlib
from pathlib import Path

MAIN_SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
MAIN_SHA256='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
ROM_SPECS={
'31a11-2.ic42':(0x10000,'5ddebfea','30073963e965250d94f0dc3bd261a054850adf95'),
'31e12-0.ic33':(0x20000,'06f22615','2e9418e372da85ea597977d912d8b35753655f4e'),
'31e13-0.ic19':(0x40000,'bd02e3c4','7ae63e48caf9919ce7b63b4c5aa9474ba8c336da'),
'31e14-0.ic18':(0x40000,'933ea1a0','61da142cfa7abd3b77ab21979c061a078c0d0c63'),
'31j0.ic1':(0x40000,'8a12b450','2e15c949efcda8bb6f11afe3ff07ba1dee9c771c'),
'31j1.ic2':(0x40000,'82ed7155','b338e1150ffe3277c11d4d6e801a7d3bd7c58492'),
'31j10.ic73':(0x80000,'6c522edb','8005d59c94160638ba2ea7caf4e991fff03003d5'),
'31j2.ic8':(0x100000,'b5a97465','08d82c29a5c02b83fdbd0bad649b74eb35ab7e54'),
'31j3.ic9':(0x100000,'e395cf1d','241f98145e295993c9b6a44dc087a9b61fbc9a6f'),
'31j4.ic10':(0x100000,'00edb66a','926606d1923936b6e75391b1ab03b369d9822d13'),
'31j5.ic11':(0x100000,'2ce545e8','82173e58a8476a6fe9d2c990fce1f71af117a0ea'),
'31j6.ic12':(0x100000,'79956cf8','52207263620a6b6dde66d3f8749b772577899ea5'),
'31j7.ic13':(0x100000,'74d774c3','a723ac5d481bf91b12e17652fbb2d869c886dec0'),
'31j8.ic14':(0x100000,'44abe127','c723e1dea117534e976d2d383e634faf073cd57b'),
'31j9.ic15':(0x100000,'dd387289','2cad42d4e7cd1a49346f844058ae18c38bc686a8'),
}
SPRITE_ORDER=['31j3.ic9','31j2.ic8','31j5.ic11','31j4.ic10','31j6.ic12','31j7.ic13','31j9.ic15','31j8.ic14']
POSE_SET_TABLE=0x38F14; POSE_BASE_TABLE=0x38FB8; SET_SLOTS=0x52
PALETTE_RESOURCE_TABLE=0x2F22
TEXT_PTR_TABLE=0x25204; TEXT_GROUPS=0x67
WORLD_FG_ROOT_TABLE=0x2709C; WORLD_BG_ROOT_TABLE=0x270BC; WORLD_MODES=8
WORLD_PRIORITY_TABLE=0x26E9E
BG_PALETTE_FG_PTR_TABLE=0x2A98; BG_PALETTE_BG_PTR_TABLE=0x2AB4; BG_PALETTE_MODES=7
TEXT_PALETTE_CONFIG_TABLE=0x2B82; TEXT_PALETTE_CONFIGS=4; TEXT_PALETTE_SLOTS=16


def sha256(b:bytes)->str:return hashlib.sha256(b).hexdigest()
def be16(b,o):return int.from_bytes(b[o:o+2],'big')
def be32(b,o):return int.from_bytes(b[o:o+4],'big')
def s8(v):return v-256 if v&0x80 else v
def pack4(px,w,h):
    out=bytearray()
    for y in range(h):
        for x in range(0,w,2):out.append((px[y*w+x]<<4)|px[y*w+x+1])
    return bytes(out)
def rgb444_to_saturn(v):
    r=v&15;g=(v>>4)&15;b=(v>>8)&15
    r=(r<<1)|(r>>3);g=(g<<1)|(g>>3);b=(b<<1)|(b>>3)
    return 0x8000|r|(g<<5)|(b<<10)

class Rom:
    def __init__(self,zpath:Path):
        self.zpath=Path(zpath)
        with zipfile.ZipFile(self.zpath) as z:
            bybase={Path(n).name:n for n in z.namelist()}
            missing=[n for n in ROM_SPECS if n not in bybase]
            if missing:raise ValueError('missing ROM members: '+','.join(missing))
            self.chips={n:z.read(bybase[n]) for n in ROM_SPECS}
        for n,(size,crc,s1) in ROM_SPECS.items():
            d=self.chips[n]
            gotcrc=f'{zlib.crc32(d)&0xffffffff:08x}';gots1=hashlib.sha1(d).hexdigest()
            if len(d)!=size or gotcrc!=crc or gots1!=s1:raise ValueError(f'ROM mismatch {n}: size={len(d):x} crc={gotcrc} sha1={gots1}')
        e=self.chips['31e14-0.ic18'];o=self.chips['31e13-0.ic19']
        p=bytearray(len(e)+len(o));p[0::2]=e;p[1::2]=o;self.p=bytes(p)
        if hashlib.sha1(self.p).hexdigest()!=MAIN_SHA1 or sha256(self.p)!=MAIN_SHA256:raise ValueError('main 68000 hash mismatch')
        self.sprites=b''.join(self.chips[n] for n in SPRITE_ORDER)
        self.bg=self.chips['31j1.ic2']+self.chips['31j0.ic1']
        self.font=self.chips['31e12-0.ic33']
        if len(self.sprites)!=0x800000 or len(self.bg)!=0x80000 or len(self.font)!=0x20000:raise AssertionError('graphics region size')

    def sprite_tile(self,code):
        if not 0<=code<65536:raise IndexError(code)
        px=[0]*256;plane_span=65536*32
        for plane,weight in enumerate((8,4,2,1)):
            a=plane*plane_span+code*32
            for y in range(16):
                for half in range(2):
                    b=self.sprites[a+half*16+y]
                    for x in range(8):px[y*16+half*8+x]|=((b>>(7-x))&1)*weight
        return bytes(px)
    def bg_tile(self,code):
        if not 0<=code<4096:raise IndexError(code)
        px=[0]*256;half=0x40000
        for y in range(16):
            for x in range(16):
                off=code*64+y*2+(32 if x>=8 else 0);bit=7-(x&7)
                for weight,boff in zip((8,4,2,1),(1,0,half+1,half)):
                    px[y*16+x]|=((self.bg[off+boff]>>bit)&1)*weight
        return bytes(px)
    def text_tile(self,code):
        if not 0<=code<4096:raise IndexError(code)
        px=[0]*64
        for y in range(8):
            for x in range(8):
                b=self.font[code*32+(x//2)*8+y];v=0
                for plane in range(4):v|=((b>>(7-(1-x%2)-plane*2))&1)<<(3-plane)
                px[y*8+x]=v
        return bytes(px)

    # ---- metasprite tables ----
    def set_offsets(self):return [be16(self.p,POSE_SET_TABLE+i*2) for i in range(SET_SLOTS)]
    def set_base(self,s):return be32(self.p,POSE_BASE_TABLE+s*4)
    def active_sets(self):
        offs=self.set_offsets();active=[]
        for s,o in enumerate(offs):
            if o not in (0,0xffff):active.append((s,o,self.set_base(s)))
        vals=[o for _,o,_ in active]
        if vals!=sorted(vals) or len(vals)!=len(set(vals)):raise ValueError('pose set offsets not strictly monotonic')
        end=self.set_base(active[0][0])-POSE_SET_TABLE
        for i,(s,o,b) in enumerate(active):
            nxt=active[i+1][1] if i+1<len(active) else end
            if nxt<o or (nxt-o)&1:raise ValueError('bad pose-list boundary')
            yield s,o,b,(nxt-o)//2
    def pose_entry(self,s,pose):
        info={x[0]:x for x in self.active_sets()}.get(s)
        if not info:return None
        _,off,base,count=info
        if not 0<=pose<count:return None
        rel=be16(self.p,POSE_SET_TABLE+off+pose*2)
        if rel in (0xfffe,0xffff):return rel
        a=base+rel
        if not 0<=a<len(self.p)-2:raise ValueError((s,pose,a))
        return a
    def raw_rows(self,addr,base,partner,depth=0):
        if depth>=8:raise ValueError('composition recursion depth')
        p=self.p;h=be16(p,addr);q=addr+2;kind=h>>14;n=h&255
        if kind==3:
            out=[]
            for i in range(n):out+=self.raw_rows(base+be16(p,q+i*2),base,partner,depth+1)
            return out
        if kind==2:
            if not 0<=partner<12:raise ValueError('kind2 partner must be 0..11')
            otherpose=be16(p,q);own=base+be16(p,q+2);q+=4
            other=self.pose_entry(partner,otherpose&0x7fff)
            if other in (None,0xfffe,0xffff):return []
            dx,dy=p[q+partner*2:q+partner*2+2]
            def segments(a,b):
                hh=be16(p,a)
                if hh>>14==3:return [self.raw_rows(b+be16(p,a+2+i*2),b,partner,depth+1) for i in range(hh&255)]
                return [self.raw_rows(a,b,partner,depth+1)]
            parts=[segments(own,base),segments(other,self.set_base(partner))]
            for seg in parts[1]:
                for r in seg:
                    r[0]=(r[0]+dx)&255
                    r[1]=(r[1]+dy)&255
            out=[];j=[0,0];bits=h&0x3fff
            while any(j[i]<len(parts[i]) for i in range(2)):
                k=bits&1;bits>>=1
                if j[k]>=len(parts[k]):k=1-k
                out+=parts[k][j[k]];j[k]+=1
            return out
        def reference(pos,tail):
            count=p[pos];flags=p[pos+1];target=be32(p,pos)&0xfffff;pos+=4;offset=[0,0,0]
            for i,bit in enumerate((64,32,16)):
                if flags&bit:offset[i]=p[pos];pos+=1
            rr=self.raw_rows(target,base,partner,depth+1);rr=rr[-count:] if tail else rr[:count]
            for r in rr:
                for i,k in enumerate((0,1,3)):r[k]=(r[k]+offset[i])&255
            return rr,pos
        before=[];after=[]
        if h&0x200:before,q=reference(q,False)
        if kind==0:
            out=[list(p[q+i*6:q+i*6+6]) for i in range(n)];q+=n*6
        else:
            q=(q+1)&~1;fmt=be16(p,q);q+=2;cols=[]
            for c in range(6):
                mode=(fmt>>(2*c))&3
                if mode==0:col=list(p[q:q+n]);q+=n
                elif mode==1:col=[p[q]]*n;q+=1
                else:
                    q=(q+1)&~1;off=0x6B0B0+be16(p,q)+(0x8000 if mode==3 else 0);q+=2;col=list(p[off:off+n])
                    if len(col)!=n:raise ValueError('column table overflow')
                cols.append(col)
            out=[list(r) for r in zip(*cols)]
        if h&0x100:after,q=reference((q+1)&~1,True)
        return before+out+after
    def pose_rows(self,s,pose,partner=0):
        a=self.pose_entry(s,pose&0x7fff)
        if a in (None,0xfffe,0xffff):return []
        return self.raw_rows(a,self.set_base(s),partner)
    def expanded_pieces(self,s,pose,partner=0,mirror=False):
        rows=self.pose_rows(s,pose,partner);out=[]
        for ri,(dx,dy,attr,res,hi,lo) in enumerate(rows):
            chain=(attr>>5)&7;fx=bool(attr&0x10)^bool(mirror);fy=bool(attr&0x08)
            rx=-s8(dx) if mirror else s8(dx);ry=-s8(dy)
            for c in range(chain+1):
                oy=(-chain*16+c*16) if fy else (-c*16)
                out.append({'row':ri,'chain_index':c,'tile':(hi<<8)|lo|0,'tile_code':((hi<<8)|lo)+c,
                            'palette_resource':res,'rel_x':rx,'rel_y':ry+oy,'flip_x':int(fx),'flip_y':int(fy),
                            'raw_dx':dx,'raw_dy':dy,'raw_attr':attr,'chain':chain})
        return out

    # ---- palette domains ----
    def resource_palette(self,res,effective=False):
        vals=[be16(self.p,PALETTE_RESOURCE_TABLE+(res&255)*32+i*2) for i in range(16)]
        if effective:vals=[0]+vals[1:]
        return vals
    def text_palette_ptr(self,cfg,slot):return be32(self.p,TEXT_PALETTE_CONFIG_TABLE+cfg*64+slot*4)
    def text_palette(self,cfg,slot):
        a=self.text_palette_ptr(cfg,slot);return [be16(self.p,a+i*2) for i in range(16)]
    def bg_palette_ptr(self,which,mode):
        base=BG_PALETTE_FG_PTR_TABLE if which=='fg' else BG_PALETTE_BG_PTR_TABLE
        return be32(self.p,base+mode*4)
    def bg_palette(self,which,mode):
        a=self.bg_palette_ptr(which,mode);return [be16(self.p,a+i*2) for i in range(256)]

    # ---- text descriptor system ----
    def text_desc(self,gid):
        a=be32(self.p,TEXT_PTR_TABLE+gid*4);typ=self.p[a];bank=self.p[a+1]&15;base=be16(self.p,a+2);q=a+4
        events=[];glyphs=[];cursor=base;linebase=base
        strides=(0x100,0x200,0x200,0x100)
        while True:
            b=self.p[q];q+=1
            if b>=8:
                if b<0x10:raise ValueError(f'text display byte {b:02x} below 10 in group {gid:02x}')
                ti=cursor//4;x=(ti%64)*8;y=(ti//64)*8
                if typ==0:tiles=[b-0x10];adv=4
                elif typ==1:tiles=[(b-0x10)*2+0x30,(b-0x10)*2+0x31];adv=4
                elif typ==2:
                    c=(b-0x10)*4+0x94;tiles=[c,c+1,c+2,c+3];adv=8
                else:tiles=[];adv=4
                events.append({'kind':'glyph','byte':b,'cursor':cursor,'x':x,'y':y,'tiles':tiles})
                for t in tiles:glyphs.append(t)
                cursor=(cursor+adv)&0xffff
            elif b==0:events.append({'kind':'end'});break
            elif b==1:
                if typ>=len(strides):raise ValueError('bad text type newline')
                linebase=(linebase+strides[typ])&0xffff;cursor=linebase;events.append({'kind':'newline','cursor':cursor})
            elif b==2:
                param=self.p[q];q+=1;events.append({'kind':'control2','param':param})
            elif b==3:
                cursor=be16(self.p,q);q+=2;linebase=cursor;events.append({'kind':'cursor','cursor':cursor})
            else:raise ValueError(f'unknown text control {b:02x} group {gid:02x}')
            if q>=len(self.p):raise ValueError('text runaway')
        return {'group':gid,'address':a,'type':typ,'palette_slot':bank,'base_cursor':base,'events':events,'glyph_tiles':glyphs,'end_address':q}

    # ---- world maps ----
    def world_root(self,layer,mode):return be32(self.p,(WORLD_FG_ROOT_TABLE if layer=='fg' else WORLD_BG_ROOT_TABLE)+mode*4)
    def world_page_ptrs(self,layer,mode):
        root=self.world_root(layer,mode);return [be32(self.p,root+i*4) for i in range(64)]
    def world_page_entries(self,layer,ptr):
        out=[]
        if layer=='fg':
            # 20x16 16px tiles, 4 bytes/tile; attr+code. Stored row-major in 10x8 2x2 metatiles.
            for my in range(8):
                for mx in range(10):
                    base=ptr+my*0xA0+mx*8
                    for sy in range(2):
                        for sx in range(2):
                            o=base+sy*0x50+sx*4;attr=be16(self.p,o);codew=be16(self.p,o+2)
                            out.append((mx*2+sx,my*2+sy,codew&0x1fff,attr&0xf,(attr>>6)&1,(attr>>7)&1,attr,codew))
        else:
            for my in range(8):
                for mx in range(10):
                    base=ptr+my*0x50+mx*4
                    for sy in range(2):
                        for sx in range(2):
                            o=base+sy*0x28+sx*2;w=be16(self.p,o)
                            out.append((mx*2+sx,my*2+sy,w&0x0fff,(w>>12)&0xf,0,0,w,w))
        return out


def write_csv(path,fieldnames,rows):
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=fieldnames);w.writeheader();w.writerows(rows)

def tile_manifest(rom:Rom,out:Path,kind:str,count:int):
    decode={'sprite':rom.sprite_tile,'background':rom.bg_tile,'text':rom.text_tile}[kind]
    rows=[];first_by_hash={};unique=0;blank=0
    for code in range(count):
        px=decode(code);packed=pack4(px,16,16) if kind!='text' else pack4(px,8,8);h=sha256(packed);isblank=not any(px)
        if h not in first_by_hash:first_by_hash[h]=code;unique+=1
        if isblank:blank+=1
        rows.append({'code':f'0x{code:04X}','packed_bytes':len(packed),'sha256':h,'blank':int(isblank),'duplicate_of': '' if first_by_hash[h]==code else f'0x{first_by_hash[h]:04X}'})
    write_csv(out/f'{kind}_tile_hashes.csv',['code','packed_bytes','sha256','blank','duplicate_of'],rows)
    return {'tiles':count,'unique_pixel_tiles':unique,'blank_tiles':blank,'manifest_sha256':sha256((out/f'{kind}_tile_hashes.csv').read_bytes())}

def generate(rom:Rom,out:Path,emit_assets=False):
    out.mkdir(parents=True,exist_ok=True)
    summary={'main_sha1':MAIN_SHA1,'main_sha256':MAIN_SHA256,'graphics':{},'metasprites':{},'palettes':{},'text':{},'world_maps':{}}
    # input ROM manifest
    rows=[]
    for n,(size,crc,s1) in ROM_SPECS.items():
        d=rom.chips[n];role=('program-even' if n=='31e14-0.ic18' else 'program-odd' if n=='31e13-0.ic19' else 'text-gfx' if n=='31e12-0.ic33' else 'background-gfx' if n in ('31j0.ic1','31j1.ic2') else 'sprite-gfx' if n in SPRITE_ORDER else 'non-graphics/support')
        rows.append({'file':n,'bytes':len(d),'crc32':f'{zlib.crc32(d)&0xffffffff:08x}','sha1':hashlib.sha1(d).hexdigest(),'sha256':sha256(d),'role':role})
    write_csv(out/'rom_inputs.csv',['file','bytes','crc32','sha1','sha256','role'],rows)
    # layout metadata
    layouts={
      'sprite16':{'region_bytes':0x800000,'tiles':65536,'tile':'16x16','bpp':4,'plane_span':0x200000,'plane_weights':[8,4,2,1],'rom_order':SPRITE_ORDER,'bytes_per_plane_tile':32,'left_half_byte_index':'y','right_half_byte_index':'16+y','bit_order':'MSB-first'},
      'background16':{'region_bytes':0x80000,'tiles':4096,'tile':'16x16','bpp':4,'rom_order':['31j1.ic2','31j0.ic1'],'tile_base_bytes':64,'plane_byte_offsets':[1,0,0x40001,0x40000],'plane_weights':[8,4,2,1],'right_half_add':32,'row_stride':2,'bit_order':'MSB-first'},
      'text8':{'region_bytes':0x20000,'tiles':4096,'tile':'8x8','bpp':4,'rom_order':['31e12-0.ic33'],'bytes_per_tile':32,'byte_index':'tile*32+(x//2)*8+y','bit_positions_even_x':[6,4,2,0],'bit_positions_odd_x':[7,5,3,1],'plane_weights':[8,4,2,1]}}
    (out/'graphics_layouts.json').write_text(json.dumps(layouts,indent=2)+'\n')
    contracts={
      'metasprite_format':{
        'set_offset_table':'0x038F14','set_slots':82,'set_base_pointer_table':'0x038FB8',
        'pose_entry':'u16 BE relative offset; 0xFFFE=MISSING; 0xFFFF=TERMINATOR',
        'pose_selector_global_flip':'selector bit15 is removed from pose index and mirrors X while XORing each piece flip_x',
        'row_bytes':['dx:u8 signed at consume','dy:u8 signed at consume','attr:u8','palette_resource:u8','tile_hi:u8','tile_lo:u8'],
        'attr':{'flip_x':'bit4','flip_y':'bit3','vertical_chain':'bits5..7, emits chain+1 sequential 16x16 tile codes'},
        'relative_position':{'normal_x':'s8(dx)','mirror_x':'-s8(dx)','base_y':'-s8(dy)','flip_y_chain':'-chain*16 + i*16','normal_chain':'-i*16'},
        'kinds':{'0':'literal six-byte rows','1':'column-coded rows','2':'two-actor composition with per-partner offsets and exact interleave mask','3':'subcomposition list'},
        'kind2_partner_domain':'graphics set 0x00..0x0B; all 12 variants are enumerated in manifests',
        'palette_field':'full u8 palette resource ID; never mask to 0x0F'
      },
      'palette_runtime':{
        'sprite_loader':'0x002AEA','resource_table':'0x002F22','resource_count':256,'resource_stride_bytes':32,
        'slot_busy':'0x001C1600','resource_cache':'0x001C1610','dynamic_hardware_slots':16,'destination':'0x00182000',
        'allocation':'first free slot 0..15; cache stores slot plus loaded bit7',
        'pen0':'destination color0 is explicitly cleared; source color0 is skipped; colors1..15 copied',
        'format':'0BGR 4:4:4 in arcade program data; Saturn conversion included as deterministic 5:5:5 bit replication',
        'text_palette_loader':'0x002A4C','text_config_pointer_table':'0x002B82','text_configs':4,'text_destination':'0x00180000',
        'world_palette_loader':'0x002A06','world_palette_mode_global':'0x001C15F4','world_fg_pointer_table':'0x002A98','world_bg_pointer_table':'0x002AB4','world_modes':7,'world_destinations':['0x00188000','0x00186000']
      },
      'text_format':{
        'pointer_table':'0x025204','groups':103,'descriptor':['type:u8','palette_slot:u8','base_cursor:u16 BE','bytecode stream'],
        'controls':{'0':'end','1':'newline','2':'consume one parameter byte','3':'set base/cursor from next BE u16'},
        'types':{'0':'8x8, tile=b-0x10, cursor+=4','1':'8x16, tiles=(b-0x10)*2+0x30/+1, cursor+=4','2':'16x16 2x2, base=(b-0x10)*4+0x94, cursor+=8'},
        'font_rom':'31e12-0.ic33','font_layout':'graphics_layouts.json:text8','pen0':'transparent in FG/text consumer; do not substitute a host font'
      },
      'world_map_format':{
        'builder':'0x026E66','world_mode_global':'0x001C007E','world_modes':8,'priority_table':'0x026E9E',
        'fg_root_table':'0x02709C','bg_root_table':'0x0270BC','root':'64 BE u32 page pointers arranged 8x8',
        'fg_page':'10x8 metatiles -> 20x16 tiles, 0x500 bytes, each tile attr:u16 + code:u16; code=code&0x1FFF, palette=attr&0xF, flipX=attr.bit6, flipY=attr.bit7',
        'bg_page':'10x8 metatiles -> 20x16 tiles, 0x280 bytes, each tile u16; code=word&0x0FFF, palette=word>>12, no flip bits',
        'note':'layer draw/priority composition belongs renderer/workpack26; this package preserves exact raw priority bytes and map attributes'
      }
    }
    for fn,obj in [('metasprite_format.json',contracts['metasprite_format']),('palette_runtime_contract.json',contracts['palette_runtime']),('text_format.json',contracts['text_format']),('world_map_format.json',contracts['world_map_format'])]:
        (out/fn).write_text(json.dumps(obj,indent=2,sort_keys=True)+'\n')
    summary['graphics']['sprite']=tile_manifest(rom,out,'sprite',65536)
    summary['graphics']['background']=tile_manifest(rom,out,'background',4096)
    summary['graphics']['text']=tile_manifest(rom,out,'text',4096)
    # metasprite set + pose + pieces manifests
    set_rows=[];pose_rows=[];pair_rows=[];piece_rows=[];kind_counts={0:0,1:0,2:0,3:0};valid=missing=endmarks=0;piece_count=0
    active=list(rom.active_sets());active_by={s:(o,b,c) for s,o,b,c in active}
    for s in range(SET_SLOTS):
        if s not in active_by:
            off=rom.set_offsets()[s];set_rows.append({'set':f'0x{s:02X}','active':0,'list_offset':f'0x{off:04X}','list_address':'','base_address':f'0x{rom.set_base(s):06X}','pose_slots':0,'valid_poses':0,'missing_fffe':0,'terminator_ffff':0,'k0':0,'k1':0,'k2':0,'k3':0,'palette_resources':'','tile_min':'','tile_max':''});continue
        off,base,count=active_by[s];sk={0:0,1:0,2:0,3:0};sv=sm=se=0;spals=set();stiles=set()
        for pose in range(count):
            ent=rom.pose_entry(s,pose)
            if ent==0xfffe:
                sm+=1;missing+=1;pose_rows.append({'set':f'0x{s:02X}','pose':pose,'entry_kind':'MISSING_FFFE','record_address':'','record_kind':'','header':'','partner_pose':'','piece_variants':0,'palette_resources':'','tile_min':'','tile_max':'','piece_hash':''});continue
            if ent==0xffff:
                se+=1;endmarks+=1;pose_rows.append({'set':f'0x{s:02X}','pose':pose,'entry_kind':'TERMINATOR_FFFF','record_address':'','record_kind':'','header':'','partner_pose':'','piece_variants':0,'palette_resources':'','tile_min':'','tile_max':'','piece_hash':''});continue
            h=be16(rom.p,ent);k=h>>14;sk[k]+=1;kind_counts[k]+=1;sv+=1;valid+=1
            partners=range(12) if k==2 else (0,);variant_hashes=[];ppals=set();ptiles=set();pv=0
            partner_pose=be16(rom.p,ent+2) if k==2 else ''
            if k==2:
                q=ent+6
                for partner in range(12):
                    pair_rows.append({'set':f'0x{s:02X}','pose':pose,'partner_set':f'0x{partner:02X}','partner_pose':f'0x{partner_pose:04X}','offset_x':s8(rom.p[q+partner*2]),'offset_y':s8(rom.p[q+partner*2+1]),'record_address':f'0x{ent:06X}'})
            for partner in partners:
                pcs=rom.expanded_pieces(s,pose,partner)
                stable=[]
                for pi,q in enumerate(pcs):
                    ppals.add(q['palette_resource']);ptiles.add(q['tile_code']);spals.add(q['palette_resource']);stiles.add(q['tile_code']);piece_count+=1;pv+=1
                    row={'set':f'0x{s:02X}','pose':pose,'partner_set':f'0x{partner:02X}' if k==2 else '', 'piece_index':pi,'source_row':q['row'],'chain_index':q['chain_index'],'tile_code':f"0x{q['tile_code']:04X}",'palette_resource':f"0x{q['palette_resource']:02X}",'rel_x':q['rel_x'],'rel_y':q['rel_y'],'flip_x':q['flip_x'],'flip_y':q['flip_y'],'raw_dx':f"0x{q['raw_dx']:02X}",'raw_dy':f"0x{q['raw_dy']:02X}",'raw_attr':f"0x{q['raw_attr']:02X}",'chain':q['chain']}
                    piece_rows.append(row);stable.append([q[x] for x in ('tile_code','palette_resource','rel_x','rel_y','flip_x','flip_y')])
                variant_hashes.append(sha256(json.dumps(stable,separators=(',',':')).encode()))
            ph=sha256(('\n'.join(variant_hashes)).encode())
            pose_rows.append({'set':f'0x{s:02X}','pose':pose,'entry_kind':'RECORD','record_address':f'0x{ent:06X}','record_kind':k,'header':f'0x{h:04X}','partner_pose':f'0x{partner_pose:04X}' if k==2 else '','piece_variants':pv,'palette_resources':' '.join(f'{x:02X}' for x in sorted(ppals)),'tile_min':f'0x{min(ptiles):04X}' if ptiles else '','tile_max':f'0x{max(ptiles):04X}' if ptiles else '','piece_hash':ph})
        set_rows.append({'set':f'0x{s:02X}','active':1,'list_offset':f'0x{off:04X}','list_address':f'0x{POSE_SET_TABLE+off:06X}','base_address':f'0x{base:06X}','pose_slots':count,'valid_poses':sv,'missing_fffe':sm,'terminator_ffff':se,'k0':sk[0],'k1':sk[1],'k2':sk[2],'k3':sk[3],'palette_resources':' '.join(f'{x:02X}' for x in sorted(spals)),'tile_min':f'0x{min(stiles):04X}' if stiles else '','tile_max':f'0x{max(stiles):04X}' if stiles else ''})
    write_csv(out/'metasprite_sets.csv',list(set_rows[0]),set_rows)
    write_csv(out/'metasprite_poses.csv',list(pose_rows[0]),pose_rows)
    write_csv(out/'metasprite_pair_offsets.csv',list(pair_rows[0]),pair_rows)
    write_csv(out/'metasprite_expanded_pieces.csv',list(piece_rows[0]),piece_rows)
    summary['metasprites']={'set_slots':SET_SLOTS,'active_sets':len(active),'valid_records':valid,'missing_fffe':missing,'terminator_ffff':endmarks,'record_kinds':{str(k):v for k,v in kind_counts.items()},'expanded_piece_variants':piece_count,'sets_sha256':sha256((out/'metasprite_sets.csv').read_bytes()),'poses_sha256':sha256((out/'metasprite_poses.csv').read_bytes()),'pieces_sha256':sha256((out/'metasprite_expanded_pieces.csv').read_bytes())}
    # palette resources: exact loader semantics: destination pen0 cleared; source word0 skipped.
    pres=[]
    for res in range(256):
        raw=rom.resource_palette(res,False);eff=rom.resource_palette(res,True)
        pres.append({'resource':f'0x{res:02X}','source_address':f'0x{PALETTE_RESOURCE_TABLE+res*32:06X}','raw_rgb444':' '.join(f'{v:03X}' for v in raw),'effective_rgb444':' '.join(f'{v:03X}' for v in eff),'effective_saturn_bgr555':' '.join(f'{rgb444_to_saturn(v):04X}' for v in eff),'raw_sha256':sha256(b''.join(struct.pack('>H',v) for v in raw)),'effective_sha256':sha256(b''.join(struct.pack('>H',v) for v in eff))})
    write_csv(out/'sprite_palette_resources.csv',list(pres[0]),pres)
    # text palette configs
    tr=[]
    for cfg in range(TEXT_PALETTE_CONFIGS):
        for slot in range(16):
            ptr=rom.text_palette_ptr(cfg,slot);vals=rom.text_palette(cfg,slot)
            tr.append({'config':cfg,'slot':slot,'pointer':f'0x{ptr:06X}','rgb444':' '.join(f'{v:03X}' for v in vals),'saturn_bgr555':' '.join(f'{rgb444_to_saturn(v):04X}' for v in vals),'sha256':sha256(b''.join(struct.pack('>H',v) for v in vals))})
    write_csv(out/'text_palette_configs.csv',list(tr[0]),tr)
    # background palette modes
    br=[]
    for mode in range(BG_PALETTE_MODES):
        for layer in ('fg','bg'):
            ptr=rom.bg_palette_ptr(layer,mode);vals=rom.bg_palette(layer,mode)
            br.append({'palette_mode':mode,'layer':layer,'source_address':f'0x{ptr:06X}','words':256,'rgb444_sha256':sha256(b''.join(struct.pack('>H',v) for v in vals)),'saturn_bgr555_sha256':sha256(b''.join(struct.pack('>H',rgb444_to_saturn(v)) for v in vals))})
    write_csv(out/'background_palette_modes.csv',list(br[0]),br)
    summary['palettes']={'sprite_resources':256,'sprite_resource_table':'0x002F22','sprite_loader_entry':'0x002AEA','dynamic_slots':16,'resource_cache':'0x001C1610','slot_busy_table':'0x001C1600','destination':'0x00182000','text_configs':4,'text_config_table':'0x002B82','text_destination':'0x00180000','background_palette_modes':7,'background_fg_ptr_table':'0x002A98','background_bg_ptr_table':'0x002AB4','background_destinations':['0x00188000','0x00186000']}
    # text groups
    td=[];te=[];allglyph=set();types={}
    for gid in range(TEXT_GROUPS):
        d=rom.text_desc(gid);types[d['type']]=types.get(d['type'],0)+1;allglyph.update(d['glyph_tiles'])
        td.append({'group':f'0x{gid:02X}','address':f"0x{d['address']:06X}",'type':d['type'],'palette_slot':d['palette_slot'],'base_cursor':f"0x{d['base_cursor']:04X}",'event_count':len(d['events']),'glyph_event_count':sum(e['kind']=='glyph' for e in d['events']),'glyph_tiles':' '.join(f'{x:04X}' for x in sorted(set(d['glyph_tiles']))),'end_address':f"0x{d['end_address']:06X}"})
        for idx,e in enumerate(d['events']):te.append({'group':f'0x{gid:02X}','event':idx,'kind':e['kind'],'byte':f"0x{e['byte']:02X}" if 'byte'in e else '','param':f"0x{e['param']:02X}" if 'param'in e else '','cursor':f"0x{e['cursor']:04X}" if 'cursor'in e else '','x':e.get('x',''),'y':e.get('y',''),'tiles':' '.join(f'{x:04X}' for x in e.get('tiles',[]))})
    write_csv(out/'text_groups.csv',list(td[0]),td);write_csv(out/'text_events.csv',list(te[0]),te)
    summary['text']={'groups':TEXT_GROUPS,'pointer_table':'0x025204','types':types,'referenced_font_tiles':len(allglyph),'groups_sha256':sha256((out/'text_groups.csv').read_bytes()),'events_sha256':sha256((out/'text_events.csv').read_bytes())}
    # world map roots/pages and unique page entries
    wr=[];page_occ=[];unique_pages={}
    for mode in range(WORLD_MODES):
        wr.append({'world_mode':mode,'fg_root':f'0x{rom.world_root("fg",mode):06X}','bg_root':f'0x{rom.world_root("bg",mode):06X}','priority_byte':f'0x{rom.p[WORLD_PRIORITY_TABLE+mode]:02X}'})
        for layer in ('fg','bg'):
            size=0x500 if layer=='fg' else 0x280
            for idx,ptr in enumerate(rom.world_page_ptrs(layer,mode)):
                if not 0<=ptr<=len(rom.p)-size:raise ValueError(('world page',mode,layer,idx,ptr))
                key=(layer,ptr);unique_pages[key]=size
                page_occ.append({'world_mode':mode,'layer':layer,'page_index':idx,'page_x':idx%8,'page_y':idx//8,'page_pointer':f'0x{ptr:06X}','page_bytes':size,'page_sha256':sha256(rom.p[ptr:ptr+size])})
    write_csv(out/'world_modes.csv',list(wr[0]),wr);write_csv(out/'world_page_occurrences.csv',list(page_occ[0]),page_occ)
    pe=[]
    for (layer,ptr),size in sorted(unique_pages.items()):
        for x,y,code,pal,fx,fy,attr,codew in rom.world_page_entries(layer,ptr):
            pe.append({'layer':layer,'page_pointer':f'0x{ptr:06X}','tile_x':x,'tile_y':y,'tile_code':f'0x{code:04X}','palette_bank':pal,'flip_x':fx,'flip_y':fy,'raw_attr':f'0x{attr:04X}','raw_code_word':f'0x{codew:04X}'})
    write_csv(out/'world_unique_page_tiles.csv',list(pe[0]),pe)
    summary['world_maps']={'world_modes':8,'fg_root_table':'0x02709C','bg_root_table':'0x0270BC','priority_table':'0x026E9E','page_grid':'8x8 pointers per root','fg_page_geometry':'20x16 tiles / 0x500 bytes','bg_page_geometry':'20x16 tiles / 0x280 bytes','unique_pages':len(unique_pages),'page_occurrences':len(page_occ),'tiles_manifest_sha256':sha256((out/'world_unique_page_tiles.csv').read_bytes())}
    # semantic anchors proved by related FINAL packages; numeric mapping only here.
    semantic=[
      {'asset':'combat graphics sources','set_or_group':'sets 0x00..0x0B','evidence':'Animation FINAL preserves logical->graphics mapping 0,1,2,3,10,11,6,7,8,9,4,5','status':'ROM-EXACT mapping from FINAL; pixels/composition extracted here'},
      {'asset':'referee','set_or_group':'set 0x0C','evidence':'Referee FINAL actor +$02=$000C; set has 18 records + FFFF terminator','status':'ROM-EXACT join'},
      {'asset':'TAG announcer/presenter controller','set_or_group':'set 0x1F','evidence':'Referee/Announcer FINAL actor +$02=$001F; selectors 4..7,8..11,13..16','status':'ROM-EXACT numeric selector->composition join; human pose names remain unassigned'},
      {'asset':'interstitial family','set_or_group':'set 0x2C','evidence':'Presentation FINAL/WIP historical anchor; 32 records + FFFF terminator','status':'ROM-EXACT table content; scene semantics only where FINAL proves'},
      {'asset':'attract biography portraits','set_or_group':'sets 0x30..0x3B poses 0..2','evidence':'Attract/Bio FINAL: 12 sets, 3 frames, palette resources 0x10+fighter plus 0x1E','status':'ROM-EXACT'},
      {'asset':'credits fixed pages','set_or_group':'set 0x4D poses 0,1,2,3,6,7,8,9,10','evidence':'Credits FINAL page table','status':'ROM-EXACT'},
      {'asset':'credits staff blocks','set_or_group':'set 0x50 poses 0..5','evidence':'Credits FINAL staff table; palette resource 0x3B','status':'ROM-EXACT'},
      {'asset':'credits world background','set_or_group':'world mode 4','evidence':'Credits FINAL + direct ROM audit: mode4 roots 0x278DC/0x279DC, palettes 0x4122/0x4F22; builder table yields 0x7C then credits controller 0x001BC0 overrides $140010 with 0x0078 after vblank','status':'ROM-EXACT; apparent priority-table discrepancy resolved by explicit post-builder write'},
      {'asset':'attract bio text/font','set_or_group':'text groups 0x06, state-indexed data/name groups','evidence':'Attract/Bio FINAL: table 0x25204, original 31e12-0.ic33 font, type0 8x8/type1 8x16','status':'ROM-EXACT'}]
    write_csv(out/'semantic_anchors.csv',list(semantic[0]),semantic)
    asset_index=[
      {'domain':'sprite pixels','rom_source':'31j3/ic9 + 31j2/ic8 + 31j5/ic11 + 31j4/ic10 + 31j6/ic12 + 31j7/ic13 + 31j9/ic15 + 31j8/ic14','decode':'graphics_layouts.json:sprite16','palette':'metasprite row palette_resource -> dynamic loader 0x002AEA','consumer_manifest':'metasprite_expanded_pieces.csv','hash_manifest':'sprite_tile_hashes.csv'},
      {'domain':'background tiles','rom_source':'31j1.ic2 + 31j0.ic1','decode':'graphics_layouts.json:background16','palette':'world page palette_bank -> background_palette_modes.csv','consumer_manifest':'world_page_occurrences.csv + world_unique_page_tiles.csv','hash_manifest':'background_tile_hashes.csv'},
      {'domain':'font/text tiles','rom_source':'31e12-0.ic33','decode':'graphics_layouts.json:text8','palette':'text group palette_slot -> text_palette_configs.csv','consumer_manifest':'text_groups.csv + text_events.csv','hash_manifest':'text_tile_hashes.csv'},
      {'domain':'sprite palettes','rom_source':'main 68000 $002F22 + resource*32','decode':'palette_runtime_contract.json','palette':'256 resources; resource->16 hardware slot allocation is runtime dynamic','consumer_manifest':'metasprite_expanded_pieces.csv palette_resource','hash_manifest':'sprite_palette_resources.csv'},
      {'domain':'world maps','rom_source':'main 68000 roots $02709C/$0270BC and pointed pages','decode':'world_map_format.json','palette':'world palette mode $1C15F4 tables $002A98/$002AB4','consumer_manifest':'world_modes.csv + world_page_occurrences.csv','hash_manifest':'world_unique_page_tiles.csv page hashes in world_page_occurrences.csv'}
    ]
    write_csv(out/'asset_consumer_index.csv',list(asset_index[0]),asset_index)
    # Optional extracted packs (not produced by default/package).
    if emit_assets:
        ad=out/'derived_assets';ad.mkdir(exist_ok=True)
        with (ad/'sprites_16x16_4bpp.pak').open('wb') as f:
            for i in range(65536):f.write(pack4(rom.sprite_tile(i),16,16))
        with (ad/'background_16x16_4bpp.pak').open('wb') as f:
            for i in range(4096):f.write(pack4(rom.bg_tile(i),16,16))
        with (ad/'text_8x8_4bpp.pak').open('wb') as f:
            for i in range(4096):f.write(pack4(rom.text_tile(i),8,8))
        pal=bytearray()
        for r in range(256):
            for v in rom.resource_palette(r,True):pal+=struct.pack('>H',rgb444_to_saturn(v))
        (ad/'sprite_palette_resources_saturn_bgr555.bin').write_bytes(pal)
        summary['derived_assets']={x.name:{'bytes':x.stat().st_size,'sha256':sha256(x.read_bytes())} for x in sorted(ad.iterdir())}
    (out/'manifest_summary.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')
    return summary

def main():
    ap=argparse.ArgumentParser();ap.add_argument('rom_zip',type=Path);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--emit-assets',action='store_true')
    a=ap.parse_args();r=Rom(a.rom_zip);s=generate(r,a.out,a.emit_assets);print(json.dumps(s,indent=2,sort_keys=True))
if __name__=='__main__':main()

