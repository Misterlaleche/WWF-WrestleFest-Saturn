#!/usr/bin/env python3
import re, zipfile, sys
from pathlib import Path
root=Path(sys.argv[1]); rom=Path(sys.argv[2])
src=(root/'src/runtime/wp12_romexact.c').read_text()
with zipfile.ZipFile(rom) as z:
    even=z.read('31e14-0.ic18'); odd=z.read('31e13-0.ic19')
p=bytearray(len(even)*2); p[0::2]=even; p[1::2]=odd
u16=lambda a:(p[a]<<8)|p[a+1]
u32=lambda a:(u16(a)<<16)|u16(a+2)

def body(name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\[[^=]*?=\s*\{(.*?)\};',src,re.S)
    assert m, name
    return m.group(1)
def nums(name):
    return [int(x,0) for x in re.findall(r'0x[0-9A-Fa-f]+|(?<![A-Za-z0-9_])\d+',body(name))]
def rows(name):
    b=body(name); out=[]; depth=0; start=None
    for i,ch in enumerate(b):
        if ch=='{':
            if depth==0: start=i+1
            depth+=1
        elif ch=='}':
            depth-=1
            if depth==0 and start is not None:
                out.append([int(x,0) for x in re.findall(r'0x[0-9A-Fa-f]+|(?<![A-Za-z0-9_])\d+',b[start:i])]); start=None
    return out

assert nums('tag_primary_xy') == [u16(0x10708+2*i) for i in range(8)]
assert nums('royal_xy') == [u16(0x10ACC+2*i) for i in range(12)]
assert nums('royal_orientation') == [u16(0x10AE4+2*i) for i in range(6)]
assert nums('identity_threshold') == [u16(0xAC9E+2*i) for i in range(12)]
assert nums('identity_voice_low') == list(p[0xAC4A:0xAC56])
assert nums('aaea_audio_low') == list(p[0xAC56:0xAC6E])
assert nums('final_audio') == [u16(0xDEE+2*i) for i in range(10)]
counts=list(p[0xAD9C:0xADA8]); assert nums('local_count')==counts
ptrs=[u32(0xAD6C+4*i) for i in range(12)]
sel=[]; thr=[]
for q,c in zip(ptrs,counts):
    sel.append([u16(q+2*j) for j in range(c)])
    thr.append([u16(q+2*c+2*j) for j in range(c)])
srows=rows('local_selector'); trows=rows('local_threshold')
assert len(srows)==12 and len(trows)==12
for i,c in enumerate(counts):
    assert srows[i]==sel[i], ('selector',i,srows[i],sel[i])
    assert trows[i]==thr[i], ('threshold',i,trows[i],thr[i])
callbacks=[u32(0xA7B4+4*i) for i in range(24)]
expected=[0xA9E2,0xAA96,0xAAEA,0xA9E2,0xAAEA,0xA814,0xAAEA,0xA8C0,0xAAEA,0xA814,0xA8C0,0xAAEA,0xA8C0,0xAAEA,0xA814,0xAAEA,0xA8C0,0xAAEA,0xA814,0xA9E2,0xAA96,0xA8EC,0xA952,0xA99A]
assert callbacks==expected
print('WP12 R02 source-table vs ROM audit: PASS')
