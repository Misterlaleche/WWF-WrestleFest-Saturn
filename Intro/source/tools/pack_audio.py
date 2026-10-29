#!/usr/bin/env python3
"""Lossless independent1024-sample packets; no quantization or resampling."""
from pathlib import Path
import argparse,hashlib,json,struct
import numpy as np

BLOCK=1024

def encode_block(samples):
    n=len(samples)
    best=(n*16,0,0,None)
    for order in (1,2):
        if n<order:continue
        residual=np.diff(samples.astype(np.int32),n=order)
        unsigned=(residual<<1)^(residual>>31)
        for k in range(17):
            bits=int(np.sum(unsigned>>k))+len(unsigned)*(k+1)+order*16
            if bits<best[0]:best=(bits,order,k,unsigned)
    _,order,k,unsigned=best
    if order==0:
        payload=samples.astype('>i2').tobytes()
    else:
        data=bytearray(samples[:order].astype('>i2').tobytes())
        word=0;available=0
        def put(value,count):
            nonlocal word,available
            word=(word<<count)|int(value);available+=count
            while available>=8:
                available-=8;data.append((word>>available)&255)
                word&=(1<<available)-1
        for value in unsigned:
            value=int(value);q=value>>k
            put(0,q);put(1,1)
            if k:put(value&((1<<k)-1),k)
        if available:put(0,8-available)
        payload=bytes(data)
    assert len(payload)<=n*2
    return struct.pack('>HHBBH',n,len(payload),order,k,0)+payload

def main():
    ap=argparse.ArgumentParser();ap.add_argument('source',type=Path)
    ap.add_argument('output',type=Path);ap.add_argument('--seconds',type=float,default=37)
    args=ap.parse_args();raw=args.source.read_bytes()
    count=round(args.seconds*44100);assert count>0 and count*2<=len(raw)
    raw=raw[:count*2];samples=np.frombuffer(raw,dtype='>i2').astype(np.int32)
    h=2166136261
    for value in raw:h=((h^value)*16777619)&0xffffffff
    packets=[encode_block(samples[i:i+BLOCK]) for i in range(0,count,BLOCK)]
    total=32+sum(map(len,packets))
    header=struct.pack('>IHHIIIIII',0x57525030,1,BLOCK,44100,count,len(packets),total,h,0)
    data=header+b''.join(packets);assert len(data)==total
    args.output.write_bytes(data)
    report=dict(format='WRP0 v1; fixed1024samples; independent raw/order1/order2 Rice blocks',
        pcm_format='44100Hz mono signedPCM16 big-endian',samples=count,seconds=count/44100,
        pcm_bytes=len(raw),packed_bytes=total,blocks=len(packets),
        max_packet_bytes=max(map(len,packets)),
        pcm_sha256=hashlib.sha256(raw).hexdigest(),packed_sha256=hashlib.sha256(data).hexdigest(),
        pcm_fnv1a=f'{h:08x}',orders={str(i):sum(p[4]==i for p in packets) for i in range(3)})
    args.output.with_suffix('.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
