#!/usr/bin/env python3
"""Independent raw-zlib differential plus malformed RFC1951 test corpus."""
import argparse, hashlib, json, os, random, struct, subprocess, zlib
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path)
parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
OUT=args.output or ROOT/'audit/wp1980-inflate-dynamic'
OUT.mkdir(parents=True,exist_ok=True)
rng=random.Random(19801951)
cases=[];categories={}

def add(packed,output,valid=True,category='valid'):
    if isinstance(output,int):size=output;data=b''
    else:size=len(output);data=output
    cases.append((packed,size,int(valid),data))
    categories[category]=categories.get(category,0)+1

def compressed(raw,strategy=zlib.Z_DEFAULT_STRATEGY,level=9):
    c=zlib.compressobj(level,zlib.DEFLATED,-15,9,strategy)
    return c.compress(raw)+c.flush()

def valid(raw,strategy=zlib.Z_DEFAULT_STRATEGY,level=9,category='zlib'):
    packed=compressed(raw,strategy,level)
    if len(packed)>65536:
        add(packed,len(raw),False,'contract-size-limit');return
    assert zlib.decompress(packed,-15)==raw
    add(packed,raw,category=category)
    for cut in sorted({0,1,len(packed)//2,len(packed)-1}):
        if cut<len(packed):add(packed[:cut],len(raw),False,'truncated')
    if raw:add(packed,len(raw)-1,False,'short-output')
    if len(raw)<65536:add(packed,len(raw)+1,False,'long-output')
    add(packed+b'\0',len(raw),False,'trailing-byte')

for size in (0,1,2,3,4,5,7,16,31,128,256,512,1024,2048,4096,8192,16384,32768,65536):
    patterns=[b'\0'*size,(b'ABCD'*(size//4+1))[:size],
              bytes(rng.randrange(256) for _ in range(size)),
              bytes(rng.randrange(4) for _ in range(size))]
    for raw in patterns:
        for strategy in (zlib.Z_DEFAULT_STRATEGY,zlib.Z_FIXED,zlib.Z_HUFFMAN_ONLY,zlib.Z_RLE):
            valid(raw,strategy)

sprite_source=ROOT/'audit/runtime18-cd/SPRITES.PAK'
ids_source=ROOT/'audit/wp1980-host/royal-tiles-direct-union.txt'
original_blocks=0
if sprite_source.exists() and ids_source.exists():
    sprites=sprite_source.read_bytes()
    assert hashlib.sha256(sprites).hexdigest()=='130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31'
    ids=sorted({int(s,0) for s in ids_source.read_text().split()})
    atlas=b''.join(sprites[t*128:t*128+128] for t in ids)
    for offset in range(0,len(atlas),4096):
        valid(atlas[offset:offset+4096],category='original-final-atlas');original_blocks+=1

# Multiple blocks in one raw stream, with back references into prior blocks.
for flush in (zlib.Z_SYNC_FLUSH,zlib.Z_FULL_FLUSH,zlib.Z_BLOCK):
    c=zlib.compressobj(6,zlib.DEFLATED,-15)
    raw=(bytes(range(256))*48)+b'ABCDE'*500
    packed=c.compress(raw[:8000])+c.flush(flush)+c.compress(raw[8000:])+c.flush()
    assert zlib.decompress(packed,-15)==raw
    add(packed,raw,category='multi-block')

class Bits:
    def __init__(self):self.bits=[]
    def put(self,value,count):self.bits.extend((value>>i)&1 for i in range(count))
    def code(self,value,count):
        self.bits.extend((value>>(count-1-i))&1 for i in range(count))
    def data(self):
        out=bytearray((len(self.bits)+7)//8)
        for i,v in enumerate(self.bits):out[i//8]|=v<<(i&7)
        return bytes(out)

def codes(lengths):
    count=[0]*16
    for n in lengths:
        if n:count[n]+=1
    next_code=[0]*16;code=0
    for n in range(1,16):code=(code+count[n-1])<<1;next_code[n]=code
    result={}
    for symbol,n in enumerate(lengths):
        if n:result[symbol]=(next_code[n],n);next_code[n]+=1
    return result

ORDER=(16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15)
def dynamic(lit,dist,payload):
    b=Bits();b.put(1,1);b.put(2,2);b.put(len(lit)-257,5);b.put(len(dist)-1,5);b.put(15,4)
    cl=[4]*13+[5]*6;cc=codes(cl)
    for symbol in ORDER:b.put(cl[symbol],3)
    for n in lit+dist:b.code(*cc[n])
    lc,dc=codes(lit),codes(dist)
    for symbol in payload:b.code(*lc[symbol])
    return b

# A complete very skewed tree forces the long-code path through15 bits.
lit=[0]*257
for i in range(14):lit[i]=i+1
lit[14]=lit[256]=15
b=dynamic(lit,[0],list(range(15))*7+[256]);raw=bytes(range(15))*7
assert zlib.decompress(b.data(),-15)==raw
add(b.data(),raw,category='handbuilt-long-code15')
# Valid literal-only and single-bit end-marker trees.
lit=[0]*257;lit[256]=1
b=dynamic(lit,[0],[256]);assert zlib.decompress(b.data(),-15)==b''
add(b.data(),b'',category='handbuilt-single-end-empty-distance')

# These streams are intentionally invalid, independently rejected by zlib.
malformed=[b'\x07',b'\x01\x01\x00\x00\x00A',b'\x03\x02\x00']
b=Bits();b.put(1,1);b.put(2,2);b.put(31,5);b.put(0,9);malformed.append(b.data())
b=Bits();b.put(1,1);b.put(2,2);b.put(0,14);b.put(0,12);malformed.append(b.data())
for cl in ([1,1,1,1],[2,2,2,0]):
    b=Bits();b.put(1,1);b.put(2,2);b.put(0,5);b.put(0,5);b.put(0,4)
    for n in cl:b.put(n,3)
    malformed.append(b.data())
# Code length16 cannot appear before a previous code length.
b=Bits();b.put(1,1);b.put(2,2);b.put(0,5);b.put(0,5);b.put(0,4)
for n in (1,0,0,1):b.put(n,3)
b.code(1,1);b.put(0,2);malformed.append(b.data())
# Literal tree without an end marker; tree itself is otherwise well formed.
lit=[0]*257;lit[0]=lit[1]=1
malformed.append(dynamic(lit,[0],[0]).data())
# Oversubscribed and incomplete literal alphabets, and an empty distance
# alphabet used by a match, must all be rejected before any unsafe copy.
lit=[0]*257;lit[65]=lit[66]=lit[256]=1
malformed.append(dynamic(lit,[0],[65]).data())
lit=[0]*257;lit[65]=lit[256]=2
malformed.append(dynamic(lit,[0],[65]).data())
lit=[0]*258;lit[65]=1;lit[256]=lit[257]=2
malformed.append(dynamic(lit,[0],[65,257]).data())
# Reserved literal286/287 and distance30/31 are represented in fixed trees
# but are not legal DEFLATE tokens.
fixed_lengths=[8]*144+[9]*112+[7]*24+[8]*8
fc=codes(fixed_lengths)
for symbol in (286,287):
    b=Bits();b.put(1,1);b.put(1,2);b.code(*fc[symbol]);b.code(*fc[256])
    malformed.append(b.data())
for symbol in (30,31):
    b=Bits();b.put(1,1);b.put(1,2);b.code(*fc[65]);b.code(*fc[257]);b.code(symbol,5)
    malformed.append(b.data())
# Two maximum zero-length repeats overflow the257+1 code-length array.
b=Bits();b.put(1,1);b.put(2,2);b.put(0,5);b.put(0,5);b.put(0,4)
for n in (0,0,1,1):b.put(n,3)
b.code(1,1);b.put(127,7);b.code(1,1);b.put(127,7);malformed.append(b.data())
for packed in malformed:
    try:zlib.decompress(packed,-15)
    except zlib.error:pass
    else:raise AssertionError(('malformed fixture accepted',packed))
    add(packed,4096,False,'handbuilt-invalid')

# Byte mutations, tested against the independent bounded zlib oracle.
seeds=[compressed(b'AB'*2048),compressed(bytes(range(64))*64),b.data()]
for seed in seeds:
    for _ in range(300):
        m=bytearray(seed)
        if m:m[rng.randrange(len(m))]^=1<<rng.randrange(8)
        packed=bytes(m);d=zlib.decompressobj(-15)
        try:
            raw=d.decompress(packed,65537)
            ok=d.eof and not d.unused_data and not d.unconsumed_tail and len(raw)<=65536
        except zlib.error:ok=False
        add(packed,raw if ok else 4096,ok,'mutated-zlib-oracle')

corpus=OUT/'vectors.bin'
with corpus.open('wb') as f:
    f.write(struct.pack('>I',len(cases)))
    for packed,size,ok,data in cases:
        f.write(struct.pack('>III',len(packed),size,ok));f.write(packed)
        if ok:f.write(data)
binary=OUT/('runner-sanitized' if args.sanitize else 'runner')
flags=['-O2','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.sanitize else ['-O2']
command=['cc','-std=c11','-Wall','-Wextra','-Werror',*flags,'-I'+str(ROOT/'include'),
         str(ROOT/'tests/r18/wp1980_inflate_dynamic_runner.c'),str(ROOT/'src/wp1980_inflate_dynamic.c'),'-o',str(binary)]
subprocess.run(command,check=True)
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';env['UBSAN_OPTIONS']='halt_on_error=1'
run=subprocess.run([str(binary),str(corpus)],env=env,capture_output=True,text=True)
print(run.stdout,end='');print(run.stderr,end='')
report={'cases':len(cases),'checks':len(cases)*4+5,'original_final_atlas_blocks':original_blocks,
        'categories':categories,'sanitized':args.sanitize,'returncode':run.returncode,
        'decoder_SHA256':hashlib.sha256((ROOT/'src/wp1980_inflate_dynamic.c').read_bytes()).hexdigest(),
        'vectors_SHA256':hashlib.sha256(corpus.read_bytes()).hexdigest(),'encoder_zlib':zlib.ZLIB_VERSION,
        'compiler':command}
(OUT/('results-sanitized.json' if args.sanitize else 'results.json')).write_text(json.dumps(report,indent=2)+'\n')
assert run.returncode==0
