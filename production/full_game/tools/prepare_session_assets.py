#!/usr/bin/env python3
"""Compile resident live video assets from verified ROM bitplanes, no captures."""
import argparse,hashlib,importlib.util,json,struct,sys,zipfile,zlib
from pathlib import Path
from recover_graphics import Graphics,pack_vdp2_pattern,pack_4bpp
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--header',type=Path,required=True)
p.add_argument('--alpha-post',action='store_true',help='Add original POST scenes and bounded final-picture checksum data; requires new output/header paths')
p.add_argument('--sprite-pack',type=Path,help='Approved unchanged SPRITES.PAK for the final-picture supplement')
a=p.parse_args()
if a.alpha_post!=bool(a.sprite_pack):p.error('--alpha-post and --sprite-pack are required together')
if a.alpha_post and (a.out.exists() or a.header.exists()):p.error('Alpha output directory and header must be new; approved outputs are never replaced')
post_families=[0x2c,*range(0x30,0x3c),0x4d,0x4e,0x4f]
sprite_pixels=None
if a.alpha_post:
 approved=json.loads((Path(__file__).resolve().parents[1]/'evidence/session08/combat-provenance.json').read_text())['SPRITES.PAK']
 sprite_pixels=a.sprite_pack.read_bytes()
 if len(sprite_pixels)!=approved['bytes'] or hashlib.sha256(sprite_pixels).hexdigest()!=approved['sha256']:
  p.error('SPRITES.PAK does not match the approved canonical resource')
root=Path(__file__).resolve().parents[3];g=Graphics(a.rom)
module=root/'Escena Presentadora/source/research_render/workpack_25/tools/wf25_graphics.py'
spec=importlib.util.spec_from_file_location('original_graphics',module);original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
r=original.Rom(a.rom)
scenes={'front':{},'entry':{},'ring':{}}
if a.alpha_post:scenes['post']={}
for s,offset,base,count in r.active_sets():
 if 13<=s<=44:scenes['front'][s]=range(count)
 if 0x40<=s<=0x4a:scenes['entry'][s]=range(min(8,count))
 if s==0x4c:scenes['entry'][s]=[0]
 if s<12:scenes['ring'][s]=[0,12,89,90,91,92,93]
 if s in (12,31):scenes['ring'][s]=range(min(32,count))
 if a.alpha_post and s in post_families:scenes['post'][s]=range(count)
def sprite_ids(sets):
 ids=set()
 for s,poses in sets.items():
  for pose in poses:
   if r.pose_entry(s,pose) in (None,0xfffe,0xffff):continue
   for dx,dy,attr,res,hi,lo in r.pose_rows(s,pose,0):
    chain=(attr>>5)&7;code=((hi<<8)|lo)&~((1<<chain.bit_length())-1);ids.update(range(code,code+chain+1))
 return sorted(ids)
atlases={name:sprite_ids(sets) for name,sets in scenes.items()}
for ids in atlases.values():assert len(ids)*128+1028*32<=0x80000
# Credits borrow the existing bounded supplement when world1BA6 starts.
# Keep their demand in the header; adding another CD file or enlarging POST
# would exceed the directory and VDP1 budgets respectively.
finale=None
if a.alpha_post:
 finale={'families':[0x50],'poses':list(range(6)),
  'sprite_ids':sprite_ids({0x50:range(6)}),'storage':'existing POST supplement'}
 if len(finale['sprite_ids'])!=257:raise ValueError('Unexpected canonical credits demand')
assets={'PROGRAM.BIN':r.p,'BACK.TEX':b''.join(pack_vdp2_pattern(g.tile('background',i)) for i in range(4096)),
 'FONT.TEX':b''.join(pack_vdp2_pattern(g.tile('text',i)) for i in range(4096))}
for name,ids in atlases.items():assets[name.upper()+'.TEX']=b''.join(pack_4bpp(g.tile('sprite',i)) for i in ids)
post=None
if a.alpha_post:
 ids=atlases['post'];texture=assets['POST.TEX']
 if len(ids)!=3250 or set(scenes['post'])!=set(post_families):raise ValueError('Unexpected canonical POST families')
 for slot,code in enumerate(ids):
  if texture[slot*128:(slot+1)*128]!=sprite_pixels[code*128:(code+1)*128]:raise ValueError('POST/SPRITES.PAK pixel mismatch')
 checksums=b''.join(struct.pack('>I',zlib.crc32(sprite_pixels[i:i+128])) for i in range(0,len(sprite_pixels),128))
 assets['POST.TEX']=texture+checksums
 post={'families':post_families,'poses':{str(s):list(poses) for s,poses in scenes['post'].items()},
  'texture_bytes':len(texture),'texture_crc32':zlib.crc32(texture),'tile_crc_offset':len(texture),
  'tile_crc_bytes':len(checksums),'tile_crc32':zlib.crc32(checksums),'supplement_tiles':512,
  'max_vdp1_bytes':1028*32+(len(ids)+512)*128,'sprite_pack':approved,
  'format':'16x16 packed4bpp texture prefix; then65536 big-endian CRC32 values indexed by canonical sprite tile ID'}
 if post['max_vdp1_bytes']>0x80000:raise ValueError('POST final picture exceeds VDP1 memory')
a.out.mkdir(parents=True,exist_ok=True);a.header.parent.mkdir(parents=True,exist_ok=True)
report={'program_sha1':g.program_sha1,'source_decoder':str(module.relative_to(root)),'source_decoder_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),'scope':'ROM texture atlases by owner family; live original maps, records, palettes and timing','sprite_ids':atlases['front'],'atlases':atlases,'assets':{}}
if post:report['post']=post
if finale:report['finale']=finale
header=['/* Generated from verified ROM. No state/animation sequence. */','#include <stdint.h>']
for name,ids in atlases.items():
 header+=['static const uint16_t wf_'+name+'_sprite_ids[]={'+','.join(map(str,ids))+'};','#define WF_'+name.upper()+'_SPRITE_BYTES '+str(len(ids)*128)+'u']
if post:
 header+=['#define WF_POST_FILE_BYTES '+str(len(assets['POST.TEX']))+'u',
  '#define WF_POST_TEXTURE_CRC 0x%08xu'%post['texture_crc32'],
  '#define WF_POST_TILE_CRC 0x%08xu'%post['tile_crc32']]
 header+=['static const uint16_t wf_finale_sprite_ids[]={'+','.join(map(str,finale['sprite_ids']))+'};',
  '#define WF_FINALE_SPRITE_COUNT '+str(len(finale['sprite_ids']))+'u']
for name,data in assets.items():
 (a.out/name).write_bytes(data);crc=zlib.crc32(data);report['assets'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'crc32':crc}
 header.append('#define WF_'+name.split('.')[0]+'_CRC 0x%08xu'%crc)
a.header.write_text('\n'.join(header)+'\n');(a.out/'asset-provenance.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:len(v) for k,v in assets.items()}))
