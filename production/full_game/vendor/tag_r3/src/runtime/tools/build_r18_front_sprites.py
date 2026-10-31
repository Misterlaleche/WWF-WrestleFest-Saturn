#!/usr/bin/env python3
"""Exact ROM sprite working set for boot/title/roster/match presentations."""
import argparse,hashlib,json,re
from pathlib import Path
from wf_r18_rom_graphics import Rom
from build_r18_hot_sprites import build_pack,PACK_SHA256,INDEX_BYTES,MEMORY_CONTRACT

def build(rom_path,source_path,out,manifest,profile='frontend'):
 r=Rom(rom_path);assert hashlib.sha1(r.p).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
 source=source_path.read_bytes();assert hashlib.sha256(source).hexdigest()==PACK_SHA256
 ids=set();poses=[]
 for graphic,_,_,count in r.active_sets():
  complete=(12<=graphic<=45 or 0x40<=graphic<=0x4c or graphic==0x4f)if profile=='frontend'else(graphic==0x2c or 0x30<=graphic<=0x3b or 0x40<=graphic<=0x4f)
  selected=range(count)if complete else[p for p in (0,89,90,91,92,93)if p<count]if graphic<12 else()
  for pose in selected:
   if r.pose_entry(graphic,pose)in(None,0xfffe,0xffff):continue
   for partner in range(12):
    for dx,dy,attr,res,hi,lo in r.pose_rows(graphic,pose,partner):
     chain=(attr>>5)&7;tile=((hi<<8)|lo)&~((1<<chain.bit_length())-1)
     ids.update(range(tile,tile+chain+1))
   poses.append([graphic,pose])
 ids=sorted(ids);blob=build_pack(ids,source,'dictionary',4)
 chunks=int.from_bytes(blob[8:12],'big');maps=int.from_bytes(blob[12:16],'big');dictionary=int.from_bytes(blob[16:20],'big')
 resident=(max(0,(maps+31)//32*32-INDEX_BYTES)+dictionary+127)//128*128
 capacity=int(re.search(r'#define WF_SATURN_LWRAM_FRONT_TILE_BYTES\s+(0x[0-9a-f]+)u',MEMORY_CONTRACT)[1],16)
 assert resident+256*128<=capacity,('frontend sprite lease overflow',resident,capacity)
 out.write_bytes(blob)
 scope='All poses of graphicsets12..45,64..76,79'if profile=='frontend'else'All poses of presenter44, continue portraits48..59 and post-TAG graphicsets64..79'
 result={'rom_sha1':hashlib.sha1(r.p).hexdigest(),'sprite_pack_sha256':PACK_SHA256,'format':'R18D4','profile':profile,'tile_count':len(ids),'dictionary_chunks':chunks,'resident_bytes':resident,'cache_capacity':capacity,'cold_tile_slots':(capacity-resident)//128,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest(),'poses':poses,'tile_ids':ids,'scope':scope+'; fighter0..11 poses0,89..93. Immutable preload only; does not choose animations or alter states.'}
 manifest.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items()if k not in('poses','tile_ids')},indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--profile',choices=('frontend','post-tag'),default='frontend');a=p.parse_args();build(a.rom,a.source,a.out,a.manifest,a.profile)
