"""Keep only the3 load segments of a SH2 music ELF; output remains ELF."""
import struct

def compact(data):
 if data[:7]!=b'\x7fELF\x01\x02\x01' or len(data)<148:raise ValueError('Expected ELF32 BE')
 if struct.unpack_from('>HH',data,16)!=(2,42):raise ValueError('Expected SH2 executable')
 table=struct.unpack_from('>I',data,28)[0]
 if table>len(data) or len(data)-table<96:raise ValueError('Truncated program headers')
 if struct.unpack_from('>HH',data,42)!=(32,3):raise ValueError('Expected3 program headers')
 segments=[]
 for i in range(3):
  h=struct.unpack_from('>8I',data,table+i*32)
  if h[0]!=1 or h[4]>h[5] or h[1]+h[4]>len(data):raise ValueError('Invalid load segment')
  if i and h[4]:raise ValueError('State/shared must be NOBITS')
  segments.append(h)
 first=segments[0];code=data[first[1]:first[1]+first[4]]
 header=bytearray(data[:52]);struct.pack_into('>II',header,28,52,0);struct.pack_into('>6H',header,40,52,32,3,40,0,0)
 ph=b''
 for i,h in enumerate(segments):ph+=struct.pack('>8I',1,256 if not i else 0,h[2],h[3],h[4],h[5],h[6],16)
 result=bytes(header)+ph+b'\0'*(256-148)+code
 if len(result)>131072:raise ValueError('Music ELF exceeds scene scratch')
 return result
