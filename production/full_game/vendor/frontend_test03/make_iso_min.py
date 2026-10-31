#!/usr/bin/env python3
from pathlib import Path
import struct, math, datetime
SECTOR=2048
root=Path(__file__).resolve().parent
cd=root/'cd'
ip=(root/'build/IP.BIN').read_bytes()
out=root/'wrestlefest-saturn-mode-roster-integration-test03.iso'
files=['A.BIN','ABS.TXT','BIB.TXT','CPY.TXT']

def both16(v): return struct.pack('<H',v)+struct.pack('>H',v)
def both32(v): return struct.pack('<I',v)+struct.pack('>I',v)
def rec(extent,size,name,flags=0):
    if isinstance(name,str): nb=name.encode('ascii')
    else: nb=name
    n=33+len(nb)+(0 if len(nb)%2 else 1)
    b=bytearray(n); b[0]=n; b[1]=0; b[2:10]=both32(extent); b[10:18]=both32(size)
    now=datetime.datetime(2026,9,10,12,0,0)
    b[18:25]=bytes([now.year-1900,now.month,now.day,now.hour,now.minute,now.second,0])
    b[25]=flags; b[26]=0; b[27]=0; b[28:32]=both16(1); b[32]=len(nb); b[33:33+len(nb)]=nb
    return bytes(b)
# layout: 0-15 system area, 16 PVD, 17 VDST, 18 L path, 19 M path, 20 root dir, files...
path_l=18; path_m=19; root_extent=20
file_ext={}; cur=21
for fn in files:
    sz=(cd/fn).stat().st_size; file_ext[fn]=(cur,sz); cur += (sz+SECTOR-1)//SECTOR
vol_sectors=cur
# root directory
entries=[rec(root_extent,SECTOR,b'\x00',2),rec(root_extent,SECTOR,b'\x01',2)]
for fn in files:
    ex,sz=file_ext[fn]; entries.append(rec(ex,sz,fn+';1',0))
dirdata=b''.join(entries)
assert len(dirdata)<=SECTOR
dirsec=dirdata.ljust(SECTOR,b'\0')
# path tables, root id len 1, ext attr 0, extent, parent=1, id=0, pad
pl=bytes([1,0])+struct.pack('<I',root_extent)+struct.pack('<H',1)+b'\0\0'
pm=bytes([1,0])+struct.pack('>I',root_extent)+struct.pack('>H',1)+b'\0\0'
# PVD
p=bytearray(SECTOR); p[0]=1; p[1:6]=b'CD001'; p[6]=1
p[8:40]=b'SEGA SEGASATURN'.ljust(32,b' '); p[40:72]=b''.ljust(32,b' ')
p[80:88]=both32(vol_sectors); p[120:124]=both16(1); p[124:128]=both16(1); p[128:132]=both16(SECTOR)
p[132:140]=both32(len(pl)); p[140:144]=struct.pack('<I',path_l); p[144:148]=struct.pack('<I',0); p[148:152]=struct.pack('>I',path_m); p[152:156]=struct.pack('>I',0)
p[156:190]=rec(root_extent,SECTOR,b'\x00',2)
p[190:318]=b'SEGA ENTERPRISES, LTD.'.ljust(128,b' '); p[318:446]=b'SEGA ENTERPRISES, LTD.'.ljust(128,b' '); p[446:574]=b'SEGA ENTERPRISES, LTD.'.ljust(128,b' ')
p[574:702]=b'CPY.TXT;1'.ljust(128,b' '); p[702:739]=b'ABS.TXT;1'.ljust(37,b' '); p[739:776]=b'BIB.TXT;1'.ljust(37,b' ')
p[881]=1
v=bytearray(SECTOR); v[0]=255; v[1:6]=b'CD001'; v[6]=1
with out.open('wb') as f:
    f.write(ip[:16*SECTOR].ljust(16*SECTOR,b'\0'))
    f.write(p); f.write(v); f.write(pl.ljust(SECTOR,b'\0')); f.write(pm.ljust(SECTOR,b'\0')); f.write(dirsec)
    for fn in files:
        d=(cd/fn).read_bytes(); f.write(d); f.write(b'\0'*((-len(d))%SECTOR))
print(out, out.stat().st_size, vol_sectors)
