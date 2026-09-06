#!/usr/bin/env python3
"""Build ROM-exact large biography portraits (sets $30-$3B) for Test255 attract mode."""
from pathlib import Path
import hashlib,json,struct,sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
import arcade_composition as comp
import lz4_codec
ROOT=Path(__file__).resolve().parents[1]
PROGRAM=ROOT/'program.bin'; GFX=ROOT/'gfx2.bin'
PROGRAM_SHA256='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
GFX_SHA256='a3b5e1fedd6c41cd398ba0cb437cddb2f50b3ae0a0dc178a10b70a7ed5d010ba'
PALETTE_TABLE=0x2F22
CHARACTERS=12; POSES=3

def fnv(data):
 v=2166136261
 for x in data:v=((v^x)*16777619)&0xffffffff
 return v

def rgb444_to_rgb555(v):
 r=v&15;g=(v>>4)&15;b=(v>>8)&15
 r=(r<<1)|(r>>3);g=(g<<1)|(g>>3);b=(b<<1)|(b>>3)
 return 0x8000|(b<<10)|(g<<5)|r

def pack(frames,textures,bt=32):
 indexes=[];rec=[]
 for f in frames:indexes.append((len(rec),len(f)));rec+=f
 body=b''.join(struct.pack('>HH',*x) for x in indexes)+b''.join(struct.pack('>HhhH',*x) for x in rec)
 off=(32+len(body)+31)&~31;raw=b''.join(textures);chunks=[]
 for first in range(0,len(textures),bt):
  u=b''.join(textures[first:first+bt]);z=lz4_codec.compress(u);assert lz4_codec.decompress(z,len(u))==u;chunks.append((u,z))
 pos=off+len(chunks)*12;directory=[];payload=[]
 for u,z in chunks:directory.append(struct.pack('>III',pos,len(z),len(u)));payload.append(z);pos+=len(z)
 return struct.pack('>8I',0x57464332,len(frames),len(rec),len(textures),off,fnv(raw),bt,len(chunks))+body+bytes(off-32-len(body))+b''.join(directory)+b''.join(payload)

def tile_decoder(g):
 region=np.frombuffer(g,np.uint8).reshape(4,65536,32);cache={}
 def tile(code):
  if code not in cache:
   bits=np.unpackbits(region[:,code,:],axis=1).reshape(4,2,16,8).transpose(0,2,1,3).reshape(4,16,16)
   px=np.sum(bits*np.array([8,4,2,1],np.uint8)[:,None,None],axis=0,dtype=np.uint8)
   cache[code]=((px[:,::2]<<4)|px[:,1::2]).tobytes()
  return cache[code]
 return tile

def main():
 p=PROGRAM.read_bytes();g=GFX.read_bytes();assert hashlib.sha256(p).hexdigest()==PROGRAM_SHA256;assert hashlib.sha256(g).hexdigest()==GFX_SHA256
 comp.load_program(PROGRAM);tile=tile_decoder(g);textures=[];lookup={};frames=[];details=[]
 for ch in range(CHARACTERS):
  setid=0x30+ch
  for pose in range(POSES):
   pieces=comp.commands(setid,pose,partner=0,x=160,y=96);assert pieces,(setid,pose)
   banks={q['bank'] for q in pieces};assert banks=={0x10+ch,0x1e},(setid,pose,banks)
   rec=[]
   for q in pieces:
    px=tile(q['code'])
    if not any(px):continue
    if px not in lookup:lookup[px]=len(textures);textures.append(px)
    rec.append([lookup[px],q['x']-160,q['y']-136,int(q['fx'])|(int(q['fy'])<<1)])
   assert rec;frames.append(rec);details.append(dict(character=ch,set=f'0x{setid:02x}',pose=pose,pieces=len(rec),banks=sorted(banks)))
 data=pack(frames,textures);(ROOT/'cd'/'ATTRPRT.PAK').write_bytes(data)
 pals=[]
 for bank in list(range(0x10,0x1c))+[0x1e]:
  vals=[int.from_bytes(p[PALETTE_TABLE+bank*32+i*2:PALETTE_TABLE+bank*32+i*2+2],'big') for i in range(16)]
  pals.extend(rgb444_to_rgb555(v) for v in vals)
 paldata=b''.join(v.to_bytes(2,'big') for v in pals);(ROOT/'assets'/'ATTRPRT.PAL').write_bytes(paldata)
 h=['#ifndef WWF_ATTRACT_PORTRAIT_ASSETS_H','#define WWF_ATTRACT_PORTRAIT_ASSETS_H',f'#define ATTRACT_PORTRAIT_CHARACTERS {CHARACTERS}U',f'#define ATTRACT_PORTRAIT_POSES {POSES}U',f'#define ATTRACT_PORTRAIT_FRAMES {len(frames)}U',f'#define ATTRACT_PORTRAIT_BYTES {len(data)}U',f'#define ATTRACT_PORTRAIT_HASH 0x{fnv(data):08x}U',f'#define ATTRACT_PORTRAIT_PALETTE_BYTES {len(paldata)}U','static inline unsigned attract_portrait_frame(unsigned ch,unsigned pose){return ch*3U+pose;}','static inline unsigned attract_portrait_set(unsigned ch){return 0x30U+ch;}','static inline unsigned attract_portrait_character_palette(unsigned ch){return 0x10U+ch;}','#define ATTRACT_PORTRAIT_COMMON_PALETTE 0x1eU','#endif']
 (ROOT/'attract_portrait_assets.h').write_text('\n'.join(h)+'\n')
 report=dict(sets='0x30-0x3b',characters=CHARACTERS,poses_per_character=POSES,frames=len(frames),pack_bytes=len(data),pack_fnv=f'{fnv(data):08x}',pack_sha256=hashlib.sha256(data).hexdigest(),palette_banks=[f'0x{x:02x}' for x in list(range(0x10,0x1c))+[0x1e]],palette_bytes=len(paldata),palette_sha256=hashlib.sha256(paldata).hexdigest(),textures=len(textures),records=sum(len(x) for x in frames),details=details)
 (ROOT/'docs'/'test255-attract-portraits.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS ATTRPRT',len(data),len(textures),report['records'],report['pack_sha256'])
if __name__=='__main__':main()
