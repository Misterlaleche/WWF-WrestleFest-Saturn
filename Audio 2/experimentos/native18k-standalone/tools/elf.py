"""Small read-only ELF32 big-endian parser for this SH2 build contract."""
from pathlib import Path
import hashlib,struct

class Elf:
 def __init__(self,path):
  self.path=Path(path);self.data=self.path.read_bytes()
  if self.data[:7]!=b'\x7fELF\x01\x02\x01':raise ValueError('Expected ELF32 big-endian')
  h=struct.unpack_from('>HHIIIIIHHHHHH',self.data,16)
  self.entry=h[3]
  self.ph=[struct.unpack_from('>8I',self.data,h[4]+i*h[8])for i in range(h[9])]
  self.sections=[struct.unpack_from('>10I',self.data,h[5]+i*h[10])for i in range(h[11])]
  self.symbols={};self.symbol_sections={}
  for section in self.sections:
   if section[1]!=2:continue
   strings=self.sections[section[6]];table=self.data[strings[4]:strings[4]+strings[5]]
   for offset in range(section[4],section[4]+section[5],section[9]):
    name,value,size,info,other,index=struct.unpack_from('>IIIBBH',self.data,offset)
    if name:
     end=table.find(b'\0',name);name=table[name:end].decode('ascii')
     self.symbols[name]=(value,size);self.symbol_sections[name]=index
 def symbol_bytes(self,name):
  value,size=self.symbols[name];s=self.sections[self.symbol_sections[name]]
  start=s[4]+value-s[3];return self.data[start:start+size]
 def loads(self):
  return [dict(type=p[0],virtual=p[2],physical=p[3],file_bytes=p[4],memory_bytes=p[5],flags=p[6],alignment=p[7],
   sha256=hashlib.sha256(self.data[p[1]:p[1]+p[4]]).hexdigest())for p in self.ph if p[0]==1]
