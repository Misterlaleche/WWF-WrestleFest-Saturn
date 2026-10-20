#!/usr/bin/env python3
"""Independent checks of compact generated ring and streamed composition indexes."""
import argparse, ctypes, random, struct, subprocess, tempfile
from pathlib import Path
from wf_r18_rom_graphics import Rom,be16,be32
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--assets',type=Path,required=True);a=p.parse_args()
r=Rom(a.rom);base=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
 lib=Path(d)/'ring.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(base/'include'),str(base/'src/wf_r18_ring.c'),'-o',str(lib)],check=True)
 c=ctypes.CDLL(str(lib));c.wf_r18_ring_bind.argtypes=[ctypes.c_void_p,ctypes.c_size_t];c.wf_r18_ring_pixel.argtypes=[ctypes.c_uint,ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_uint8)]
 blob=(a.assets/'R18RING.BIN').read_bytes();buf=ctypes.create_string_buffer(blob);assert c.wf_r18_ring_bind(buf,len(blob))==0
 mode=be16(blob,6);rng=random.Random(18)
 for i in range(10000):
  layer=i%2;wx=rng.randrange(-2560,5120);wy=rng.randrange(-2048,4096);x=wx%2560;y=wy%2048;tx=x//16;ty=y//16
  lp='fg' if layer==0 else 'bg';ptr=r.world_page_ptrs(lp,mode)[ty//16*8+tx//20]
  # Independent source metatile address arithmetic from $26CA8/$26DAC.
  xx=tx%20;yy=ty%16
  off=ptr+(yy//2)*(0xa0 if layer==0 else 0x50)+(xx//2)*(8 if layer==0 else 4)+(yy%2)*(0x50 if layer==0 else 0x28)+(xx%2)*(4 if layer==0 else 2)
  raw=be16(r.p,off)
  if layer==0:code=be16(r.p,off+2)&0x1fff;pal=raw&15;fx=bool(raw&64);fy=bool(raw&128)
  else:code=raw&4095;pal=raw>>12;fx=fy=False
  px=15-x%16 if fx else x%16;py=15-y%16 if fy else y%16
  expected=pal*16+r.bg_tile(code)[py*16+px];actual=ctypes.c_uint8()
  assert c.wf_r18_ring_pixel(layer,wx,wy,ctypes.byref(actual))==0 and actual.value==expected,(layer,wx,wy,actual.value,expected)
 assert c.wf_r18_ring_update(320,512,320,512,0x7c)==0
 assert c.wf_r18_ring_update(320,512,320,512,0xff)==-1
 broken=ctypes.create_string_buffer(blob[:100]);assert c.wf_r18_ring_bind(broken,100)==-1
idx=(a.assets/'R18POSE.IDX').read_bytes();data=(a.assets/'R18POSE.BIN').read_bytes();counts=0
for s,_,_,n in r.active_sets():
 off=be32(idx,8+s*8);assert be16(idx,12+s*8)==n
 for pose in range(n):
  at=off+pose*8;pos,count,variants=struct.unpack_from('>IHH',idx,at)
  assert variants in (0,1,12)
  entries=[(pos,count)] if variants==1 else [struct.unpack_from('>IH',data,pos+k*8) for k in range(12)] if variants==12 else []
  for k,(where,num) in enumerate(entries):
   pp=r.expanded_pieces(s,pose,k);assert num==len(pp) and num<=127 and where+num*10<=len(data)
   for j,q in enumerate(pp):assert struct.unpack_from('>HBBhhBB',data,where+j*10)==(q['tile_code'],q['palette_resource'],q['flip_x']|q['flip_y']<<1,q['rel_x'],q['rel_y'],q['chain_index'],q['chain'])
   counts+=1
print('PASS: 10000 independent ROM-to-native-ring pixel checks; malformed blob/priority; %d all-set pose variants and bounded piece records'%counts)
