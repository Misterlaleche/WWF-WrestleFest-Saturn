from pathlib import Path
from PIL import Image
import importlib.util, hashlib, math, csv, json, sys
ROOT=Path(__file__).resolve().parents[1]
WP25=ROOT/'tools'/'wf25_graphics.py'
spec=importlib.util.spec_from_file_location('wf25',WP25); wf=importlib.util.module_from_spec(spec); spec.loader.exec_module(wf)
ROM=Path(sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip')
R=wf.Rom(ROM); p=R.p
OUT=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/'preview'
GOLDEN=Path(sys.argv[3]) if len(sys.argv)>3 else ROOT/'golden'
OUT.mkdir(parents=True,exist_ok=True); GOLDEN.mkdir(parents=True,exist_ok=True)

def rgb(v,a=255): return ((v&15)*17,((v>>4)&15)*17,((v>>8)&15)*17,a)

def tile_rgba(px,pal,w,h,transparent=True,flipx=False,flipy=False):
    im=Image.new('RGBA',(w,h),(0,0,0,0)); q=im.load()
    for y in range(h):
      yy=h-1-y if flipy else y
      for x in range(w):
        xx=w-1-x if flipx else x; pen=px[yy*w+xx]
        if transparent and pen==0: continue
        q[x,y]=rgb(pal[pen])
    return im

def build_layer(layer,mode=4,scrollx=0x140,scrolly=0x200,palmode=4,transparent_pen0=False):
    tilemap=[[None]*32 for _ in range(32)]
    coarsex=(scrollx&0xffff)>>5; coarsey=(scrolly&0xffff)>>5
    roots=R.world_page_ptrs(layer,mode)
    for outer in range(16):
      e4=coarsey+4-outer; gy=(64-e4)&0xffff
      for inner in range(16):
        gx=(coarsex-3+inner)&0xffff; px=(gx//10)&7; py=(gy//8)&7
        ptr=roots[py*8+px]
        ents={(x,y):(code,pal,fx,fy) for x,y,code,pal,fx,fy,*_ in R.world_page_entries(layer,ptr)}
        mx=gx%10; my=gy%8; dxm=gx&15; dym=gy&15
        for sy in range(2):
          for sx in range(2): tilemap[dym*2+sy][dxm*2+sx]=ents[(mx*2+sx,my*2+sy)]
    palwords=R.bg_palette(layer,palmode); im=Image.new('RGBA',(512,512),(0,0,0,0)); d=im.load()
    for ty in range(32):
      for tx in range(32):
        cell=tilemap[ty][tx]
        if cell is None: continue
        code,pb,fx,fy=cell; arr=R.bg_tile(code&0xfff)
        for y in range(16):
          for x in range(16):
            xx=15-x if fx else x; yy=15-y if fy else y; pen=arr[yy*16+xx]
            if transparent_pen0 and pen==0: continue
            d[tx*16+x,ty*16+y]=rgb(palwords[pb*16+pen])
    return im

def crop_wrap(im,x0=0x140,y0=0,w=320,h=240):
    out=Image.new('RGBA',(w,h),(0,0,0,0)); s=im.load(); d=out.load()
    for y in range(h):
      for x in range(w): d[x,y]=s[(x0+x)&511,(y0+y)&511]
    return out

FG=crop_wrap(build_layer('fg',transparent_pen0=False)); BG=crop_wrap(build_layer('bg',transparent_pen0=True)); BASE=FG.copy(); BASE.alpha_composite(BG)
BASE.save(OUT/'entry_world_7B.png'); BASE.save(GOLDEN/'ENTRY-WORLD-7B.png')

def init_textmap(): return bytearray(0x2000)
def be16(a): return int.from_bytes(p[a:a+2],'big')
def be32(a): return int.from_bytes(p[a:a+4],'big')
def desc_9aca(mem,a2,d2,d7):
    a1=be32(0x9B26+(d2&0xff)*4); a3=a2; d2v=be16(a1); d3=be16(a1+2); off=4
    while True:
      for _ in range(d3+1):
        tile=d2v&0xffff; idx=a3-0x0C0000
        if 0<=idx<=len(mem)-4: mem[idx+1]=tile&0xff; mem[idx+3]=((tile>>8)&0x0f)|(d7&0xf0)
        a3+=4; d2v=(d2v+1)&0xffff
      delta=be16(a1+off)
      if delta==0: break
      d3=be16(a1+off+2);off+=4;a2+=delta;a3=a2

def b29e(mem):
    a=0x1D08; mem[a:a+4]=bytes.fromhex('fffaff14')
    for i in range(28): mem[a+4+i*4:a+8+i*4]=bytes.fromhex('fffbff14')
    desc_9aca(mem,0x0C0000+0x1A7C,0x0D,0x10)

def write_group(mem,gid):
    d=R.text_desc(gid); pb=d['palette_slot']
    for e in d['events']:
      if e['kind']!='glyph': continue
      cursor=e['cursor']; typ=d['type']; tiles=e['tiles']
      if typ==0:
        pos=[(cursor,tiles[0])]
      elif typ==1:
        pos=[(cursor,tiles[0]),(cursor+0x100,tiles[1])]
      elif typ==2:
        pos=[(cursor,tiles[0]),(cursor+4,tiles[1]),(cursor+0x100,tiles[2]),(cursor+0x104,tiles[3])]
      else: pos=[]
      for cur,tile in pos:
        idx=cur
        if 0<=idx<=len(mem)-4:
          mem[idx+1]=tile&0xff; mem[idx+3]=((tile>>8)&0x0f)|((pb&0x0f)<<4)

def render_text(mem,cfg=0):
    out=Image.new('RGBA',(320,240),(0,0,0,0)); pals=[R.text_palette(cfg,i) for i in range(16)]
    for ycell in range(30):
      for xcell in range(40):
        mi=(ycell*64+xcell)*4; lo=mem[mi+1]; c=mem[mi+3]; tile=lo|((c&15)<<8); pb=(c>>4)&15
        im=tile_rgba(R.text_tile(tile),pals[pb],8,8,True); out.alpha_composite(im,(xcell*8,ycell*8))
    return out

def text_for_groups(groups):
    mem=init_textmap(); b29e(mem)
    for g in groups:
      if g: write_group(mem,g)
    return render_text(mem,0)

TEXT_TAG_01=text_for_groups([0x2D,0x2C])
TEXT_TAG_23=text_for_groups([0x2F,0x32])
TEXT_R={g:text_for_groups([g]) for g in [0x2C,0x2E,0x30,0x32]}
TEXT_TAG_01.save(OUT/'tag_overlay_pair01.png'); TEXT_TAG_23.save(OUT/'tag_overlay_pair23.png')

def decode_sprite_x(raw_x):
    raw_x &= 0x01ff
    return raw_x-512 if raw_x>496 else raw_x

def decode_sprite_y(raw_y):
    raw_y &= 0x01ff
    return ((256-raw_y)&0x01ff)-16

def render_sprite(canvas,setid,pose,actor_sx,actor_sy,partner=0,mirror=False):
    for piece in R.expanded_pieces(setid,pose,partner,mirror):
      dx=int(piece['rel_x'])
      dy=((int(piece['raw_dy'])+0x80)&0xff)-0x80
      pre_x=int(actor_sx)+0x10+dx
      pre_y=int(actor_sy)+0x90+dy
      if not (0 <= pre_x <= 0x15f and 0 <= pre_y <= 0x187):
          continue
      cmd_x=pre_x-0x10
      cmd_y=pre_y-0x90
      draw_x=decode_sprite_x(cmd_x)
      base_y=decode_sprite_y(cmd_y)
      chain=int(piece['chain']); count=int(piece['chain_index'])
      draw_y=(base_y-16*chain+16*count) if bool(piece['flip_y']) else (base_y-16*count)
      pal=R.resource_palette(piece['palette_resource'],effective=True)
      im=tile_rgba(R.sprite_tile(piece['tile_code']),pal,16,16,True,bool(piece['flip_x']),bool(piece['flip_y']))
      canvas.alpha_composite(im,(draw_x,draw_y))

def s16(v):
    v &= 0xffff
    return v-0x10000 if v&0x8000 else v

def actor_screen_y_raw(world_y,z=0x100,yoff=0):
    return s16(s16(world_y)+s16(z)-s16(0x0200)+int(yoff))

def sprite_raster_y_from_actor_screen(actor_sy):
    return decode_sprite_y(int(actor_sy)&0x01ff)

def screen_xy(world_x,world_y,z=0x100,xoff=0,yoff=0,facing=0):
    xo=-xoff if (facing & 0x8000) else xoff
    actor_sx=s16(s16(world_x)-s16(0x0140)+int(xo))
    actor_sy=actor_screen_y_raw(world_y,z,yoff)
    return actor_sx,sprite_raster_y_from_actor_screen(actor_sy)

def entry_y_offset(identity,pose):
    return 0x30 if identity==1 and pose>=4 else 0

def sample_frame(gfxs,world_y,poses,overlay,tag=True,follower_pose=0):
    f=BASE.copy()
    xs=[0x1C0,0x1F0] if tag else [0x1D8]*len(gfxs)
    for ident,x,pose in zip(gfxs,xs,poses):
      sx=s16(s16(x)-s16(0x0140)); sy=actor_screen_y_raw(world_y,0x100,entry_y_offset(ident,pose))
      render_sprite(f,0x40+ident if 0x40+ident!=0x4b else 0x4a,pose,sx,sy)
    sx=s16(s16(0x1D8)-s16(0x0140)); sy=actor_screen_y_raw((world_y+1)&0xffff)
    render_sprite(f,0x4C,follower_pose,sx,sy)
    f.alpha_composite(overlay)
    return f

for name,gfxs,ov in [('tag01',[1,0],TEXT_TAG_01),('tag23',[2,3],TEXT_TAG_23)]:
    for y in [0x130,0x110,0x0F0,0x0B0,0x090]:
      sample_frame(gfxs,y,[0,0],ov,True).save(OUT/f'{name}_y{y:03x}.png')
print('built',OUT)

MASK_RGB=rgb(R.text_palette(0,15)[1])[:3]

def apply_mask_rows(frame,solid_rows):
    if solid_rows<=0: return frame.copy()
    out=frame.copy(); q=Image.new('RGBA',out.size,(0,0,0,0)); d=q.load()
    for y in range(240):
        if (y & 7) >= 8-solid_rows:
            for x in range(320): d[x,y]=(*MASK_RGB,255)
    out.alpha_composite(q); return out

def follower_step(state):
    draw=0 if state['phase']==0 else 0xffff
    state['counter']+=1
    threshold=2 if state['phase']==0 else 1
    if state['counter']>threshold:
        state['counter']=0
        state['phase'] ^= 1
    return draw,state['counter'],state['phase']

def actor_pose_for_update(n):
    if n<1: return 0
    return ((n-1)//12)%4

def actor_y_fixed(n):
    return (0x01300000 - n*0x00008000) & 0xffffffff

def frame_for_segment(gfxs,n,overlay,tag,follower_draw):
    yfix=actor_y_fixed(n); yword=(yfix>>16)&0xffff
    poses=[actor_pose_for_update(n) for _ in gfxs]
    f=BASE.copy(); xs=([0x1c0,0x1f0] if tag else [0x1d8]*len(gfxs))
    for ident,x,pose in zip(gfxs,xs,poses):
        sx=s16(s16(x)-s16(0x0140)); sy=actor_screen_y_raw(yword,0x100,entry_y_offset(ident,pose))
        render_sprite(f,0x40+ident if 0x40+ident!=0x4b else 0x4a,pose,sx,sy)
    if follower_draw!=0xffff:
        sx=s16(s16(0x1d8)-s16(0x0140)); sy=actor_screen_y_raw((yword+1)&0xffff)
        render_sprite(f,0x4c,0,sx,sy)
    f.alpha_composite(overlay)
    return f,yfix,yword,poses

def save_apng(path,frames,durations):
    frames[0].save(path,format='PNG',save_all=True,append_images=frames[1:],
                   duration=durations,loop=0,disposal=0,blend=0,
                   optimize=False,compress_level=1)

def build_trace(name,segments):
    frames=[]; durations=[]; rows=[]; preview_unit_ms=15
    seq_index=0
    follower={'counter':0,'phase':0}
    for si,(gfxs,overlay,label,tag,golden_prefix) in enumerate(segments):
        for solid in range(8,0,-1):
            fr=apply_mask_rows(BASE,solid); frames.append(fr); durations.append(3*preview_unit_ms)
            rows.append([seq_index,si,label,'reveal',solid,'',3,'','','','',
                         follower['counter'],follower['phase']]); seq_index+=1
        last=None
        for n in range(1,321):
            fd,fc,fp=follower_step(follower)
            fr,yfix,yword,poses=frame_for_segment(gfxs,n,overlay,tag,fd)
            last=fr
            if golden_prefix and n in (1,160,320):
                fr.save(GOLDEN/f'{golden_prefix}-U{n:03d}.png')
            frames.append(fr); durations.append(preview_unit_ms)
            rows.append([seq_index,si,label,'walk','',n,1,f'0x{yfix:08X}',f'0x{yword:04X}',
                         '/'.join(map(str,poses)),f'0x{fd:04X}',fc,fp]); seq_index+=1
        terminal_fix=actor_y_fixed(321)
        terminal_word=(terminal_fix>>16)&0xffff
        for j,solid in enumerate(range(1,9)):
            fr=apply_mask_rows(last,solid)
            if tag and si==0 and solid in (1,4,8):
                fr.save(GOLDEN/f'TAG-CLOSE-MASK-{solid}.png')
            frames.append(fr); durations.append(3*preview_unit_ms)
            rows.append([seq_index,si,label,'close',solid,321 if j==0 else '',3,
                         f'0x{terminal_fix:08X}' if j==0 else '',f'0x{terminal_word:04X}' if j==0 else '',
                         '', '',follower['counter'],follower['phase']]); seq_index+=1
        full=apply_mask_rows(last,8); frames.append(full); durations.append(33*preview_unit_ms)
        rows.append([seq_index,si,label,'close_hold',8,'',33,'','','','',
                     follower['counter'],follower['phase']]); seq_index+=1
        fd,fc,fp=follower_step(follower)
        frames.append(full.copy()); durations.append(preview_unit_ms)
        rows.append([seq_index,si,label,'post_close_outer_sync',8,'',1,'','','',f'0x{fd:04X}',fc,fp]); seq_index+=1

    apng=OUT/f'{name}.apng'
    save_apng(apng,frames,durations)
    csvp=OUT/f'{name}-TRACE.csv'
    with csvp.open('w',newline='') as fh:
        w=csv.writer(fh)
        w.writerow(['preview_frame','segment','label','phase','mask_solid_rows',
                    'actor_update_event','represented_rom_quanta','actor_y_fixed_16_16',
                    'actor_y_word','actor_pose_selector','follower_selector',
                    'follower_counter_after','follower_phase_after'])
        w.writerows(rows)
    return apng,csvp

TAG_APNG,TAG_CSV=build_trace('WP12-R04-TAG-FIRST-MATCH-ROM-EXACT',[
    ([1,0],TEXT_TAG_01,'TAG pair 1: input ids 0/1 -> normalized ids 1/0',True,'TAG-PAIR01'),
    ([2,3],TEXT_TAG_23,'TAG pair 2: ids 2/3',True,'TAG-PAIR23'),
])
ROY_APNG,ROY_CSV=build_trace('WP12-R04-ROYAL-FIRST-MATCH-ROM-EXACT',[
    ([0],TEXT_R[0x2C],'ROYAL actor runtime id 0',False,'ROYAL-ID0'),
    ([1],TEXT_R[0x2E],'ROYAL actor runtime id 1',False,None),
    ([2],TEXT_R[0x30],'ROYAL actor runtime id 2',False,None),
    ([3],TEXT_R[0x32],'ROYAL actor runtime id 3',False,None),
])
for f in [TAG_APNG,TAG_CSV,ROY_APNG,ROY_CSV]: print(f.name,hashlib.sha256(Path(f).read_bytes()).hexdigest(),Path(f).stat().st_size)
