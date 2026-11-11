#!/usr/bin/env python3
"""Adapt the recovered TAG resident format to an explicitly declared lineup.
The supplied original codec/crop tools are read only; all output is separate.
"""
import argparse,collections,hashlib,importlib.util,json,struct,zlib
from pathlib import Path
from tag_resident_geometry import GEOMETRIES
def module(path,name):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--tools',type=Path,required=True);p.add_argument('--cd',type=Path,required=True)
 p.add_argument('--catalog',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 p.add_argument('--missing',type=Path,help='Additional exact resource demands from the complete native route')
 p.add_argument('--prefer-observed',action='store_true',help='Prioritize observed demand in the uncompressed primary lease; all other catalog poses remain secondary')
 p.add_argument('--geometry',choices=tuple(GEOMETRIES),default='legacy16384',help='Exact secondary count and primary memory lease of the target TAG module')
 a=p.parse_args()
 tiles,block_count,resident,workspace,input_bytes,primary_bytes=GEOMETRIES[a.geometry]
 codec=module(a.tools/'build_secondary_asset.py','tag_codec');crop=module(a.tools/'build_crop_asset.py','tag_crop')
 catalog=json.loads(a.catalog.read_text());all_ids=set(catalog['ids']);fighters=tuple(catalog['fighters'])
 rom=(a.cd/'ROM68K.BIN').read_bytes();sprites=(a.cd/'SPRITES.PAK').read_bytes();old=(a.cd/'HOTSPR.BIN').read_bytes()
 assert codec.sha256(rom)==codec.ROM_SHA256 and codec.sha256(sprites)==codec.SPRITES_SHA256
 assert len(fighters)==4 and len(all_ids)<=6474+tiles
 codec.FIGHTERS=fighters;poses=codec.Poses(rom);verified,_=poses.catalog();assert verified==all_ids
 if a.missing:
  trace=json.loads(a.missing.read_text());assert not trace['overflow'] and not trace.get('other',0)
  all_ids.update(trace['tiles'])
 assert len(all_ids)<=6474+tiles and all(0<=i<len(sprites)//128 for i in all_ids)
 count=struct.unpack_from('>H',old,4)[0];offset=8 if old[:4]==b'R18H' else 24
 old_ids=set(struct.unpack_from('>'+str(count)+'H',old,offset));frequency=collections.Counter()
 for graphic in fighters+codec.COMMON:
  _,base,total=poses.sets[graphic]
  for pose in range(total):
   address=poses.entry(graphic,pose)
   if address is None:continue
   for partner in fighters:
    for _,_,attr,_,high,low in poses.rows(address,base,partner):
     chain=(attr>>5)&7;tile=((high<<8)|low)&~((1<<chain.bit_length())-1)
     frequency.update(range(tile,tile+chain+1))
 if a.prefer_observed:
  assert a.missing,'Observed-primary selection needs a complete trace'
  demanded=set(trace['tiles']);primary=set()
  costs={}
  for tile in all_ids:
   hint=crop.metadata(sprites[tile*128:(tile+1)*128]);costs[tile]=(hint&255)*(4 if hint&0x2000 else 8)
  capacity=primary_bytes-8192-128;used=0
  for tile in sorted(all_ids,key=lambda x:(x not in demanded,-frequency[x],costs[x],x)):
   if len(primary)==6474:break
   if used+costs[tile]<=capacity:primary.add(tile);used+=costs[tile]
 else:
  primary=set(old_ids&all_ids)
  for tile in sorted(all_ids-primary,key=lambda x:(-frequency[x],x)):
   if len(primary)==6474:break
   primary.add(tile)
 assert len(primary)==6474
 ids=sorted(primary);index=bytearray();payload=bytearray();descriptors=[]
 for tile in ids:
  raw=sprites[tile*128:(tile+1)*128];descriptor=crop.metadata(raw);descriptors.append(descriptor)
  meta,height=descriptor>>8,descriptor&255;top=meta&31;columns=4 if meta&32 else 8;column=4 if meta&64 else 0
  packed=b''.join(raw[(top+y)*8+column:(top+y)*8+column+columns] for y in range(height))
  check=bytearray(128)
  for y in range(height):check[(top+y)*8+column:(top+y)*8+column+columns]=packed[y*columns:(y+1)*columns]
  assert check==raw
  index+=struct.pack('>IHH',len(payload),descriptor,0);payload+=packed
 assert len(payload)+8192+128<=primary_bytes,('TAG primary/tail budget',len(payload))
 header=bytearray(16384);struct.pack_into('>4sHH4I',header,0,b'R18P',6474,128,8,len(index),len(payload),len(header))
 id_bytes=struct.pack('>6474H',*ids);header[24:24+len(id_bytes)]=id_bytes;index+=bytes((-len(index))&31)
 hot=bytes(header+index+payload);desc=struct.pack('>6474H',*descriptors)
 crop_blob=struct.pack('>4sHHII',b'R18C',1,6474,codec.fnv(id_bytes),codec.fnv(desc))+desc
 secondary=all_ids-primary
 # Pad the fixed geometry with exact original sparse tiles. They are not
 # requested by the catalog; favoring compressible padding avoids wasting
 # the secondary pool on unrelated fighter poses. No pixel is synthesized.
 padding=sorted(set(range(len(sprites)//128))-primary-secondary,
                key=lambda i:(sum(v!=0 for v in sprites[i*128:(i+1)*128]),i))
 for tile in padding:
  if len(secondary)==tiles:break
  if tile not in primary:secondary.add(tile)
 assert len(secondary)==tiles and all_ids<=primary|secondary and not primary&secondary
 secondary=sorted(secondary);raw=b''.join(sprites[i*128:(i+1)*128] for i in secondary)
 blocks=[raw[i:i+1024] for i in range(0,len(raw),1024)];tokens=[];ll=[0]*286;dd=[0]*30
 for block in blocks:
  c=zlib.compressobj(9,zlib.DEFLATED,-15);t,check=codec.tokenize(c.compress(block)+c.flush());assert check==block;tokens.append(t)
  for value,length,distance,_ in t:
   ll[value]+=1
   if length:dd[distance]+=1
 literal,distance=codec.model(ll,10),codec.model(dd,7);packed=[];offsets=[0];hashes=[]
 for block,t in zip(blocks,tokens):
  encoded=codec.encode(t,literal,distance);assert codec.decode(encoded,len(block),literal,distance)==block
  packed.append(encoded);offsets.append(offsets[-1]+len(encoded));hashes.append(codec.fnv(block))
 # Capacity constants match the selected target module. The original
 # codec and VDP/CD ownership rules remain unchanged.
 assert max(map(len,packed))<=input_bytes,('secondary input capacity',max(map(len,packed)))
 data=b''.join(packed);assert len(data)<=resident+8192+312960+192*2352,('resident pool capacity',len(data))
 bitmap=bytearray(8192)
 for i in secondary:bitmap[i>>3]|=1<<(i&7)
 prefix=[];total=0
 for i in range(256):prefix.append(total);total+=sum(bin(x).count('1') for x in bitmap[i*32:(i+1)*32])
 assert total==tiles
 metadata=bytes(bitmap)+struct.pack('>256H',*prefix)+struct.pack('>'+str(block_count+1)+'I',*offsets)+struct.pack('>'+str(block_count)+'I',*hashes)+struct.pack('>316H',*(literal[0]+distance[0]))
 secondary_blob=struct.pack('>4sHH6I',b'R18S',1,3,tiles,block_count,len(data),len(metadata),codec.fnv(metadata),codec.fnv(data))+metadata+data
 a.out.mkdir(exist_ok=True,parents=True)
 outputs={'HOTSPR.BIN':hot,'CROP.BIN':crop_blob,'TAGEX.BIN':secondary_blob}
 for name,blob in outputs.items():(a.out/name).write_bytes(blob)
 report=dict(geometry=a.geometry,runtime_tiles=tiles,primary_pool_bytes=primary_bytes,fighters=fighters,common=codec.COMMON,catalog_tiles=len(all_ids),primary_payload_bytes=len(payload),secondary_payload_bytes=len(data),secondary_workspace_bytes=workspace,primary_ids=ids,secondary_ids=secondary,all_tiles_byte_verified=True,source_sha256={str(x):codec.sha256(x.read_bytes()) for x in (a.tools/'build_secondary_asset.py',a.tools/'build_crop_asset.py',a.catalog)},output_sha256={n:codec.sha256(v) for n,v in outputs.items()},scope='Declared lineup only; original secondary codec with a larger resident lease, exact pixels, no gameplay changes')
 if a.missing:report['source_sha256'][str(a.missing)]=codec.sha256(a.missing.read_bytes())
 report['observed_primary_tiles']=len(primary&set(trace['tiles'])) if a.missing else None
 report['prefer_observed']=a.prefer_observed
 (a.out/'resident.json').write_text(json.dumps(report,indent=2)+'\n');print({k:report[k] for k in ('fighters','catalog_tiles','primary_payload_bytes','secondary_payload_bytes','all_tiles_byte_verified')})
if __name__=='__main__':main()
