#!/usr/bin/env python3
"""Exercise both real C decoders with original PCM and independent bitstreams.

Catches unary/remainder bit order, truncated reads, padding, predictor range,
sample count and output overwrites. The encoder below is independent Python;
the old C decoder is a compatibility oracle for malformed inputs only.
"""
import argparse,ctypes,hashlib,json,random,struct,subprocess,tempfile
from pathlib import Path

def build(src,directory,name):
    out=directory/(name+'.so')
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',
                    '-I'+str(src.parent),str(src),'-o',str(out)],check=True)
    lib=ctypes.CDLL(str(out));fn=lib.wp0_rice_decode_block
    fn.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_int16),
                 ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint)]
    fn.restype=ctypes.c_bool
    return lib,fn

def decode(fn,packet,capacity=1024):
    raw=ctypes.create_string_buffer(packet)
    out=(ctypes.c_int16*1026)();out[0]=12345;out[1025]=23456
    n=ctypes.c_uint(0x76543210)
    ok=fn(raw,len(packet),ctypes.cast(ctypes.byref(out,2),ctypes.POINTER(ctypes.c_int16)),capacity,ctypes.byref(n))
    assert out[0]==12345 and out[1025]==23456,'Output canary overwritten'
    assert not ok or n.value<=capacity
    return ok,n.value,list(out[1:1+n.value]) if ok else []

def packet(samples,order,k):
    if order==0:payload=struct.pack('>'+str(len(samples))+'h',*samples)
    else:
        bits=''
        for i in range(order,len(samples)):
            pred=samples[i-1] if order==1 else 2*samples[i-1]-samples[i-2]
            r=samples[i]-pred;u=2*r if r>=0 else -2*r-1
            bits+='0'*(u>>k)+'1'
            if k:bits+=format(u&((1<<k)-1),'0'+str(k)+'b')
        bits+='0'*((-len(bits))%8)
        payload=struct.pack('>'+str(order)+'h',*samples[:order])+bytes(int(bits[i:i+8],2) for i in range(0,len(bits),8))
    assert len(payload)<65536
    return struct.pack('>HHBBBB',len(samples),len(payload),order,k,0,0)+payload

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['baseline','candidate','packed','pcm','output']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();rng=random.Random(0x57503034);cases=0;valid=0
    with tempfile.TemporaryDirectory(prefix='wp0_rice_diff_') as temp:
        oldlib,old=build(a.baseline,Path(temp),'old');newlib,new=build(a.candidate,Path(temp),'new')
        def check(data,expected=None,capacity=1024):
            nonlocal cases,valid
            x=decode(old,data,capacity);y=decode(new,data,capacity);cases+=1
            assert x==y,('compatibility',cases,x[:2],y[:2],data.hex()[:96])
            if expected is not None:assert y==(True,len(expected),expected),('PCM',cases)
            valid+=bool(y[0])
        packed=a.packed.read_bytes();raw=a.pcm.read_bytes();off=32;sample=0;blocks=0
        while off<len(packed):
            n,size=struct.unpack_from('>HH',packed,off);data=packed[off:off+8+size]
            values=list(struct.unpack_from('>'+str(n)+'h',raw,2*sample));check(data,values)
            off+=8+size;sample+=n;blocks+=1
        assert sample==1631700 and blocks==1594 and 2*sample==len(raw)
        # Independent known sample sequences across every supported parameter.
        for order in [1,2]:
            for k in range(17):
                for length in [order,3,4,9,31,64,1024]:
                    if length<order:continue
                    values=[rng.randrange(-400,401)]
                    while len(values)<length:
                        values.append(max(-32000,min(32000,values[-1]+rng.randrange(-8,9))))
                    data=packet(values,order,k);check(data,values)
                    for cut in sorted({0,1,7,8,len(data)-1,len(data)//2}):check(data[:cut])
                    check(data,capacity=length-1)
                    for index in [0,2,4,5,6,7,len(data)-1]:
                        altered=bytearray(data);altered[index]^=1;check(bytes(altered))
        for order in [1,2]:
            for k in range(17):
                values=[-32768,32767,-32768][:3]
                if k==0 and order==1:values=values[:2]
                data=packet(values,order,k);check(data,values)
        for length in [1,2,7,1024]:
            values=[rng.randrange(-32768,32768) for _ in range(length)]
            check(packet(values,0,0),values)
        # Random mutations exercise canonical termination and bounds.
        for _ in range(3000):
            order=rng.choice([1,2]);k=rng.randrange(17);n=rng.randrange(3,40)
            values=[rng.randrange(-30,31) for _ in range(n)]
            data=bytearray(packet(values,order,k));i=rng.randrange(len(data));data[i]^=1<<rng.randrange(8)
            if rng.randrange(4)==0:data=data[:rng.randrange(len(data)+1)]
            check(bytes(data))
    report={'pass':True,'cases':cases,'valid_cases':valid,'original_blocks':blocks,
            'original_samples_exact':sample,'all_k_0_through_16':True,'orders':[0,1,2],
            'baseline_sha256':hashlib.sha256(a.baseline.read_bytes()).hexdigest(),
            'candidate_sha256':hashlib.sha256(a.candidate.read_bytes()).hexdigest(),
            'pcm_sha256':hashlib.sha256(raw).hexdigest()}
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
