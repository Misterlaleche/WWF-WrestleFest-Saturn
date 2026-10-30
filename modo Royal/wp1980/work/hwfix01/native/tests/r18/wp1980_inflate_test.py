#!/usr/bin/env python3
"""Independent zlib encoder / C decoder differential and rejection checks."""
import ctypes
import random
import subprocess
import tempfile
import zlib
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]

def fixed(data):
    c=zlib.compressobj(9,zlib.DEFLATED,-15,9,zlib.Z_FIXED)
    return c.compress(data)+c.flush()

def main():
    rng=random.Random(1980)
    cases=[b'',b'a',b'a'*4096,bytes(range(256))*16,b'\0'*4096]
    cases.extend(bytes(rng.randrange(256) for _ in range(n))
                 for n in (2,7,31,128,512,1024,2048,4096))
    cases.extend(bytes(rng.randrange(4) for _ in range(4096)) for _ in range(50))
    ids=ROOT/'audit/wp1980-host/royal-tiles-demo.txt'
    sprites=ROOT/'audit/runtime18-cd/SPRITES.PAK'
    if ids.exists() and sprites.exists():
        original=sprites.read_bytes()
        selected=[int(x,16 if x.startswith('0x') else 10)
                  for x in ids.read_text().splitlines() if x and not x.startswith('#')]
        atlas=b''.join(original[t*128:t*128+128] for t in selected)
        cases.extend(atlas[i:i+4096] for i in range(0,len(atlas),4096))
    with tempfile.TemporaryDirectory() as td:
        lib=Path(td)/'inflate.so'
        subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror',
                        '-shared','-fPIC','-I'+str(ROOT/'include'),
                        str(ROOT/'src/wp1980_inflate.c'),'-o',str(lib)],check=True)
        f=ctypes.CDLL(str(lib)).wp1980_inflate_fixed
        f.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t]
        f.restype=ctypes.c_int
        checks=0
        for data in cases:
            packed=fixed(data)
            out=ctypes.create_string_buffer(len(data)+16)
            out.raw=b'\xa5'*len(out)
            assert f(packed,len(packed),out,len(data))==0,(len(data),packed[:16])
            assert out.raw[:len(data)]==data
            assert out.raw[len(data):]==b'\xa5'*16
            checks+=1
            for cut in sorted({0,1,len(packed)//2,len(packed)-1}):
                if cut>=len(packed):continue
                assert f(packed,cut,out,len(data))!=0,('truncation',len(data),cut)
                checks+=1
            if data:
                assert f(packed,len(packed),out,len(data)-1)!=0
                checks+=1
        for packed in (b'\x07',b'\x05',b'\x01\x01\x00\x00\x00A',b'\x03\x02\x00'):
            out=ctypes.create_string_buffer(4096)
            assert f(packed,len(packed),out,4096)!=0,packed
            checks+=1
        print(f'PASS: {len(cases)} zlib-generated blocks; {checks} exact/rejection checks')

if __name__=='__main__':main()
