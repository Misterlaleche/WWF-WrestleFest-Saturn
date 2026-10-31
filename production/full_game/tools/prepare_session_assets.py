#!/usr/bin/env python3
"""Compile resident live video assets from verified ROM bitplanes, no captures."""
import argparse,hashlib,importlib.util,json,struct,sys,zipfile,zlib
from pathlib import Path
from recover_graphics import Graphics,pack_vdp2_pattern,pack_4bpp
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--header',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[3];g=Graphics(a.rom)
module=root/'Escena Presentadora/source/research_render/workpack_25/tools/wf25_graphics.py'
spec=importlib.util.spec_from_file_location('original_graphics',module);original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
r=original.Rom(a.rom);ids=set()
for s,offset,base,count in r.active_sets():
 if not 13<=s<=44:continue
 for pose in range(count):
  if r.pose_entry(s,pose) in (None,0xfffe,0xffff):continue
  for dx,dy,attr,res,hi,lo in r.pose_rows(s,pose,0):
   chain=(attr>>5)&7;code=((hi<<8)|lo)&~((1<<chain.bit_length())-1);ids.update(range(code,code+chain+1))
ids=sorted(ids);assert len(ids)*128+1028*32<=0x80000
assets={'PROGRAM.BIN':r.p,'BACK.TEX':b''.join(pack_vdp2_pattern(g.tile('background',i)) for i in range(4096)),
 'FONT.TEX':b''.join(pack_vdp2_pattern(g.tile('text',i)) for i in range(4096)),
 'FRONT.TEX':b''.join(pack_4bpp(g.tile('sprite',i)) for i in ids)}
a.out.mkdir(parents=True,exist_ok=True);a.header.parent.mkdir(parents=True,exist_ok=True)
report={'program_sha1':g.program_sha1,'source_decoder':str(module.relative_to(root)),'source_decoder_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),'scope':'Immutable ROM texture atlas for sets13..44; live maps and sprite commands supplied by wf_session','sprite_ids':ids,'assets':{}}
header=['/* Generated from verified ROM. No state/animation sequence. */','#include <stdint.h>','static const uint16_t wf_front_sprite_ids[]={'+','.join(map(str,ids))+'};','#define WF_FRONT_SPRITE_BYTES '+str(len(assets['FRONT.TEX']))+'u']
for name,data in assets.items():
 (a.out/name).write_bytes(data);crc=zlib.crc32(data);report['assets'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'crc32':crc}
 header.append('#define WF_'+name.split('.')[0]+'_CRC 0x%08xu'%crc)
a.header.write_text('\n'.join(header)+'\n');(a.out/'asset-provenance.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:len(v) for k,v in assets.items()}))
