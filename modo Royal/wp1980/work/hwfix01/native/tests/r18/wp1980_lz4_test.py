#!/usr/bin/env python3
"""Independent liblz4-HC encoder / Saturn decoder byte and bounds tests."""
import ctypes
import ctypes.util
import random
import subprocess
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
def main():
    encoder=ctypes.CDLL(ctypes.util.find_library('lz4'))
    encode=encoder.LZ4_compress_HC
    encode.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int]
    rng=random.Random(1980)
    cases=[b'',b'a',b'a'*4096,bytes(range(256))*16,b'\0'*4096,b'ab'*2048,b'abc'*1365]
    cases.extend(bytes(rng.randrange(256)for _ in range(n))for n in (2,7,31,128,512,1024,2048,4096))
    cases.extend(bytes(rng.randrange(4)for _ in range(4096))for _ in range(50))
    ids=ROOT/'audit/wp1980-host/royal-tiles-direct-union.txt'
    if ids.exists():
        original=(ROOT/'audit/runtime18-cd/SPRITES.PAK').read_bytes()
        selected=[int(x,16 if x.startswith('0x')else 10)for x in ids.read_text().splitlines()if x and not x.startswith('#')]
        atlas=b''.join(original[t*128:t*128+128]for t in selected)
        cases.extend(atlas[i:i+4096]for i in range(0,len(atlas),4096))
    with tempfile.TemporaryDirectory()as td:
        library=Path(td)/'lz4.so'
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',
                        '-I'+str(ROOT/'include'),str(ROOT/'src/wp1980_lz4.c'),'-o',str(library)],check=True)
        decode=ctypes.CDLL(str(library)).wp1980_lz4_decode
        decode.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t]
        checks=0
        for data in cases:
            temp=ctypes.create_string_buffer(len(data)*2+64)
            n=encode(data,temp,len(data),len(temp),12);assert n>0
            packed=temp.raw[:n]
            for alignment in range(4):
                inp=ctypes.create_string_buffer(alignment+n)
                ctypes.memmove(ctypes.addressof(inp)+alignment,packed,n)
                out=ctypes.create_string_buffer(len(data)+32)
                out.raw=b'\xa5'*len(out);start=8+alignment
                op=ctypes.addressof(out)+start;ip=ctypes.addressof(inp)+alignment
                assert decode(ip,n,op,len(data))==0,(len(data),alignment)
                assert out.raw[start:start+len(data)]==data
                assert out.raw[:start]==b'\xa5'*start
                assert out.raw[start+len(data):]==b'\xa5'*(len(out)-start-len(data))
                checks+=1
                for cut in sorted({0,1,n//2,n-1}):
                    if cut<n:assert decode(ip,cut,op,len(data))!=0;checks+=1
                if data:assert decode(ip,n,op,len(data)-1)!=0;checks+=1
        for packed in [b'\xf0',b'\xf0\xff',b'\x00\x00\x00',b'\x00\x01\x00']:
            out=ctypes.create_string_buffer(4096)
            assert decode(packed,len(packed),out,4096)!=0;checks+=1
        print(f'PASS: {len(cases)} liblz4-generated blocks; {checks} byte/bounds/rejection checks')
if __name__=='__main__':main()
