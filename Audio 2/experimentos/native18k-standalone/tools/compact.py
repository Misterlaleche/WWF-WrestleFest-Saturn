import struct
def compact(data):
 assert data[:7]==b'\x7fELF\x01\x02\x01'
 table=struct.unpack_from('>I',data,28)[0]
 ents,n=struct.unpack_from('>HH',data,42);assert ents==32 and n==4
 seg=[struct.unpack_from('>8I',data,table+i*32) for i in range(n)]
 assert all(h[0]==1 for h in seg)
 program=seg[:2];start=0x05c5c000;end=max(h[3]+h[4] for h in program)
 assert end<=0x05c7bf00 and all(h[4]==h[5] for h in program)
 code=bytearray(end-start)
 for h in program:code[h[3]-start:h[3]-start+h[4]]=data[h[1]:h[1]+h[4]]
 header=bytearray(data[:52]);struct.pack_into('>II',header,28,52,0)
 struct.pack_into('>6H',header,40,52,32,3,40,0,0)
 ph=struct.pack('>8I',1,256,start,start,len(code),len(code),5,16)
 for h in seg[2:]:
  assert h[4]==0
  ph+=struct.pack('>8I',1,0,h[2],h[2],0,h[5],6,16)
 result=bytes(header)+ph+b'\0'*(256-148)+code
 assert len(result)<=131072
 return result
