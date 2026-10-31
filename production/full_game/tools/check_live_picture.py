#!/usr/bin/env python3
"""Compare visible background pixels with a separate ROM bitplane renderer.

Uses a read-only HWRAM dump and the Mednafen native PNG. Text pixels and sprite
rectangles are excluded: their later screenshot can contain a different blink
phase. This is a background/scroll/palette check, not full-frame equivalence.
"""
import argparse,hashlib,json,struct,zlib
from pathlib import Path
from recover_graphics import Graphics

def symbols(path):
 b=path.read_bytes()
 if b[:6]!=b'\x7fELF\x01\x02':raise ValueError('Expected ELF32 big endian')
 h=struct.unpack_from('>HHIIIIIHHHHHH',b,16)
 sections=[struct.unpack_from('>IIIIIIIIII',b,h[5]+i*h[10]) for i in range(h[11])];out={}
 for s in sections:
  if s[1]!=2:continue
  names=sections[s[6]];strings=b[names[4]:names[4]+names[5]]
  for at in range(s[4],s[4]+s[5],s[9]):
   n,value,size,_,_,_=struct.unpack_from('>IIIBBH',b,at)
   name=strings[n:].split(b'\0',1)[0].decode()
   if name:out[name]=(value,size)
 return out

def png(path):
 b=path.read_bytes();assert b[:8]==b'\x89PNG\r\n\x1a\n';at=8;packed=bytearray()
 while at<len(b):
  n=struct.unpack_from('>I',b,at)[0];kind=b[at+4:at+8];data=b[at+8:at+8+n]
  assert zlib.crc32(kind+data)==struct.unpack_from('>I',b,at+8+n)[0]
  if kind==b'IHDR':w,h,depth,color,comp,filt,inter=struct.unpack('>IIBBBBB',data);assert (depth,color,comp,filt,inter)==(8,2,0,0,0)
  if kind==b'IDAT':packed+=data
  at+=n+12
 raw=zlib.decompress(packed);assert len(raw)==h*(w*3+1);rows=[];previous=bytearray(w*3)
 for y in range(h):
  start=y*(w*3+1);kind=raw[start];row=bytearray(raw[start+1:start+1+w*3]);assert kind<=4
  for x in range(w*3):
   a=row[x-3] if x>=3 else 0;b=previous[x];c=previous[x-3] if x>=3 else 0
   p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
   predictor=(0,a,b,(a+b)//2,a if pa<=pb and pa<=pc else b if pb<=pc else c)[kind]
   row[x]=(row[x]+predictor)&255
  rows.append(row);previous=row
 return w,h,rows

def compare(rom,elf,dump,screenshot):
 sy=symbols(elf);mem=dump.read_bytes()
 def data(n):
  at,size=sy[n];at-=0x06000000
  if at<0 or at+size>len(mem):raise ValueError('Symbol outside HWRAM: '+n)
  return mem[at:at+size]
 board=data('_board');raw=data('_previous_bg');text=data('_previous_text')
 pal=struct.unpack('>1024I',data('_previous_palette'))
 scroll=struct.unpack_from('>4H',board,14);priority=board[63]
 if priority!=0x78:raise ValueError('This evidence check covers selection priority78 only')
 gfx=Graphics(rom);w,h,actual=png(screenshot)
 if (w,h)!=(352,240):raise ValueError('Expected native352x240 overscan PNG')
 excluded=bytearray(320*240)
 count=struct.unpack('>I',data('_wf_video_sprites'))[0]
 pieces=data('_pieces')
 for i in range(count):
  _,x,y,_,_=struct.unpack_from('>HhhBB',pieces,i*8)
  for yy in range(max(0,y),min(240,y+16)):
   for xx in range(max(0,x),min(320,x+16)):excluded[yy*320+xx]=1
 for y in range(240):
  for x in range(320):
   # Original FG0 is a64x32 map of8x8 glyphs; board visible Y begins at8.
   i=((y+8)//8)*64+x//8;a,b=struct.unpack_from('>HH',text,i*4)
   if gfx.tile('text',(a&255)|((b&15)<<8))[(y+8)&7][x&7]:excluded[y*320+x]=1
 checked=mismatch=0;examples=[]
 for y in range(240):
  for x in range(320):
   if excluded[y*320+x]:continue
   value=0
   for layer in (0,1):
    xx=(x+scroll[2 if layer==0 else 0])&511;yy=(y+scroll[3 if layer==0 else 1]+8)&511;i=(yy//16)*32+xx//16
    if layer:attr,code=struct.unpack_from('>HH',raw,4096+i*4);bank=attr&15;flip=(attr>>6)&3
    else:attr=struct.unpack_from('>H',raw,i*2)[0];code=attr&4095;bank=attr>>12;flip=0
    px,py=xx&15,yy&15
    if flip&1:px=15-px
    if flip&2:py=15-py
    pen=gfx.tile('background',code&4095)[py][px]
    if layer==0 or pen:value=pal[256*(layer+1)+bank*16+pen]
   wanted=bytes((value&255,(value>>8)&255,(value>>16)&255));observed=bytes(actual[y][(x+16)*3:(x+17)*3])
   checked+=1
   if wanted!=observed:
    mismatch+=1
    if len(examples)<10:examples.append({'x':x,'y':y,'expected':list(wanted),'actual':list(observed)})
 return {'status':'PASS' if checked>30000 and mismatch==0 else 'FAIL','checked_background_pixels':checked,
         'excluded_text_sprite_pixels':sum(excluded),'mismatches':mismatch,'examples':examples,
         'scope':'selection background pixels only; sprite rectangles/text pixels excluded',
         'files':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (elf,dump,screenshot)}}

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,required=True);p.add_argument('--elf',type=Path,required=True)
 p.add_argument('--dump',type=Path,required=True);p.add_argument('--png',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
 a=p.parse_args();result=compare(a.rom,a.elf,a.dump,a.png);a.out.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));raise SystemExit(result['status']!='PASS')
