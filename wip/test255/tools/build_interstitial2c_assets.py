#!/usr/bin/env python3
"""Build ROM-exact gfx-set $2C presentation assets used by attract interstitials.

Requires the user's exact assembled gfx2.bin. The output contains only the 32
frames used by the three $AE20 objects and the three palette resources $31-$33.
"""
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
BANK_BY_FRAME={**{i:0x31 for i in range(0x00,0x0a)},**{i:0x32 for i in range(0x0a,0x16)},0x1f:0x32,**{i:0x33 for i in range(0x16,0x1f)}}

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
 for pose in range(32):
  pieces=comp.commands(0x2c,pose,partner=0,x=160,y=96);assert pieces,(pose,'missing set2c frame')
  expected=BANK_BY_FRAME[pose];banks={q['bank'] for q in pieces};assert banks=={expected},(pose,banks,expected)
  rec=[]
  for q in pieces:
   px=tile(q['code']);
   if not any(px):continue
   if px not in lookup:lookup[px]=len(textures);textures.append(px)
   rec.append([lookup[px],q['x']-160,q['y']-136,int(q['fx'])|(int(q['fy'])<<1)])
  assert rec;frames.append(rec);details.append(dict(frame=pose,palette_bank=expected,pieces=len(rec),codes=[q['code'] for q in pieces]))
 data=pack(frames,textures);(ROOT/'cd'/'INTER2C.PAK').write_bytes(data)
 pals=[]
 for bank in (0x31,0x32,0x33):
  vals=[int.from_bytes(p[PALETTE_TABLE+bank*32+i*2:PALETTE_TABLE+bank*32+i*2+2],'big') for i in range(16)];assert vals[0]==0
  pals.extend(rgb444_to_rgb555(v) for v in vals)
 paldata=b''.join(v.to_bytes(2,'big') for v in pals);(ROOT/'assets'/'INTER2C.PAL').write_bytes(paldata)
 h=['#ifndef WWF_INTERSTITIAL_ASSETS_H','#define WWF_INTERSTITIAL_ASSETS_H','#define INTERSTITIAL2C_FRAMES 32U',f'#define INTERSTITIAL2C_BYTES {len(data)}U',f'#define INTERSTITIAL2C_HASH 0x{fnv(data):08x}U',f'#define INTERSTITIAL2C_PALETTE_BYTES {len(paldata)}U','static inline unsigned interstitial2c_palette_bank(unsigned frame){',' if(frame<=0x09U)return 0x31U;',' if(frame<=0x15U||frame==0x1fU)return 0x32U;',' if(frame<=0x1eU)return 0x33U;',' return 0xffffU;','}','#endif']
 (ROOT/'interstitial_assets.h').write_text('\n'.join(h)+'\n')
 report=dict(gfx_set='0x2c',frames=32,frame_groups={'right_demolition':'0x00-0x09 / bank31','presenter':'0x0a-0x15,0x1f / bank32','left_demolition':'0x16-0x1e / bank33'},pack_bytes=len(data),pack_fnv=f'{fnv(data):08x}',pack_sha256=hashlib.sha256(data).hexdigest(),palette_bytes=len(paldata),palette_sha256=hashlib.sha256(paldata).hexdigest(),textures=len(textures),records=sum(len(x) for x in frames),details=details)
 (ROOT/'docs'/'test255-interstitial2c-assets.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS INTER2C',len(data),len(textures),report['records'],report['pack_sha256'])
if __name__=='__main__':main()
