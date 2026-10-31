#!/usr/bin/env python3
"""Compile resident live video assets from verified ROM bitplanes, no captures."""
import argparse,hashlib,importlib.util,json,struct,sys,zipfile,zlib
from pathlib import Path
from recover_graphics import Graphics,pack_vdp2_pattern,pack_4bpp
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--header',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[3];g=Graphics(a.rom)
module=root/'Escena Presentadora/source/research_render/workpack_25/tools/wf25_graphics.py'
spec=importlib.util.spec_from_file_location('original_graphics',module);original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
r=original.Rom(a.rom)
scenes={'front':{},'entry':{},'ring':{}}
for s,offset,base,count in r.active_sets():
 if 13<=s<=44:scenes['front'][s]=range(count)
 if 0x40<=s<=0x4a:scenes['entry'][s]=range(min(8,count))
 if s==0x4c:scenes['entry'][s]=[0]
 if s<12:scenes['ring'][s]=[0,12,89,90,91,92,93]
 if s in (12,31):scenes['ring'][s]=range(min(32,count))
atlases={}
for name,sets in scenes.items():
 ids=set()
 for s,poses in sets.items():
  for pose in poses:
   if r.pose_entry(s,pose) in (None,0xfffe,0xffff):continue
   for dx,dy,attr,res,hi,lo in r.pose_rows(s,pose,0):
    chain=(attr>>5)&7;code=((hi<<8)|lo)&~((1<<chain.bit_length())-1);ids.update(range(code,code+chain+1))
 ids=sorted(ids);assert len(ids)*128+1028*32<=0x80000
 atlases[name]=ids
assets={'PROGRAM.BIN':r.p,'BACK.TEX':b''.join(pack_vdp2_pattern(g.tile('background',i)) for i in range(4096)),
 'FONT.TEX':b''.join(pack_vdp2_pattern(g.tile('text',i)) for i in range(4096))}
for name,ids in atlases.items():assets[name.upper()+'.TEX']=b''.join(pack_4bpp(g.tile('sprite',i)) for i in ids)
a.out.mkdir(parents=True,exist_ok=True);a.header.parent.mkdir(parents=True,exist_ok=True)
report={'program_sha1':g.program_sha1,'source_decoder':str(module.relative_to(root)),'source_decoder_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),'scope':'ROM texture atlases by owner family; live original maps, records, palettes and timing','sprite_ids':atlases['front'],'atlases':atlases,'assets':{}}
header=['/* Generated from verified ROM. No state/animation sequence. */','#include <stdint.h>']
for name,ids in atlases.items():
 header+=['static const uint16_t wf_'+name+'_sprite_ids[]={'+','.join(map(str,ids))+'};','#define WF_'+name.upper()+'_SPRITE_BYTES '+str(len(ids)*128)+'u']
for name,data in assets.items():
 (a.out/name).write_bytes(data);crc=zlib.crc32(data);report['assets'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'crc32':crc}
 header.append('#define WF_'+name.split('.')[0]+'_CRC 0x%08xu'%crc)
a.header.write_text('\n'.join(header)+'\n');(a.out/'asset-provenance.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:len(v) for k,v in assets.items()}))
