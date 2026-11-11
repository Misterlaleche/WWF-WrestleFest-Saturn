#!/usr/bin/env python3
"""Replace TAG module and append verified TAG ROM catalog in an internal ISO."""
import hashlib, json, mmap, shutil, struct
from pathlib import Path
R=Path('/Users/alejandro/Documents/Codex/2026-10-30/files-pasted-by-the-user-encargo/work')
S=R/'session294-tag-rom-catalog-stream-aligned-r01'
SOURCE=R/'session275-demolition-profile-plan-r01/internal-profile-test.iso'
OUT=S/'internal-catalog-test.iso'
EXPECTED_SOURCE='22ab673a6ebbb5d27c15d62d1ceb97aabfe279e626478f8e84e184ccfcd7ab1d'
EXPECTED_CATALOG='b101ea8d505e589da289276399bdd0636b9a14151a6c3fcee7a4ad00fa38f94b'
SECTOR=2048

def sha(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  for b in iter(lambda:f.read(1<<20),b''):h.update(b)
 return h.hexdigest()

def record(name,lba,size):
 n=(name+';1').encode('ascii');length=33+len(n)+(len(n)%2==0)
 r=bytearray(length);r[0]=length
 struct.pack_into('<I',r,2,lba);struct.pack_into('>I',r,6,lba)
 struct.pack_into('<I',r,10,size);struct.pack_into('>I',r,14,size)
 struct.pack_into('<H',r,28,1);struct.pack_into('>H',r,30,1)
 r[32]=len(n);r[33:33+len(n)]=n
 return r

if OUT.exists() or sha(SOURCE)!=EXPECTED_SOURCE:raise SystemExit('unexpected base or output exists')
cat=R/'session294-tag-rom-catalog-stream-aligned-r01/TAGROM.BIN'
if sha(cat)!=EXPECTED_CATALOG:raise SystemExit('TAG catalog hash changed')
package=S/'package-cd';new={n:(package/n).read_bytes() for n in ('TAG.BIN','TAG.HDR')}
shutil.copyfile(SOURCE,OUT)
with OUT.open('r+b') as f:
 m=mmap.mmap(f.fileno(),0);pvd=16*SECTOR
 if m[pvd:pvd+6]!=b'\x01CD001' or len(m)%SECTOR:raise SystemExit('invalid ISO')
 root=pvd+156;start=struct.unpack_from('<I',m,root+2)[0]*SECTOR
 end=start+struct.unpack_from('<I',m,root+10)[0]
 records={};at=start;last=start
 while at<end:
  size=m[at]
  if not size:at=(at//SECTOR+1)*SECTOR;continue
  name=m[at+33:at+33+m[at+32]].decode('ascii')
  records[name]=at;last=at+size;at=last
 if len(records)-2!=68 or 'TAGROM.BIN;1' in records:raise SystemExit('unexpected root')
 for name,raw in new.items():
  rec=records[name+';1'];lba=struct.unpack_from('<I',m,rec+2)[0]
  old=struct.unpack_from('<I',m,rec+10)[0];sectors=(old+SECTOR-1)//SECTOR
  if (len(raw)+SECTOR-1)//SECTOR!=sectors:raise SystemExit(name+' sector count changed')
  off=lba*SECTOR;m[off:off+len(raw)]=raw
  m[off+len(raw):off+sectors*SECTOR]=bytes(sectors*SECTOR-len(raw))
  struct.pack_into('<I',m,rec+10,len(raw));struct.pack_into('>I',m,rec+14,len(raw))
 cat_size=cat.stat().st_size;cat_lba=len(m)//SECTOR;entry=record('TAGROM.BIN',cat_lba,cat_size)
 if last//SECTOR!=(last+len(entry)-1)//SECTOR or any(m[last:last+len(entry)]):raise SystemExit('root directory lacks safe record space')
 m[last:last+len(entry)]=entry;m.flush();m.close()
 f.seek(0,2)
 with cat.open('rb') as c:shutil.copyfileobj(c,f)
 f.write(bytes((-cat_size)%SECTOR));f.flush()
 sector=cat_lba+(cat_size+SECTOR-1)//SECTOR
 f.seek(pvd+80);f.write(struct.pack('<I',sector));f.write(struct.pack('>I',sector))
result={'status':'INTERNAL_ISO_ONLY','source_sha256':EXPECTED_SOURCE,'iso_sha256':sha(OUT),'root_files':69,'volume_sectors':sector,'files':{'TAGROM.BIN':{'sha256':EXPECTED_CATALOG,'size':cat_size},**{n:{'sha256':hashlib.sha256(raw).hexdigest(),'size':len(raw)} for n,raw in new.items()}},'scope':'Mednafen only; no BIN/CUE or physical claim'}
OUT.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
