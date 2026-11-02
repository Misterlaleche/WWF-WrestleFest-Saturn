"""Retain four direct PT_LOADs, omitting debug sections and file padding."""
import struct

def compact(data):
    if len(data)<52 or data[:7]!=b'\x7fELF\x01\x02\x01':
        raise ValueError('ELF32 BE required')
    table=struct.unpack_from('>I',data,28)[0]
    stride,count=struct.unpack_from('>HH',data,42)
    if stride!=32 or count!=4 or table<52 or table+128>len(data):
        raise ValueError('Four complete program headers required')
    loads=[struct.unpack_from('>8I',data,table+i*32) for i in range(4)]
    for kind,offset,address,physical,file_bytes,memory_bytes,flags,align in loads:
        if kind!=1 or physical!=address or not memory_bytes or file_bytes>memory_bytes or \
           offset+file_bytes>len(data) or (offset|address|file_bytes|memory_bytes)&3:
            raise ValueError('Invalid direct load')
    result=bytearray(data[:52]);result.extend(bytes(256-52))
    struct.pack_into('>II',result,28,52,0)
    struct.pack_into('>6H',result,40,52,32,4,40,0,0)
    for i,load in enumerate(loads):
        kind,offset,address,physical,file_bytes,memory_bytes,flags,align=load
        new_offset=len(result) if file_bytes else 0
        struct.pack_into('>8I',result,52+i*32,kind,new_offset,address,physical,
                         file_bytes,memory_bytes,flags,4)
        result.extend(data[offset:offset+file_bytes])
    if len(result)>131072:
        raise ValueError('Music file exceeds the staging window')
    return bytes(result)
