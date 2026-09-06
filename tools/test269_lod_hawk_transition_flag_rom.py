#!/usr/bin/env python3
from pathlib import Path
import argparse

def expect(buf, off, hx, label):
    want=bytes.fromhex(hx.replace(' ',''))
    got=buf[off:off+len(want)]
    assert got==want, f'{label}: {got.hex()} != {want.hex()}'

p=argparse.ArgumentParser()
p.add_argument('--program', required=True)
a=p.parse_args()
b=Path(a.program).read_bytes()
expect(b,0x1C06A,'43 F9 00 01 14 78 30 28 00 1C','normal-state descriptor table')
expect(b,0x11498,'00 01 1E A6','state8 descriptor pointer')
expect(b,0x11EA6,'00 01 1F 3A','state8 callback pointer')
expect(b,0x11F5E,'00 01 1F AE 00 01 1F AE 00 01 1F AE 00 01 1F AE 00 01 20 78 00 01 20 78 00 01 20 78 00 01 20 78 00 01 21 22 00 01 21 22','state8 initializer table')
for off,label in [(0x11FBA,'state8 init A'),(0x1208A,'state8 init B'),(0x1212E,'state8 init C')]:
    expect(b,off,'08 E8 00 01 00 32',label+' sets +32 bit1')
expect(b,0x1206A,'31 7C 00 09 00 20','state8 A completion -> state9')
expect(b,0x12112,'31 7C 00 09 00 20','state8 B completion -> state9')
expect(b,0x121BE,'31 7C 00 09 00 20','state8 C completion -> state9')
expect(b,0x12C40,'08 A8 00 01 00 32 66 0E','Hawk consumes +32 bit1')
expect(b,0x12C62,'08 E8 00 03 00 34 08 A8 00 00 00 32 31 7C 04 00 00 58 31 7C 02 00 00 5C 31 7C 00 38 00 5E 31 7C 01 00 00 5A','Hawk transition-special launch')
expect(b,0x123B6,'08 A8 00 01 00 32','later transition clear A')
expect(b,0x12408,'08 A8 00 01 00 32','later transition clear B')
print('PASS Test269-ROM: wrestler state 8 sets +$32 bit1 in all 10 variants; Hawk $12C22 consumes that exact flag to enter +$34 bit3 special launch, with later transition paths clearing it')
